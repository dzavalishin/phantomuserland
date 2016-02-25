/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#include "vm/object.h"
#include "vm/alloc.h"
#include "vm/exception.h"
#include "vm/internal_da.h"
#include "vm/object_flags.h"
#include "vm/exec.h"

#include <phantom_libc.h>

//#include <stdio.h>
//#include <string.h>

//#define da_po_limit(o)	 (o->_da_size/sizeof(pvm_object))
//#define da_po_ptr(da)  ((pvm_object)&da)

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
    if( o.data )
    {
        verify_p( o.data );
        verify_p( o.interface );
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

struct pvm_object  pvm_get_array_ofield(struct pvm_object_storage *o, unsigned int slot  )
{
    verify_p(o);
    if(
       !(PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & (o->_flags) ) ||
       !( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & (o->_flags) )
      )
        pvm_exec_panic( "attempt to do an array op to non-array" );

    struct data_area_4_array *da = (struct data_area_4_array *)&(o->da);

    if( slot >= da->used_slots )
        pvm_exec_panic( "load: array index out of bounds" );

    return pvm_get_ofield( da->page, slot);
}

// TODO need semaphores here
void pvm_set_array_ofield(struct pvm_object_storage *o, unsigned int slot, struct pvm_object value )
{
    verify_p(o);
    verify_o(value);
    if(
       !(PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & (o->_flags) ) ||
       !( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & (o->_flags) )
      )
        pvm_exec_panic( "attempt to do an array op to non-array" );

    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE &  (o->_flags) )
        pvm_exec_panic( "attempt to set_array_ofield for immutable" );


    struct data_area_4_array *da = (struct data_area_4_array *)&(o->da);

    // need resize?
    if( pvm_is_null(da->page) || slot >= da->page_size )
        {
        const int step = 1024;
        int new_page_size = slot < (step/2) ? (slot * 2) : (slot+step);

        if(new_page_size < 16) new_page_size = 16;

        if( (!pvm_is_null(da->page)) && da->page_size > 0 )
            //da->page = pvm_object_storage::create_page( new_page_size, da->page.data()->da_po_ptr(), da->page_size );
            da->page = pvm_create_page_object( new_page_size, (void *)&(da->page.data->da), da->page_size );
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
void pvm_append_array(struct pvm_object_storage *array, struct pvm_object value_to_append )
{
    struct data_area_4_array *da = (struct data_area_4_array *)&(array->da);

    pvm_set_array_ofield(array, da->used_slots, value_to_append );
}

int get_array_size(struct pvm_object_storage *array)
{
    struct data_area_4_array *da = (struct data_area_4_array *)&(array->da);
    return da->used_slots;
}

void pvm_pop_array(struct pvm_object_storage *array, struct pvm_object value_to_pop )
{
    struct data_area_4_array *da = (struct data_area_4_array *)&(array->da);

    verify_p(array);
    if(
       !(PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & (array->_flags) ) ||
       !( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & (array->_flags) )
      )
        pvm_exec_panic( "attempt to do an array op to non-array" );

    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE &  (array->_flags) )
        pvm_exec_panic( "attempt to pop_array for immutable" );

    //swap with last and decrement used_slots
    struct pvm_object *p = da_po_ptr((da->page.data)->da);
    unsigned int slot;
    for( slot = 0; slot < da->used_slots; slot++ )
    {
        if ( ( p[slot] ).data == value_to_pop.data )  //please don't leak refcnt
        {
            if (slot != da->used_slots-1) {
                p[slot] = p[da->used_slots-1];
            }
            da->used_slots--;
            return;
        }
    }

    pvm_exec_panic( "attempt to remove non existing element from array" );
}



/**
 *
 * Fields access for noninternal ones.
 *
**/

// TODO BUG XXX - races possible, see below
struct pvm_object
pvm_get_field( struct pvm_object_storage *o, unsigned int slot )
{
    verify_p(o);
    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & (o->_flags) )
    {
        if( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & (o->_flags) )
        {
            return pvm_get_array_ofield( o, slot );
        }
        pvm_exec_panic( "attempt to load from internal" );
    }

    if( slot >= da_po_limit(o) )
    {
        pvm_exec_panic( "save: slot index out of bounds" );
    }

    verify_o(da_po_ptr(o->da)[slot]);
    return da_po_ptr(o->da)[slot];
}

