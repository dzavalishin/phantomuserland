/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Object access
 *
**/


#include "vm/object.h"
#include "vm/alloc.h"
#include "vm/exception.h"
#include "vm/internal_da.h"
#include "vm/object_flags.h"
#include "vm/exec.h"

#include <phantom_libc.h>



#if 1
static inline void verify_p( pvm_object_storage_t *p )
{
    debug_catch_object( "V", p );

    if( p && ! pvm_object_is_allocated_light(p))
    {
        dumpo((addr_t)p);
        //pvm_object_is_allocated_assert(p);
        panic("freed object accessed");
    }
}

static inline void verify_o( pvm_object_t o )
{
    if( o )
    {
        verify_p( o );
        //verify_p( o.interface );
    }
}
#else
#define verify_p()
#define verify_o()
#endif

/**
 *
 * Fields access for array.
 *
**/

pvm_object_t  pvm_get_array_ofield(pvm_object_t o, unsigned int slot  )
{
    verify_p(o);
    if(
       !(PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & (o->_flags) ) ||
       !( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & (o->_flags) )
      )
        pvm_exec_panic0( "attempt to do an array op to non-array" );

    struct data_area_4_array *da = (struct data_area_4_array *)&(o->da);

    if( slot >= da->used_slots )
        pvm_exec_panic0( "load: array index out of bounds" );

    return pvm_get_ofield( da->page, slot);
}

// TODO need semaphores here
void pvm_set_array_ofield(pvm_object_t o, unsigned int slot, pvm_object_t value )
{
    verify_p(o);
    verify_o(value);
    if(
       !(PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & (o->_flags) ) ||
       !( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & (o->_flags) )
      )
        pvm_exec_panic0( "attempt to do an array op to non-array" );

    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE &  (o->_flags) )
        pvm_exec_panic0( "attempt to set_array_ofield for immutable" );


    struct data_area_4_array *da = (struct data_area_4_array *)&(o->da);

    // need resize?
    if( pvm_is_null(da->page) || slot >= da->page_size )
        {
        const int step = 1024;
        int new_page_size = slot < (step/2) ? (slot * 2) : (slot+step);

        if(new_page_size < 16) new_page_size = 16;

        if( (!pvm_is_null(da->page)) && da->page_size > 0 )
            //da->page = pvm_object_storage::create_page( new_page_size, da->page()->da_po_ptr(), da->page_size );
            da->page = pvm_create_page_object( new_page_size, (void *)&(da->page->da), da->page_size );
        else
            da->page = pvm_create_page_object( new_page_size, 0, 0 );

        da->page_size = new_page_size;
        }

    if( slot >= da->used_slots )
        {
        // set to null all new slots except for indexed
        //for( int i = da->used_slots; i < index; i++ )
             //da->page.save(i, pvm_object());
        da->used_slots = slot+1;
        }

    pvm_set_ofield( da->page, slot, value );
}

// TODO need semaphores here
void pvm_append_array(pvm_object_t array, pvm_object_t value_to_append )
{
    struct data_area_4_array *da = (struct data_area_4_array *)&(array->da);

    pvm_set_array_ofield(array, da->used_slots, value_to_append );
}

int get_array_size(pvm_object_t array)
{
    struct data_area_4_array *da = (struct data_area_4_array *)&(array->da);
    return da->used_slots;
}

void pvm_pop_array(pvm_object_t array, pvm_object_t value_to_pop )
{
    struct data_area_4_array *da = (struct data_area_4_array *)&(array->da);

    verify_p(array);
    if(
       !(PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & (array->_flags) ) ||
       !( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & (array->_flags) )
      )
        pvm_exec_panic0( "attempt to do an array op to non-array" );

    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE &  (array->_flags) )
        pvm_exec_panic0( "attempt to pop_array for immutable" );

    //swap with last and decrement used_slots
    pvm_object_t *p = da_po_ptr((da->page)->da);
    unsigned int slot;
    for( slot = 0; slot < da->used_slots; slot++ )
    {
        if ( ( p[slot] ) == value_to_pop )  //please don't leak refcnt
        {
            if (slot != da->used_slots-1) {
                p[slot] = p[da->used_slots-1];
            }
            da->used_slots--;
            return;
        }
    }

    pvm_exec_panic0( "attempt to remove non existing element from array" );
}



/**
 *
 * Fields access for noninternal ones.
 * 
 * TODO BUG XXX - races possible, see below
 *
**/

