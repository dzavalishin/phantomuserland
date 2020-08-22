/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Create objects of some types
 *
**/


#define DEBUG_MSG_PREFIX "vm.create"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/vm.h>
#include <spinlock.h>

#include <vm/root.h>
#include <vm/exec.h>
#include <vm/object.h>
#include <vm/object_flags.h>
#include <vm/alloc.h>
#include <vm/internal.h>
#include <vm/internal_da.h>
#include <vm/spin.h>

#include "ids/opcode_ids.h"

#include <assert.h>

#include <video/screen.h>


/**
 *
 * Create general object.
 * TODO: catch for special object creation and throw?
 *
 **/


static pvm_object_t 
pvm_object_create_fixed( pvm_object_t object_class )
{
	return pvm_object_create_dynamic( object_class, -1 );
}

pvm_object_t 
pvm_object_create_dynamic( pvm_object_t object_class, int das )
{
	struct data_area_4_class *cda = (struct data_area_4_class *)(&(object_class->da));

	if( das < 0 )
		das = cda->object_data_area_size;

	unsigned int flags = cda->object_flags;

	pvm_object_t  out = pvm_object_alloc( das, flags, 0 );
	out->_class = object_class;
	//out->_da_size = das; // alloc does it
	//out->_flags = flags; // alloc does it

	//pvm_object_t ret;
	//ret = out;
	//ret.interface = cda->object_default_interface.data;

	return out;
}


/**
 *
 * The most general way to create object. Must be used if no other requirements exist.
 *
 **/

pvm_object_t     pvm_create_object(pvm_object_t type)
{
	struct data_area_4_class *cda = pvm_data_area(type,class);

	pvm_object_t ret = pvm_object_create_fixed(type);

	if( cda->object_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL )
	{
        // init internal object

		int rec = pvm_iclass_by_class( type );

		pvm_internal_classes[rec].init( ret );
	}

	return ret;
}






/**
 *
 * Create special null object.
 *
 **/

//__inline__
pvm_object_t     pvm_create_null_object()
{
    return pvm_root.null_object;  // already created once forever
}

void pvm_internal_init_void(pvm_object_t  os) { (void)os; }

void pvm_gc_iter_void(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void)func;
    (void)os;
    (void)arg;
    // Empty
}

pvm_object_t     pvm_create_int_object(int _value)
{
	pvm_object_t 	out = pvm_object_create_fixed( pvm_get_int_class() );
	((struct data_area_4_int*)&(out->da))->value = _value;
	return out;
}

void pvm_internal_init_int(pvm_object_t  os)
{
    (void)os;
}

void pvm_gc_iter_int(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void)os;
    (void)arg;
    (void)func;
    // Empty
}



pvm_object_t     pvm_create_long_object(int64_t _value)
{
	pvm_object_t 	out = pvm_object_create_fixed( pvm_get_long_class() );
	((struct data_area_4_long*)&(out->da))->value = _value;
	return out;
}

void pvm_internal_init_long(pvm_object_t  os)
{
    (void)os;
}

void pvm_gc_iter_long(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void)os;
    (void)arg;
    (void)func;
    // Empty
}


pvm_object_t     pvm_create_float_object(float _value)
{
	pvm_object_t 	out = pvm_object_create_fixed( pvm_get_float_class() );
	((struct data_area_4_float*)&(out->da))->value = _value;
	return out;
}

void pvm_internal_init_float(pvm_object_t  os)
{
    (void)os;
}

void pvm_gc_iter_float(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void)os;
    (void)arg;
    (void)func;
    // Empty
}




pvm_object_t     pvm_create_double_object(double _value)
{
	pvm_object_t 	out = pvm_object_create_fixed( pvm_get_double_class() );
	((struct data_area_4_double*)&(out->da))->value = _value;
	return out;
}

void pvm_internal_init_double(pvm_object_t  os)
{
    (void)os;
}

void pvm_gc_iter_double(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void)os;
    (void)arg;
    (void)func;
    // Empty
}