// TODO BUG XXX - races possible, read obj, then other thread writes
// to slot (derements refctr and kills object), then we attempt to
// use it (even increment refctr) -> death. Need atomic (to slot write? to refcnt dec?)
// refcnt incr here
struct pvm_object
pvm_get_ofield( struct pvm_object op, unsigned int slot )
{
    verify_o(op);
    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & ((op.data)->_flags) )
    {
        if( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & ((op.data)->_flags) )
        {
            return pvm_get_array_ofield( op.data, slot );
        }
        pvm_exec_panic( "attempt to load from internal" );
    }

    if( slot >= da_po_limit(op.data) )
    {
        pvm_exec_panic( "load: slot index out of bounds" );
    }

    verify_o(da_po_ptr((op.data)->da)[slot]);
    return da_po_ptr((op.data)->da)[slot];
}


void
pvm_set_field( struct pvm_object_storage *o, unsigned int slot, struct pvm_object value )
{
    verify_p(o);
    verify_o(value);
    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & (o->_flags) )
    {
        if( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & (o->_flags) )
        {
            pvm_set_array_ofield( o, slot, value );
            return;
        }
        pvm_exec_panic( "attempt to save to internal" );
    }

    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE &  (o->_flags) )
        pvm_exec_panic( "attempt to set_field for immutable" );

    if( slot >= da_po_limit(o) )
    {
        pvm_exec_panic( "load: slot index out of bounds" );
    }

    if(da_po_ptr(o->da)[slot].data)     ref_dec_o(da_po_ptr(o->da)[slot]);  //decr old value
    da_po_ptr(o->da)[slot] = value;
}

void
pvm_set_ofield( struct pvm_object op, unsigned int slot, struct pvm_object value )
{
    verify_o(op);
    verify_o(value);
    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL & ((op.data)->_flags) )
    {
        if( PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE & ((op.data)->_flags) )
        {
            pvm_set_array_ofield( op.data, slot, value );
            return;
        }
        pvm_exec_panic( "attempt to save to internal" );
    }

    if( PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE &  (op.data->_flags) )
        pvm_exec_panic( "attempt to set_ofield for immutable" );


    if( slot >= da_po_limit(op.data) )
    {
        pvm_exec_panic( "slot index out of bounds" );
    }

    if(da_po_ptr((op.data)->da)[slot].data) ref_dec_o(da_po_ptr((op.data)->da)[slot]);  //decr old value
    da_po_ptr((op.data)->da)[slot] = value;
}



/**
 *
 * Class is: checks if object's class is tclass or it's parent.
 *
**/
/*
int pvm_object_class_is( struct pvm_object object, struct pvm_object tclass )
{
    struct pvm_object_storage *tested = object.data->_class.data;
    struct pvm_object_storage *nullc = pvm_get_null_class().data;

    while( !pvm_is_null( tclass ) )
    {
        if( tested == tclass.data )
            return 1;

        if( tclass.data == nullc )
            break;

        tclass = pvm_object_da( tclass, class )->class_parent;
    }
    return 0;
}
*/

int pvm_object_class_exactly_is( struct pvm_object object, struct pvm_object tclass )
{
    struct pvm_object_storage *tested = object.data->_class.data;
    //struct pvm_object_storage *nullc = pvm_get_null_class().data;

    if( (!pvm_is_null( tclass )) && (tested == tclass.data) )
        return 1;

    return 0;
}

// Really need this?
int pvm_object_class_is_or_parent( struct pvm_object object, struct pvm_object tclass )
{
    struct pvm_object_storage *tested = object.data->_class.data;
    struct pvm_object_storage *nullc = pvm_get_null_class().data;

    while( !pvm_is_null( tclass ) )
    {
        if( tested == tclass.data )
            return 1;

        if( tclass.data == nullc )
            break;

        tclass = pvm_object_da( tclass, class )->class_parent;
    }
    return 0;
}

