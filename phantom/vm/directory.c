/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Directory: hash map of strings to pvm objects
 *
 *
**/


#define DEBUG_MSG_PREFIX "vm.dir"
#include <debug_ext.h>
#define debug_level_flow 11
#define debug_level_error 11
#define debug_level_info 11

#include <hashfunc.h>
#include <vm/syscall.h>
#include <vm/object.h>
#include <vm/alloc.h>
#include <vm/internal_da.h>
#include <spinlock.h>

#define debug_print 0


// TODO very long spin lock
// TODO crashes for accessed objects can be paged out
// pagefault in closed interrupts and spinlock is forbidden

#warning lock
#if DIR_MUTEX_O
#  define LOCK_DIR(__dir) pvm_spin_lock(&(__dir)->pvm_lock)
#  define UNLOCK_DIR(__dir) pvm_spin_unlock(&(__dir)->pvm_lock)
#else
#  define LOCK_DIR(__dir) hal_wired_spin_lock(&(__dir)->lock)
#  define UNLOCK_DIR(__dir) hal_wired_spin_unlock(&(__dir)->lock)
#endif
//#warning SYS_FREE_O() for removed values - array access funcs do it
#warning do we need ref_inc for both saved objects and returned copies in get?

//#  define LOCK_DIR(__dir)
//#  define UNLOCK_DIR(__dir)

/**
 *
 * BUG! TODO no mutex, TODO general short-term sync primitive for persistent mem
 *
 * General design:
 *
 * 3 arrays, array of strings (keys), array of obvects (values), bit array of flags - single or multiple value in dir entry.
 * Hash func points to array slot.
 * If we have just one entry for given key, just hold it and its key in corresponding arrays. Have 0 in multiple flag array.
 * If we have more than one entry for given key, hold 2nd level arrays (for values and keys) in corresponding arrays. Have 1 in multiple flag array.
 * In 2nd level arrays key and value positions are correspond too. Null in key slot menas that slot is unused.
 *
**/
/*
typedef struct hashdir {
    u_int32_t                           capacity;       // size of 1nd level arrays
    u_int32_t                           nEntries;       // number of actual entries stored

    struct pvm_object   		keys;      	// Where we actually hold keys
    struct pvm_object   		values;      	// Where we actually hold values
    u_int8_t 				*flags;      	// Is this keys/values slot pointing to 2nd level array

    hal_spinlock_t                      lock;

} hashdir_t;
*/
static int hdir_cmp_keys( const char *ikey, size_t ikey_len, pvm_object_t okey );

/**
 *
 * \brief Lookup object in hash table.
 *
 * \param[in]  dir          Directory (hash table) to lookup in
 * \param[in]  ikey         Key to lookup for (string)
 * \param[in]  i_key_len    Key length
 * \param[out] out          Object found (returned even if delete requested)
 * \param[in]  delete_found Delete key/value from hash
 * 
 * \return 0 if found, ENOENT if not found
 *
 * Actually implements '.internal.directory' internal class
 *
**/

errno_t hdir_find( hashdir_t *dir, const char *ikey, size_t i_key_len, pvm_object_t *out, int delete_found )
{
    if( dir->nEntries == 0 ) return ENOENT;

    assert(dir->capacity);
    assert(dir->keys.data != 0);
    assert(dir->values.data != 0);
    assert(dir->flags != 0);

    LOCK_DIR(dir);

    //hexdump( dir->flags, dir->capacity, "find hdir flags", 0 );

    int keypos = calc_hash( ikey, ikey+i_key_len ) % dir->capacity;

    // Can't access array slot out of array's real size
    int kasize = get_array_size( dir->keys.data );
    if( keypos >= kasize )
        goto not_found;

    pvm_object_t okey = pvm_get_array_ofield( dir->keys.data, keypos );
    if( pvm_is_null( okey ) )
    {
    not_found:
        UNLOCK_DIR(dir);
        return ENOENT;
    }

    u_int8_t flags = dir->flags[keypos];
    //if(debug_print) lprintf("find keypos = %d flags = %d\n", keypos, flags );

    // No indirection?
    if( flags == 0 )
    {
        if( 0 == hdir_cmp_keys( ikey, i_key_len, okey ) )
        {
            *out = pvm_get_array_ofield( dir->values.data, keypos );
            ref_inc_o( *out );
            if( delete_found )
            {
                // TODO need ref_dec?
                pvm_set_array_ofield( dir->values.data, keypos, pvm_create_null_object() );
                pvm_set_array_ofield( dir->keys.data, keypos, pvm_create_null_object() );
                dir->nEntries--;
            }
            UNLOCK_DIR(dir);
            if(debug_print) lprintf("hdir found\n");
            return 0;
        }
        UNLOCK_DIR(dir);
        return ENOENT; // Not found
    }

    // Indirect, find by linear search in 2nd level array
    // okey is arrray

    pvm_object_t keyarray = pvm_get_array_ofield( dir->keys.data, keypos );
    pvm_object_t valarray = pvm_get_array_ofield( dir->values.data, keypos );

    if(debug_print) lprintf("find indirect keypos = %d\n", keypos );
    SHOW_INFO( 10, "find indirect keypos = %d", keypos );
    //if( debug_print || (debug_level_flow >= 10) )
    if( debug_print )
    {
        pvm_object_dump( keyarray );
        pvm_object_dump( valarray );
    }

    if( pvm_is_null( valarray ) )
    {
        lprintf("keyarray exists, valarray not"); // don't panic ;), but actually it is inconsistent
        UNLOCK_DIR(dir);
        return ENOENT;
    }

    size_t indir_size = get_array_size( keyarray.data );

    int i;
    for( i = 0; i < indir_size; i++ )
    {
        pvm_object_t indir_key = pvm_get_array_ofield( keyarray.data, i );
        
        if( pvm_is_null(indir_key) )
            continue;

        if( 0 == hdir_cmp_keys( ikey, i_key_len, indir_key ) )
        {
            *out = pvm_get_array_ofield( valarray.data, i );
            ref_inc_o( *out );
            if( delete_found ) // TODO refdec?
            {
                pvm_set_array_ofield( valarray.data, i, pvm_create_null_object() );
                pvm_set_array_ofield( keyarray.data, i, pvm_create_null_object() );
                dir->nEntries--;
            }
            UNLOCK_DIR(dir);
            //lprintf("hdir found\n");
            return 0;
        }
    }

    UNLOCK_DIR(dir);
    return ENOENT;

}