pvm_object_t 
pvm_get_field( pvm_object_t o, unsigned int slot )
{
    verify_p(o);
    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & (o->_flags) )
    {
        if( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & (o->_flags) )
        {
            return pvm_get_array_ofield( o, slot );
        }
        pvm_exec_panic0( "attempt to load from internal" );
    }

    if( slot >= da_po_limit(o) )
    {
        pvm_exec_panic0( "get: slot index out of bounds" );
    }

    verify_o(da_po_ptr(o->da)[slot]);
    return da_po_ptr(o->da)[slot];
}
/*
// TODO BUG XXX - races possible, read obj, then other thread writes
// to slot (derements refctr and kills object), then we attempt to
// use it (even increment refctr) -> death. Need atomic (to slot write? to refcnt dec?)
// refcnt incr here
pvm_object_t 
pvm_get_ofield( pvm_object_t op, unsigned int slot )
{
    verify_o(op);
    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & ((op)->_flags) )
    {
        if( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & ((op)->_flags) )
        {
            return pvm_get_array_ofield( op, slot );
        }
        pvm_exec_panic0( "attempt to load from internal" );
    }

    if( slot >= da_po_limit(op) )
    {
        pvm_exec_panic0( "load: slot index out of bounds" );
    }

    verify_o(da_po_ptr((op)->da)[slot]);
    return da_po_ptr((op)->da)[slot];
}
*/

void
pvm_set_field( pvm_object_t o, unsigned int slot, pvm_object_t value )
{
    verify_p(o);
    verify_o(value);

    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE &  (o->_flags) )
        pvm_exec_panic0( "attempt to set_field for immutable" );

    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & (o->_flags) )
    {
        if( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & (o->_flags) )
        {
            pvm_set_array_ofield( o, slot, value );
            return;
        }
        pvm_exec_panic0( "attempt to save to internal" );
    }

    if( slot >= da_po_limit(o) )
    {
        pvm_exec_panic0( "set: slot index out of bounds" );
    }

    if(da_po_ptr(o->da)[slot])     ref_dec_o(da_po_ptr(o->da)[slot]);  //decr old value
    da_po_ptr(o->da)[slot] = value;
}
/*
void
pvm_set_ofield( pvm_object_t op, unsigned int slot, pvm_object_t value )
{
    verify_o(op);
    verify_o(value);
    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & ((op)->_flags) )
    {
        if( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & ((op)->_flags) )
        {
            pvm_set_array_ofield( op, slot, value );
            return;
        }
        pvm_exec_panic0( "attempt to save to internal" );
    }

    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE &  (op->_flags) )
        pvm_exec_panic0( "attempt to set_ofield for immutable" );


    if( slot >= da_po_limit(op) )
    {
        pvm_exec_panic0( "slot index out of bounds" );
    }

    if(da_po_ptr((op)->da)[slot]) ref_dec_o(da_po_ptr((op)->da)[slot]);  //decr old value
    da_po_ptr((op)->da)[slot] = value;
}
*/


/**
 *
 * Class exactly is: checks if object's class is tclass.
 *
**/


int pvm_object_class_exactly_is( pvm_object_t object, pvm_object_t tclass )
{
    if( pvm_is_null( tclass ) ) return 0;
    if( pvm_is_null( object ) ) return 0;

    pvm_object_t tested = object->_class;
    //pvm_object_t nullc = pvm_get_null_class();

    if( (!pvm_is_null( tclass )) && (tested == tclass) )
        return 1;

    return 0;
}

/**
 *
 * Class is or parent: checks if object's class is tclass or it's parent.
 *
**/

int pvm_object_class_is_or_parent( pvm_object_t object, pvm_object_t tclass )
{
    if( pvm_is_null( object ) ) return 0;

    pvm_object_t tested = object->_class;
    pvm_object_t nullc = pvm_get_null_class();

    while( !pvm_is_null( tclass ) )
    {
        if( tested == tclass )
            return 1;

        if( tclass == nullc )
            break;

        tclass = pvm_object_da( tclass, class )->class_parent;
    }
    return 0;
}

/**
 *
 * Class is or child: checks if object's class is tclass or it's child.
 *
**/


int pvm_object_class_is_or_child( pvm_object_t object, pvm_object_t tclass )
{
    if( pvm_is_null( tclass ) ) return 0;
    if( pvm_is_null( object ) ) return 0;

    pvm_object_t oclass = object->_class;
    pvm_object_t nullc = pvm_get_null_class();

    while( !pvm_is_null(oclass) )
    {
        struct data_area_4_class *oclass_da = pvm_object_da( oclass, class );

        if( oclass == tclass )
            return 1;

        if( oclass == nullc )
            break;
        oclass = oclass_da->class_parent;
    }
    return 0;
}


/**
 *
 * Shallow copy of the object
 *
** /

pvm_object_t 
pvm_copy_object( pvm_object_t in_object )
{
    // TODO ERROR throw!
    if(in_object->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL)
        panic("internal object copy?!");

    //ref_inc_o( in_object->_class );  //increment if class is refcounted
    pvm_object_t in_class = in_object->_class;
    int da_size = in_object->_da_size;

    pvm_object_t out = pvm_object_create_dynamic( in_class, da_size );

    int i = da_size/sizeof(pvm_object_t );
    for(; i >= 0; i-- )
    {
        if(da_po_ptr((in_object)->da)[i])
            ref_inc_o( da_po_ptr((in_object)->da)[i] );
    }

    memcpy( out->da, in_object->da, da_size );
    // TODO: check for special cases - copy c'tor?

    return out;
}
*/