pvm_object_t     pvm_create_string_object_binary(const char *value, int n_bytes)
{
	int das = sizeof(struct data_area_4_string)+n_bytes;
	pvm_object_t string_class = pvm_get_string_class();

	pvm_object_t 	_data = pvm_object_create_dynamic( string_class, das );

	struct data_area_4_string* data_area = (struct data_area_4_string*)&(_data->da);

	pvm_internal_init_string(_data);

	if(value)
		memcpy(data_area->data, value, data_area->length = n_bytes );

	return _data;

}

pvm_object_t     pvm_create_string_object(const char *value)
{
	return pvm_create_string_object_binary(value, strlen(value));
}

pvm_object_t     pvm_create_string_object_binary_cat(
		const char *value1, int n_bytes1,
		const char *value2, int n_bytes2
)
{
	int das = sizeof(struct data_area_4_string)+n_bytes1+n_bytes2;
	pvm_object_t string_class = pvm_get_string_class();

	pvm_object_t 	_data = pvm_object_create_dynamic( string_class, das );

	struct data_area_4_string* data_area = (struct data_area_4_string*)&(_data->da);

	pvm_internal_init_string(_data);

	if(value1)
	{
		memcpy(data_area->data, value1, data_area->length = n_bytes1 );
		if(value2)
		{
			memcpy(data_area->data+n_bytes1, value2, n_bytes2 );
			data_area->length += n_bytes2;
		}
	}

	return _data;

}


void pvm_internal_init_string(pvm_object_t  os)
{
	struct data_area_4_string* data_area = (struct data_area_4_string*)&(os->da);

	memset( (void *)data_area, 0, os->_da_size );
	data_area->length = 0;
}

void pvm_gc_iter_string(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void)func;
    (void)os;
    (void)arg;
    // Empty
}




int pvm_strcmp(pvm_object_t s1, pvm_object_t s2)
{
    int l1 = pvm_get_str_len( s1 );
    int l2 = pvm_get_str_len( s2 );

    char *d1 = pvm_get_str_data( s1 );
    char *d2 = pvm_get_str_data( s2 );

    if( l1 > l2 ) return 1;
    if( l2 > l1 ) return -1;

    return strncmp( d1, d2, l1 );
}











//pvm_object_t     pvm_create_array_object();

pvm_object_t 
pvm_create_page_object( int n_slots, pvm_object_t *init, int init_slots )
{
	int das = n_slots*sizeof(pvm_object_t );

	pvm_object_t _data = pvm_object_create_dynamic( pvm_get_page_class(), das );

	pvm_object_t * data_area = (pvm_object_t *)&(_data->da);

	// NB! Bug! Here is the way to set object pointers to some garbage value
	//if( init_value )	memcpy( data_area, init_value, das );
	//else            	memset( data_area, 0, das );

	assert(init_slots < n_slots);

	int i;
	for( i = 0; i < init_slots; i++ )
		data_area[i] = *init++;

	for( ; i < n_slots; i++ )
		data_area[i] = pvm_get_null_object();

	return _data;
}

void pvm_internal_init_page(pvm_object_t  os)
{
	assert( ((os->_da_size) % sizeof(pvm_object_t )) == 0); // Natural num of

	int n_slots = (os->_da_size) / sizeof(pvm_object_t );
	pvm_object_t * data_area = (pvm_object_t *)&(os->da);

	int i;
	for( i = 0; i < n_slots; i++ )
		data_area[i] = pvm_get_null_object();
}

void pvm_gc_iter_page(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
	int n_slots = (os->_da_size) / sizeof(pvm_object_t );
	pvm_object_t * data_area = (pvm_object_t *)&(os->da);

	int i;
	for( i = 0; i < n_slots; i++ )
	{
		func( data_area[i], arg );
	}
}