//! Return EEXIST if dup
errno_t hdir_add( hashdir_t *dir, const char *ikey, size_t i_key_len, pvm_object_t add )
{
    pvm_object_t okey;

    assert(dir->capacity);
    assert(dir->keys.data != 0);
    assert(dir->values.data != 0);
    assert(dir->flags != 0);

    if(debug_print) printf("---- hdir add %.*s\n", i_key_len, ikey );

    //hexdump( dir->flags, dir->capacity, "find add flags", 0 );

    LOCK_DIR(dir);

    int keypos = calc_hash( ikey, ikey+i_key_len ) % dir->capacity;

    // Can't access array slot out of array's real size
    int kasize = get_array_size( dir->keys.data );
#if 1 // turn off to generate pvm_panic and, if E4C enabled, exception
    if( keypos >= kasize )
        okey = pvm_create_null_object();
    else
#endif
        okey = pvm_get_array_ofield( dir->keys.data, keypos );
    u_int8_t flags = dir->flags[keypos];

    // No indirection and key is equal
    if( (!flags) && !pvm_is_null( okey ) )
    {
        if( 0 == hdir_cmp_keys( ikey, i_key_len, okey ) )
        {
            UNLOCK_DIR(dir);
            return EEXIST;
        }
    }

    // No indirection and corresponding key slot is empty
    if( (!flags) && pvm_is_null( okey ) )
    {
        // Just put

        if(debug_print) lprintf("put direct keypos = %d\n", keypos );
        pvm_set_array_ofield( dir->keys.data, keypos, pvm_create_string_object_binary( ikey, i_key_len ) );
        pvm_set_array_ofield( dir->values.data, keypos, add );
        ref_inc_o( add );
        dir->nEntries++;

        if(debug_print) lprintf("put direct flags[] = %d\n", dir->flags[keypos] );

        UNLOCK_DIR(dir);
        return 0;
    }

    // No indirection, stored key exists and not equal to given
    // Need to convert 1-entry to indirection

    if( (!flags) && !pvm_is_null( okey ) && (0 != hdir_cmp_keys( ikey, i_key_len, okey )) )
    {
        if(debug_print) lprintf("put convert to indirect keypos = %d\n", keypos );
        pvm_object_t oval = pvm_get_array_ofield( dir->values.data, keypos );

        // Now create arrays
        pvm_object_t keya = pvm_create_array_object();
        pvm_object_t vala = pvm_create_array_object();

        // put old key/val
        pvm_append_array( keya.data, okey );
        pvm_append_array( vala.data, oval );

        // pvm_set_array_ofield below will clear prev values in dir->keys.data[keypos] and dir->values.data[keypos]
        // so ref inc 'em here

        ref_inc_o( okey );
        ref_inc_o( oval );

        // put new key/val
        pvm_append_array( keya.data, pvm_create_string_object_binary( ikey, i_key_len ) );
        pvm_append_array( vala.data, add );
        ref_inc_o( add );
        dir->nEntries++;

        pvm_set_array_ofield( dir->keys.data, keypos, keya );
        pvm_set_array_ofield( dir->values.data, keypos, vala );
        dir->flags[keypos] = 1;

        SHOW_INFO( 10, "create indirection keypos = %d", keypos );
        //if(debug_level_flow >= 10 )
        if( debug_print )
        {
            pvm_object_dump( keya );
            pvm_object_dump( vala );
        }

        UNLOCK_DIR(dir);
        return 0;
    }

    assert(flags);

    // Indirection. Scan and check for key to exist

    if(debug_print) lprintf("put to existing indirect keypos = %d\n", keypos );

    size_t indir_size = get_array_size( dir->keys.data );

    int i, empty_pos = -1;
    for( i = 0; i < indir_size; i++ )
    {
        pvm_object_t indir_key = pvm_get_array_ofield( dir->keys.data, i );
        if(pvm_is_null(indir_key))
        {
            empty_pos = i;
            continue;
        }

        // Have key
        if( 0 == hdir_cmp_keys( ikey, i_key_len, indir_key ) )
        {
            UNLOCK_DIR(dir);
            return EEXIST;
        }
    }

    // put new key/val
    pvm_object_t new_key = pvm_create_string_object_binary( ikey, i_key_len );
    if( empty_pos >= 0 )
    {
        pvm_set_array_ofield( dir->keys.data, empty_pos, new_key );
        pvm_set_array_ofield( dir->values.data, empty_pos, add );
    }
    else
    {
        pvm_append_array( dir->keys.data, new_key );
        pvm_append_array( dir->values.data, add );
    }
    ref_inc_o( add );
    dir->nEntries++;

    UNLOCK_DIR(dir);
    return 0;
}