int pvm_object_class_is_or_child( struct pvm_object object, struct pvm_object tclass )
{
    struct pvm_object oclass = object.data->_class;
    //struct pvm_object_storage *tested = object.data->_class.data;
    struct pvm_object_storage *nullc = pvm_get_null_class().data;

    if( pvm_is_null( tclass ) ) return 0;

    while(1)
    {
        if( oclass.data == tclass.data )
            return 1;

        if( oclass.data == nullc )
            break;

        oclass = pvm_object_da( oclass, class )->class_parent;
    }
    return 0;
}


/**
 *
 * Shallow copy of the object
 *
**/

struct pvm_object
pvm_copy_object( struct pvm_object in_object )
{
    // TODO ERROR throw!
    if(in_object.data->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL)
        panic("internal object copy?!");

    //ref_inc_o( in_object.data->_class );  //increment if class is refcounted
    struct pvm_object in_class = in_object.data->_class;
    int da_size = in_object.data->_da_size;

    struct pvm_object out = pvm_object_create_dynamic( in_class, da_size );

    int i = da_size/sizeof(struct pvm_object);
    for(; i >= 0; i-- )
    {
        if(da_po_ptr((in_object.data)->da)[i].data)
            ref_inc_o( da_po_ptr((in_object.data)->da)[i] );
    }

    memcpy( out.data->da, in_object.data->da, da_size );
    // TODO: check for special cases - copy c'tor?

    return out;
}



pvm_object_t pvm_storage_to_object(pvm_object_storage_t *st)
{
    pvm_object_t ret;

    ret.data = st;
    ret.interface = pvm_get_default_interface(st).data;

    return ret;
}



/**
 *
 * Debug.
 *
**/

void pvm_puts(struct pvm_object o )
{
    if(o.data->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING)
    {
        struct data_area_4_string *da = (struct data_area_4_string *)&(o.data->da);
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

void pvm_object_print(struct pvm_object o )
{
    if(o.data->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT)
    {
        printf( "%d", pvm_get_int( o ) );
    }
    else if(o.data->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING)
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

void print_object_flags(struct pvm_object_storage *o)
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
    struct pvm_object_storage *o = (struct pvm_object_storage *)addr;

    printf("Flags: '");
    print_object_flags(o);
    printf("'\n");
    printf("Da size: %ld\n", (long)(o->_da_size) );


    if(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING)
    {
        printf("String: '");

        struct data_area_4_string *da = (struct data_area_4_string *)&(o->da);
        int len = da->length;
        unsigned const char *sp = da->data;
        while( len-- )
        {
            putchar(*sp++);
        }
        printf("'\n");
    }
    if(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS)
    {
        struct data_area_4_class *da = (struct data_area_4_class *)&(o->da);
        printf("Is class: '"); pvm_object_print( da->class_name ); printf("'\n");
    }
    else
    {
        // Don't dump class class
        printf("Class: { "); dumpo( (addr_t)(o->_class.data) ); printf("}\n");
        //pvm_object_print( o->_class );
    }
}

void pvm_object_dump(struct pvm_object o )
{
	dumpo((addr_t)o.data);
}

struct pvm_object
pvm_get_class_name( struct pvm_object o )
{
    struct pvm_object c = o.data->_class;
    if(c.data->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS)
    {
        struct data_area_4_class *da = (struct data_area_4_class *)&(c.data->da);
        ref_inc_o( da->class_name );
        return da->class_name;
    }
    else
        return pvm_create_string_object("(class corrupt?)");
}


void pvm_check_is_thread( struct pvm_object new_thread )
{
    struct pvm_object_storage	* d = new_thread.data;

    if( d->_flags != (PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_THREAD) )
        panic("Thread object has no INTERNAL flag");

    if( !pvm_object_class_exactly_is( new_thread, pvm_get_thread_class() ) )
        panic("Thread object class is not .internal.thread");
}