pvm_object_t     pvm_create_call_frame_object()
{
	pvm_object_t _data = pvm_object_create_fixed( pvm_get_call_frame_class() );

	pvm_internal_init_call_frame( _data );
	return _data;
}

void pvm_internal_init_call_frame(pvm_object_t  os)
{
	//struct data_area_4_call_frame *da = pvm_data_area( os, call_frame );
	struct data_area_4_call_frame *da = (struct data_area_4_call_frame *)&(os->da);

	da->IP_max = 0;
	da->IP = 0;
	da->code = 0;

	da->this_object = pvm_get_null_object();
	da->prev = pvm_get_null_object();

	da->istack = pvm_create_istack_object();
	da->ostack = pvm_create_ostack_object();
	da->estack = pvm_create_estack_object();
}



#define gc_fcall( f, arg, o )   f( o, arg )


void pvm_gc_iter_call_frame(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
	struct data_area_4_call_frame *da = (struct data_area_4_call_frame *)&(os->da);
	gc_fcall( func, arg, da->this_object );
	gc_fcall( func, arg, da->prev ); // FYI - shall never be followed in normal situation, must contain zero data ptr if being considered by refcount
	gc_fcall( func, arg, da->istack );
	gc_fcall( func, arg, da->ostack );
	gc_fcall( func, arg, da->estack );
}


// interface is a quite usual phantom object.
// (just has a specific flag - for performance only!)
// we just need an internal interface creation code
// to be able to start Phantom from scratch

static pvm_object_t create_interface_worker( int n_methods )
{
	//if(debug_init) printf("create interface\n");

	int das = n_methods*sizeof(pvm_object_t );

	pvm_object_t ret = pvm_object_create_dynamic( pvm_get_interface_class(), das );
	return ret;
}


pvm_object_t     pvm_create_interface_object( int n_methods, pvm_object_t parent_class )
{
    pvm_object_t 	ret = create_interface_worker( n_methods );
    pvm_object_t * data_area = (pvm_object_t *)ret->da;

    if(pvm_is_null( parent_class ))
        pvm_exec_panic0( "create interface: parent is null" );

    pvm_object_t base_i =  ((struct data_area_4_class*)parent_class->da)->object_default_interface;

    int base_icount = da_po_limit(base_i);

    if(base_icount > n_methods)
    {
        // root classes have N_SYS_METHODS slots in interface, don't cry about that
        if( n_methods > N_SYS_METHODS )
            printf( " create interface: child has less methods (%d) than parent (%d)\n", n_methods, base_icount );
        base_icount = n_methods;
    }

    int i = 0;
    // copy methods from parent
    for( ; i < base_icount; i++ )
    {
        pvm_object_t baseMethod = (da_po_ptr(base_i->da))[i];
        ref_inc_o(baseMethod);
        data_area[i] = baseMethod;
    }

    // fill others with nulls
    for( ; i < n_methods; i++ )
        data_area[i] = pvm_get_null_object(); // null

    return ret;
}


void pvm_internal_init_interface(pvm_object_t  os)
{
	memset( os->da, 0, os->_da_size );
}

void pvm_gc_iter_interface(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void)func;
    (void)os;
    (void)arg;
    // Empty
    // Must not be called - this is not really an internal object
    pvm_exec_panic0("Interface GC iterator called");
}





pvm_object_t pvm_create_class_object(pvm_object_t name, pvm_object_t iface, int da_size)
{
	pvm_object_t _data = pvm_object_create_fixed( pvm_get_class_class() );

	struct data_area_4_class *      da = (struct data_area_4_class *)_data->da;

	da->object_data_area_size       = da_size;
	da->object_flags                = 0;
	da->object_default_interface    = iface;
	da->sys_table_id                = -1;

	da->class_name                  = name;
	da->class_parent                = pvm_get_null_class();

	return _data;
}