/*
pvm_object_t pvm_storage_to_object(pvm_object_storage_t *st)
{
    pvm_object_t ret;

    ret.data = st;
    ret.interface = pvm_get_default_interface(st).data;

    return ret;
}
*/


/**
 *
 * Debug.
 *
**/

void pvm_puts(pvm_object_t o )
{
    if( pvm_is_null( o ) )
    {
        printf( "(null)" );
        return;
    }

    if(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING)
    {
        struct data_area_4_string *da = (struct data_area_4_string *)&(o->da);
        int len = da->length;
        unsigned const char *sp = da->data;
        /* TODO BUG! From unicode! */
        while( len-- )
        {
            putchar(*sp++);
        }
    }
    else
        printf( "?" );
}

void pvm_object_print(pvm_object_t o )
{
    if(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT)
    {
        printf( "%d", pvm_get_int( o ) );
    }
    else if(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING)
    {
        pvm_puts( o );
    }
    else 
    {
        pvm_object_t r = pvm_exec_run_method( o, 5, 0, 0 ); // ToString
        pvm_puts( r );
        ref_dec_o( r );
    }
}

void print_object_flags(pvm_object_t o)
{
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_FINALIZER )       printf("FINALIZER ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE )       printf("CHILDFREE ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_THREAD )          printf("THREAD ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME )     printf("STACK_FRAME ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME )      printf("CALL_FRAME ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL )        printf("INTERNAL ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE )      printf("RESIZEABLE ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING )          printf("STRING ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT )             printf("INT ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_DECOMPOSEABLE )   printf("DECOMPOSEABLE ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS )           printf("CLASS ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE )       printf("INTERFACE ");
    if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE )            printf("CODE ");
}

// For debugging - to call from GDB - arg is object storage!

void dumpo( addr_t addr )
{
    pvm_object_t o = (pvm_object_t)addr;

    if( o == 0)
    {
        printf("dumpo(0)\n");
        return;
    }

    printf("Flags: '");
    print_object_flags(o);
    printf("', ");
    //printf("', da size: %ld, ", (long)(o->_da_size) );


    if(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING)
    {
        printf("String: '");

        struct data_area_4_string *da = (struct data_area_4_string *)&(o->da);
        int len = da->length;
        // limit to realistic size
        if( len < 0 ) len = 0;
        if( len > 1024 ) len = 1024;
        unsigned const char *sp = da->data;
        while( len-- )
        {
            putchar(*sp++);
        }
        printf("'");
    }
    if(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS)
    {
        struct data_area_4_class *da = (struct data_area_4_class *)&(o->da);
        printf("Is class: '"); pvm_object_print( da->class_name ); printf("' @%p", o);
        if(
            (!pvm_isnull(da->class_parent)) &&
            (pvm_get_null_class() != da->class_parent )
            )
        {
            printf(" Parent: "); pvm_object_dump( da->class_parent ); 
            //printf(" Parent: '"); pvm_object_print( da->class_parent ); 
            //printf("' @%p", da->class_parent );
        }
    }
    else
    {
        // Don't dump class class
        //printf("Class: { "); dumpo( (addr_t)(o->_class) ); printf("}\n");
        //pvm_object_print( o->_class );
        pvm_object_t cl = o->_class;
        struct data_area_4_class *cda = (struct data_area_4_class *)&(cl->da);
        //printf("Class: '"); pvm_object_print( cda->class_name ); printf(" @%p'", o);
        printf("Class: '"); pvm_object_print( cda->class_name ); printf("' o@%p class@%p", o, cl );
    }
    printf("\n");
}

void pvm_object_dump(pvm_object_t o )
{
	dumpo((addr_t)o);
}

pvm_object_t 
pvm_get_class_name( pvm_object_t o )
{
    pvm_object_t c = o->_class;
    if(c->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS)
    {
        struct data_area_4_class *da = (struct data_area_4_class *)&(c->da);
        ref_inc_o( da->class_name );
        return da->class_name;
    }
    else
        return pvm_create_string_object("(class corrupt?)");
}


void pvm_check_is_thread( pvm_object_t new_thread )
{
    struct pvm_object_storage	* d = new_thread;

    if( d->_flags != (PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_THREAD) )
        panic("Thread object has no INTERNAL flag");

    if( !pvm_object_class_exactly_is( new_thread, pvm_get_thread_class() ) )
        panic("Thread object class is not .internal.thread");
}