static int hdir_cmp_keys( const char *ikey, size_t ikey_len, pvm_object_t okey )
{
    size_t okey_len = pvm_get_str_len(okey);

    if(okey_len > ikey_len) return 1;
    if(ikey_len > okey_len) return -1;

    const char *okeyp = pvm_get_str_data(okey);

    int i;
    for( i = 0; i < ikey_len; i++ )
    {
        if( *okeyp > *ikey ) return 1;
        if( *ikey > *okeyp ) return 1;
    }

    return 0;
}

#define DEFAULT_SIZE 256

//! Return EEXIST if dup
static errno_t hdir_init( hashdir_t *dir, size_t initial_size )
{
    if( initial_size <= 10)
        initial_size = DEFAULT_SIZE;

    if( initial_size > 10000 )
    {
        SHOW_ERROR( 0, "hdir_init size %d", initial_size );
        initial_size = DEFAULT_SIZE;
    }

    printf("---- hdir init sz %d\n", initial_size );

#if DIR_MUTEX_O
    pvm_spin_init( &dir->pvm_lock );
#else
    hal_spin_init( &dir->lock );
#endif
    LOCK_DIR(dir);

    dir->nEntries = 0;
    dir->capacity = initial_size;

    dir->keys = pvm_create_array_object();
    dir->values = pvm_create_array_object();
    dir->flags = calloc( sizeof(u_int8_t), initial_size );

    //hexdump( dir->flags, dir->capacity, "hdir flags", 0 );

    UNLOCK_DIR(dir);

    return 0;
}













// -----------------------------------------------------------------------
//
// Internal class table entry points
//
// (From create.c)
//
// -----------------------------------------------------------------------






void pvm_internal_init_directory(struct pvm_object_storage * os)
{
    struct data_area_4_directory      *da = (struct data_area_4_directory *)os->da;

    //da->elSize = 256;
    //da->capacity = 16;
    //da->nEntries = 0;

    //da->container = pvm_create_binary_object( da->elSize * da->capacity , 0 );
    printf("---- hdir init obj\n" );

    errno_t rc = hdir_init( da, 0 );
    if( rc ) panic("can't create directory"); // TODO do not panic? must return errno?
}


// left in create.c
/*
void pvm_gc_iter_directory(gc_iterator_call_t func, struct pvm_object_storage * os, void *arg)
{
    struct data_area_4_directory      *da = (struct data_area_4_directory *)os->da;

    gc_fcall( func, arg, da->keys );
    gc_fcall( func, arg, da->values );
}

struct pvm_object     pvm_create_directory_object(void)
{
    pvm_object_t ret = pvm_object_create_fixed( pvm_get_directory_class() );
    return ret;
} */

// Unused, not supposed to be called
void pvm_gc_finalizer_directory( struct pvm_object_storage * os )
{
    //struct data_area_4_window      *da = (struct data_area_4_window *)os->da;
    assert(0);
}

// Unused, not supposed to be called
void pvm_restart_directory( pvm_object_t o )
{
    //struct data_area_4_directory *da = pvm_object_da( o, directory );
    assert(0);
}