void pvm_internal_init_class(pvm_object_t  os)
{
	struct data_area_4_class *      da = (struct data_area_4_class *)os->da;

	da->object_data_area_size   	= 0;
	da->object_flags     		= 0;
	da->object_default_interface 	= pvm_get_null_class();
	da->sys_table_id             	= -1;

	da->class_name 			= pvm_get_null_object();
        da->class_parent		= pvm_get_null_class();

        da->static_vars                 = pvm_create_object( pvm_get_array_class() );
}


void pvm_gc_iter_class(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
	struct data_area_4_class *da = (struct data_area_4_class *)&(os->da);
	gc_fcall( func, arg, da->object_default_interface );
	gc_fcall( func, arg, da->class_name );
	gc_fcall( func, arg, da->class_parent );

        gc_fcall( func, arg, da->static_vars );

        gc_fcall( func, arg, da->ip2line_maps );
        gc_fcall( func, arg, da->method_names );
        gc_fcall( func, arg, da->field_names );
}



void pvm_internal_init_thread(pvm_object_t  os)
{
	struct data_area_4_thread *      da = (struct data_area_4_thread *)os->da;

	///hal_spin_init(&da->spin);
#if NEW_VM_SLEEP
    da->sleep_flag              = 0;
#endif
	//hal_cond_init(&(da->wakeup_cond), "VmThrdWake");

    pvm_spin_init( &da->lock );

	da->call_frame   			= pvm_create_call_frame_object();
	da->stack_depth				= 1;

	da->owner = 0;
	da->environment = 0;

	da->code.code     			= 0;
	da->code.IP                 = 0;
	da->code.IP_max             = 0;

	//da->_this_object 			= pvm_get_null_object();

    pvm_exec_load_fast_acc(da); // Just to fill shadows with something non-null
}


void pvm_gc_iter_thread(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
	struct data_area_4_thread *da = (struct data_area_4_thread *)&(os->da);
	gc_fcall( func, arg, da->call_frame );
	gc_fcall( func, arg, da->owner );
	gc_fcall( func, arg, da->environment );
}


void pvm_internal_init_code(pvm_object_t  os)
{
	struct data_area_4_code *      da = (struct data_area_4_code *)os->da;

	da->code_size     			= 0;
}

void pvm_gc_iter_code(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void)func;
    (void)os;
    (void)arg;
    // Empty
}



void pvm_internal_init_array(pvm_object_t  os)
{
	struct data_area_4_array *      da = (struct data_area_4_array *)os->da;

	da->used_slots     			= 0;
	da->page_size                       = 16;
	da->page                       = 0;
}


void pvm_gc_iter_array(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
	struct data_area_4_array *da = (struct data_area_4_array *)&(os->da);
	if(da->page != 0)
		gc_fcall( func, arg, da->page );
}



void pvm_internal_init_boot(pvm_object_t  os)
{
     (void)os;
     // Nohing!
}

void pvm_gc_iter_boot(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void)func;
    (void)os;
    (void)arg;
    // Empty
}




void pvm_internal_init_mutex(pvm_object_t  os)
{
    struct data_area_4_mutex *      da = (struct data_area_4_mutex *)os->da;

    da->waiting_threads_array = pvm_create_object( pvm_get_array_class() );
    pvm_spin_init( &da->lock );
    //hal_spin_init( &da->spinlock );
    //in_method = 0;
}

void pvm_gc_iter_mutex(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_mutex *      da = (struct data_area_4_mutex *)os->da;
    //int i;

    //pvm_spin_init( &da->pvm_lock );
//    in_method = 0;

    gc_fcall( func, arg, da->waiting_threads_array );

    //for( i = 0; i < MAX_MUTEX_THREADS; i++ )
    //    gc_fcall( func, arg, da->waiting_threads[i] );


    gc_fcall( func, arg, pvm_da_to_object(da->owner_thread) );
}





void pvm_internal_init_cond(pvm_object_t  os)
{
    struct data_area_4_cond *      da = (struct data_area_4_cond *)os->da;

    da->waiting_threads_array = pvm_create_object( pvm_get_array_class() );
}

void pvm_gc_iter_cond(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_cond *      da = (struct data_area_4_cond *)os->da;
    //int i;

    gc_fcall( func, arg, da->waiting_threads_array );

    //for( i = 0; i < MAX_MUTEX_THREADS; i++ )
    //    gc_fcall( func, arg, da->waiting_threads[i] );

    //gc_fcall( func, arg, pvm_da_to_object(da->owner_thread) );
}




void pvm_internal_init_sema(pvm_object_t  os)
{
    struct data_area_4_sema *      da = (struct data_area_4_sema *)os->da;

    da->waiting_threads_array = pvm_create_object( pvm_get_array_class() );
}

void pvm_gc_iter_sema(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{

    struct data_area_4_sema *      da = (struct data_area_4_sema *)os->da;
    //int i;

    gc_fcall( func, arg, da->waiting_threads_array );

    //for( i = 0; i < MAX_MUTEX_THREADS; i++ )
    //    gc_fcall( func, arg, da->waiting_threads[i] );

    gc_fcall( func, arg, pvm_da_to_object(da->owner_thread) );
}









void pvm_internal_init_binary(pvm_object_t  os)
{
    (void)os;
    //struct data_area_4_binary *      da = (struct data_area_4_binary *)os->da;
    // empty
}

void pvm_gc_iter_binary(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void)func;
    (void)os;
    (void)arg;
    // Empty
}


pvm_object_t     pvm_create_binary_object(int size, void *init)
{
	pvm_object_t ret = pvm_object_create_dynamic( pvm_get_binary_class(), size + sizeof(struct data_area_4_binary) );

	struct data_area_4_binary *da = (struct data_area_4_binary *)ret->da;
	da->data_size = size;
	if( init != NULL ) memcpy( da->data, init, size );
	return ret;
}



void pvm_internal_init_bitmap(pvm_object_t  os)
{
    (void)os;
    // Nothing
}

void pvm_gc_iter_bitmap(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
	struct data_area_4_bitmap *da = (struct data_area_4_bitmap *)&(os->da);
	if(da->image != 0)
		gc_fcall( func, arg, da->image );
}



void pvm_internal_init_world(pvm_object_t  os)
{
    (void)os;
    //struct data_area_4_binary *      da = (struct data_area_4_binary *)os->da;
    // empty
}

void pvm_gc_iter_world(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void)func;
    (void)os;
    (void)arg;
    // Empty
}



void pvm_internal_init_closure(pvm_object_t  os)
{
	struct data_area_4_closure *      da = (struct data_area_4_closure *)os->da;
	da->object = 0;
}

void pvm_gc_iter_closure(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_closure *      da = (struct data_area_4_closure *)os->da;
    //if(da->image.object != 0)
    gc_fcall( func, arg, da->object );
}





#if COMPILE_WEAKREF

void pvm_internal_init_weakref(pvm_object_t  os)
{
    struct data_area_4_weakref *      da = (struct data_area_4_weakref *)os->da;
    da->object = 0;
#if WEAKREF_SPIN
    hal_spin_init( &da->lock );
#else
    hal_mutex_init( &da->mutex, "WeakRef" );
#endif
}

void pvm_gc_iter_weakref(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_weakref *      da = (struct data_area_4_weakref *)os->da;

    (void) da;
    // No! We are weak ref and do not count our reference or make GC
    // know of it so that our reference does not prevent ref'd object
    // from being GC'ed
    //gc_fcall( func, arg, da->object );
}


pvm_object_t     pvm_create_weakref_object(pvm_object_t owned )
{
    if(owned->_satellites != 0)
        return owned->_satellites;

    pvm_object_t ret = pvm_object_create_fixed( pvm_get_weakref_class() );
    struct data_area_4_weakref *da = (struct data_area_4_weakref *)ret->da;

    // Interlocked to make sure no races can happen
    // (ref ass'ment seems to be non-atomic)

#if WEAKREF_SPIN
    wire_page_for_addr( &da->lock );
    int ie = hal_save_cli();
    hal_spin_lock( &da->lock );
#else
    hal_mutex_lock( &da->mutex );
#endif

    // No ref inc!
    da->object = owned;
    owned->_satellites = ret;

#if WEAKREF_SPIN
    hal_spin_unlock( &da->lock );
    if( ie ) hal_sti();
    unwire_page_for_addr( &da->lock );
#else
    hal_mutex_unlock( &da->mutex );
#endif

    return ret;
}



pvm_object_t pvm_weakref_get_object(pvm_object_t wr )
{
    struct data_area_4_weakref *da = pvm_object_da( wr, weakref );
    pvm_object_t out;

    // still crashes :(

    // HACK HACK HACK BUG - wiring target too. TODO need wire size parameter for page cross situations!
    wire_page_for_addr( &(da->object) );
    wire_page_for_addr( da->object );

    // All we do is return new reference to our object,
    // incrementing refcount before

#if WEAKREF_SPIN
    wire_page_for_addr( &da->lock );
    int ie = hal_save_cli();
    hal_spin_lock( &da->lock );
#else
    hal_mutex_lock( &da->mutex );
#endif

    // TODO should we check refcount before and return null if zero?
    if( 0 == da->object->_ah.refCount )
        printf("zero object in pvm_weakref_get_object\n");

    out = ref_inc_o( da->object );

#if WEAKREF_SPIN
    hal_spin_unlock( &da->lock );
    if( ie ) hal_sti();
    unwire_page_for_addr( &da->lock );
#else
    hal_mutex_unlock( &da->mutex );
#endif

    unwire_page_for_addr( da->object );
    unwire_page_for_addr( &(da->object) );

    return out;
}

#endif







void pvm_internal_init_window(pvm_object_t os)
{
    struct data_area_4_window      *da = (struct data_area_4_window *)os->da;

    //pvm_object_t bin = pvm_create_binary_object( PVM_MAX_TTY_PIXELS * 4 + sizeof(drv_video_window_t), 0 );
    pvm_object_t bin = pvm_create_binary_object( drv_video_window_bytes( PVM_DEF_TTY_XSIZE, PVM_DEF_TTY_YSIZE ) + sizeof(drv_video_window_t), 0 );
    da->o_pixels = bin;

    struct data_area_4_binary *bda = (struct data_area_4_binary *)bin->da;

    void *pixels = &bda->data;

    strlcpy( da->title, "Window", sizeof(da->title) );

    da->fg = COLOR_BLACK;
    da->bg = COLOR_WHITE;
    da->x = 0;
    da->y = 0;
    da->autoupdate = 1;

    //lprintf("pvm_internal_init_window w %p pix %p\n", &(da->w), pixels );

    drv_video_window_init( &(da->w), pixels, PVM_DEF_TTY_XSIZE, PVM_DEF_TTY_YSIZE, 100, 100, da->bg, WFLAG_WIN_DECORATED, da->title );

    {
    pvm_object_t o;
    o = os;


    da->connector = pvm_create_connection_object();
    struct data_area_4_connection *cda = (struct data_area_4_connection *)(da->connector->da);

    phantom_connect_object_internal(cda, 0, o, 0);

    // This object needs OS attention at restart
    // TODO do it by class flag in create fixed or earlier?
    pvm_add_object_to_restart_list( o );
    }

}


void pvm_gc_iter_window(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_window *da = (struct data_area_4_window *)os->da;

    gc_fcall( func, arg, da->connector );
    gc_fcall( func, arg, da->o_pixels );
}


pvm_object_t     pvm_create_window_object(pvm_object_t owned )
{
    //pvm_object_t ret = pvm_object_create_fixed( pvm_get_window_class() );
    pvm_object_t ret = pvm_create_object( pvm_get_window_class() );
    struct data_area_4_window *da = (struct data_area_4_window *)ret->da;

    //lprintf("pvm_create_window_object %p n", ret );

    (void)da;

    return ret;
}

void pvm_gc_finalizer_window( pvm_object_t  os )
{
    // is it called?
    struct data_area_4_window      *da = (struct data_area_4_window *)os->da;

    //struct data_area_4_binary *bda = (struct data_area_4_binary *)da->o_pixels->da;
    //void *pixels = &bda->data;

    drv_video_window_destroy(&(da->w));
}

#include <event.h>

void pvm_restart_window( pvm_object_t o )
{
    pvm_add_object_to_restart_list( o ); // Again!

    struct data_area_4_window *da = pvm_object_da( o, window );

    struct data_area_4_binary *bda = (struct data_area_4_binary *)da->o_pixels->da;
    window_handle_t pixels = (window_handle_t)&bda->data;

    printf("restart WIN\n");

    w_restart_init( &(da->w), pixels );

    //&(da->w)->title = da->title; // must be correct in snap? don't reset?
    w_set_title( &(da->w), da->title );

    /*
    queue_init(&(da->w.events));
    da->w.events_count = 0;

    iw_enter_allwq( &da->w );

    //event_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, &da->w );
    ev_q_put_win( 0, 0, UI_EVENT_WIN_REDECORATE, &da->w );
    */
    w_restart_attach( &(da->w) );
}




































/* moved to directory.c
void pvm_internal_init_directory(pvm_object_t  os)
{
    struct data_area_4_directory      *da = (struct data_area_4_directory *)os->da;

    da->elSize = 256;
    da->capacity = 16;
    da->nEntries = 0;

    da->container = pvm_create_binary_object( da->elSize * da->capacity , 0 );
}


void pvm_gc_iter_directory(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_directory      *da = (struct data_area_4_directory *)os->da;

    struct data_area_4_binary *bin = pvm_object_da( da->container, binary );

    void *bp = bin->data;
    int i;
    for( i = 0; i < da->nEntries; i++, bp += da->elSize )
    {
        gc_fcall( func, arg, *((pvm_object_t*)bp) );
    }

    gc_fcall( func, arg, da->container );
}



// Unused, not supposed to be called
void pvm_gc_finalizer_directory( pvm_object_t  os )
{
    // is it called?
    //struct data_area_4_window      *da = (struct data_area_4_window *)os->da;

}

// Unused, not supposed to be called
void pvm_restart_directory( pvm_object_t o )
{
    //struct data_area_4_directory *da = pvm_object_da( o, directory );

}
*/


pvm_object_t     pvm_create_directory_object(void)
{
    pvm_object_t ret = pvm_create_object( pvm_get_directory_class() );
    return ret;
}

void pvm_gc_iter_directory(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_directory      *da = (struct data_area_4_directory *)os->da;

    gc_fcall( func, arg, da->keys );
    gc_fcall( func, arg, da->values );
}












void pvm_internal_init_connection(pvm_object_t  os)
{
    struct data_area_4_connection      *da = (struct data_area_4_connection *)os->da;

    da->kernel = 0;
    memset( da->name, 0, sizeof(da->name) );
}


pvm_object_t     pvm_create_connection_object(void)
{
    pvm_object_t ret = pvm_create_object( pvm_get_connection_class() );
    return ret;
}

void pvm_gc_iter_connection(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_connection      *da = (struct data_area_4_connection *)os->da;

    pvm_object_t ot;
    //ot.interface = 0;
    ot = (void *) (((addr_t)da->owner)-DA_OFFSET());

    gc_fcall( func, arg, ot );
    gc_fcall( func, arg, da->p_kernel_state_object );
    gc_fcall( func, arg, da->callback );
}


void pvm_gc_finalizer_connection( pvm_object_t  os )
{
    // is it called?
    struct data_area_4_connection *da = (struct data_area_4_connection *)os->da;
    //#warning disconnect!
    errno_t ret = phantom_disconnect_object( da );
    if( ret )
        printf("automatic disconnect failed - %d\n", ret );


}

void pvm_restart_connection( pvm_object_t o )
{
    struct data_area_4_connection *da = pvm_object_da( o, connection );
printf("restarting connection");
    da->kernel = 0;

    int ret = pvm_connect_object(o,0);

    if( ret )
        printf("reconnect failed %d\n", ret );

//#warning restart!
}







void pvm_internal_init_tcp(pvm_object_t os)
{
    struct data_area_4_tcp      *da = (struct data_area_4_tcp *)os->da;

    da->connected = 0;
    //memset( da->name, 0, sizeof(da->name) );
}

pvm_object_t     pvm_create_tcp_object(void)
{
    return pvm_create_object( pvm_get_tcp_class() );
}

void pvm_gc_iter_tcp(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_tcp      *da = (struct data_area_4_tcp *)os->da;

    (void) da;

    //gc_fcall( func, arg, ot );
    //gc_fcall( func, arg, da->p_kernel_state_object );
    //gc_fcall( func, arg, da->callback );
}


void pvm_gc_finalizer_tcp( pvm_object_t  os )
{
    // is it called?
    struct data_area_4_tcp *da = (struct data_area_4_tcp *)os->da;
    if( da->connected )
    {
        lprintf("disconnect!\n");
        //pvm_tcp_disconnect();
    }
}

void pvm_restart_tcp( pvm_object_t o )
{
    struct data_area_4_tcp *da = pvm_object_da( o, tcp );

    //da->connected = 0;
    if( da->connected )
    {
        printf("restarting TCP - unimpl!");
    }

}










// -----------------------------------------------------------------------
// Specials
// -----------------------------------------------------------------------



pvm_object_t     pvm_create_code_object(int size, void *code)
{
	pvm_object_t ret = pvm_object_create_dynamic( pvm_get_code_class(), size + sizeof(struct data_area_4_code) );

	struct data_area_4_code *da = (struct data_area_4_code *)ret->da;
	da->code_size = size;
	memcpy( da->code, code, size );
	return ret;
}



pvm_object_t     pvm_create_thread_object(pvm_object_t start_cf )
{
	pvm_object_t ret = pvm_create_object( pvm_get_thread_class() );
	struct data_area_4_thread *da = (struct data_area_4_thread *)ret->da;

	da->call_frame = start_cf;
	da->stack_depth = 1;

	pvm_exec_load_fast_acc(da);

	// add to system threads list
	pvm_append_array(pvm_root.threads_list, ret);
	ref_inc_o(ret);

	// not for each and every one
	//phantom_activate_thread(ret);

	return ret;
}


void   pvm_release_thread_object( pvm_object_t thread )
{
    // the thread was decr once... while executing class loader code... TODO: should be rewritten!

    // remove from system threads list.
    pvm_pop_array(pvm_root.threads_list, thread);

    ref_dec_o( thread );  //free now
}




/**
 *
 * Create special syscall-only interface for internal classes.
 *
 **/

static pvm_object_t pvm_create_syscall_code( int sys_num );

// creates interface such that each method
// has just syscall (with a number corresponding to method no)
// and return
void pvm_fill_syscall_interface( pvm_object_t iface, int syscall_count )
{
	pvm_object_t *da = (pvm_object_t *)iface->da;

	int i;
	for( i = 0; i < syscall_count; i++ )
		da[i] = pvm_create_syscall_code( i );
}


// syscall code segment generator
static pvm_object_t pvm_create_syscall_code( int sys_num )
{
/*
	if( sys_num <= 15 )
	{
		char code[2];
		code[0] = opcode_sys_0+(unsigned char)sys_num;
		code[1] = opcode_ret;
		return pvm_create_code_object( sizeof(code), code );
	}
	else
*/    
	{
		char code[3];
		code[0] = opcode_sys_8bit;
		code[1] = (unsigned char)sys_num;
		code[2] = opcode_ret;
		return pvm_create_code_object( sizeof(code), code );
	}

}



