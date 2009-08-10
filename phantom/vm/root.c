/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#include <assert.h>
#include <phantom_libc.h>


// Will trigger const definitions in pvm_object_flags.h
#define POSF_CONST_INIT


#include "vm/root.h"
#include "vm/exec.h"
#include "vm/alloc.h"
#include "vm/internal.h"
#include "vm/internal_da.h"
#include "vm/object_flags.h"
#include "vm/exception.h"
#include "vm/bulk.h"

//#include "vm/systable_id.h"

static void pvm_create_root_objects();

static void pvm_save_root_objects();

static void set_root_from_table();

static void pvm_boot();

struct pvm_root_t pvm_root;


/**
 *
 * Either load OS state from disk or (if persistent memory is empty) create
 * a new object set for a new OS instance to start from.
 *
 * In a latter case special bootstrap object will be created and activated to
 * look for the rest of the system and bring it in.
 *
**/

void pvm_root_init(void)
{
    struct pvm_object_storage *root = get_root_object_storage();

    if(root->_ah.object_start_marker != PVM_OBJECT_START_MARKER)
    {
        // Not an object in the beginning of the address pace?
        // We have a fresh OS instance then. Set it up from scratch.

        //root->_ah.object_start_marker = PVM_OBJECT_START_MARKER;
        pvm_alloc_clear_mem();
        pvm_create_root_objects();
        pvm_save_root_objects();

        pvm_boot();

        return;
    }

    assert(root->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED);

    // internal classes
    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        pvm_internal_classes[i].class_object = pvm_get_field( root,
                       pvm_internal_classes[i].root_index
                       );
        ref_saturate_o( pvm_internal_classes[i].class_object );
    }

    set_root_from_table();


    pvm_root.null_object = pvm_get_field( root, PVM_ROOT_OBJECT_NULL );
    pvm_root.threads_list = pvm_get_field( root, PVM_ROOT_OBJECT_THREAD_LIST );
    pvm_root.sys_interface_object = pvm_get_field( root, PVM_ROOT_OBJECT_SYSINTERFACE );
    pvm_root.windows_list = pvm_get_field( root, PVM_ROOT_OBJECT_WINDOWS_LIST );
    pvm_root.users_list = pvm_get_field( root, PVM_ROOT_OBJECT_USERS_LIST );
    pvm_root.class_loader = pvm_get_field( root, PVM_ROOT_OBJECT_CLASS_LOADER );
    pvm_root.kernel_environment = pvm_get_field( root, PVM_ROOT_OBJECT_KERNEL_ENVIRONMENT );
    pvm_root.os_entry = pvm_get_field( root, PVM_ROOT_OBJECT_OS_ENTRY );

    ref_saturate_o( pvm_root.null_object );
    ref_saturate_o( pvm_root.sys_interface_object );
    ref_saturate_o( pvm_root.class_loader );
    ref_saturate_o( pvm_root.threads_list );
    ref_saturate_o( pvm_root.windows_list );
    ref_saturate_o( pvm_root.users_list );
    ref_saturate_o( pvm_root.kernel_environment );
    ref_saturate_o( pvm_root.os_entry );

}


static void pvm_save_root_objects()
{
    pvm_object_storage_t *root = get_root_object_storage();

    assert(root->_ah.object_start_marker == PVM_OBJECT_START_MARKER);

    // internal classes
    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        pvm_set_field( root,
                       pvm_internal_classes[i].root_index,
                       pvm_internal_classes[i].class_object
                     );
        // Make sure refcount can't kill it
        ref_saturate_o( pvm_internal_classes[i].class_object );
    }

    // Make sure refcount can't kill it

    ref_saturate_o( pvm_root.null_object );
    ref_saturate_o( pvm_root.sys_interface_object );
    ref_saturate_o( pvm_root.class_loader );
    ref_saturate_o( pvm_root.threads_list );
    ref_saturate_o( pvm_root.windows_list );
    ref_saturate_o( pvm_root.users_list );
    ref_saturate_o( pvm_root.kernel_environment );
    ref_saturate_o( pvm_root.os_entry );

    pvm_set_field( root, PVM_ROOT_OBJECT_NULL, pvm_root.null_object );
    pvm_set_field( root, PVM_ROOT_OBJECT_SYSINTERFACE, pvm_root.sys_interface_object );

    pvm_set_field( root, PVM_ROOT_OBJECT_CLASS_LOADER, pvm_root.class_loader );

    pvm_set_field( root, PVM_ROOT_OBJECT_THREAD_LIST, pvm_root.threads_list );
    pvm_set_field( root, PVM_ROOT_OBJECT_WINDOWS_LIST, pvm_root.windows_list );
    pvm_set_field( root, PVM_ROOT_OBJECT_USERS_LIST, pvm_root.users_list );
    pvm_set_field( root, PVM_ROOT_OBJECT_KERNEL_ENVIRONMENT, pvm_root.kernel_environment );
    pvm_set_field( root, PVM_ROOT_OBJECT_OS_ENTRY, pvm_root.os_entry );

}

#define CL_DA(name) (*((struct data_area_4_class *)&(pvm_root. name##_class .data->da)))

static void pvm_create_root_objects()
{
    int root_da_size = PVM_ROOT_OBJECTS_COUNT * sizeof(struct pvm_object);
    // Allocate the very first object
    struct pvm_object_storage *root = get_root_object_storage();
    struct pvm_object_storage *roota = pvm_object_alloc( root_da_size );

    // and make sure it is really the first
    assert(root == roota);

    // TODO set class later

    root->_da_size = root_da_size;
    root->_flags = 0; // usual plain vanilla array

    ref_saturate_p(root); // no refcount!

    // Partially build interface object for internal classes

    pvm_root.sys_interface_object.data = pvm_object_alloc( N_SYS_METHODS * sizeof(struct pvm_object) );
    pvm_root.sys_interface_object.interface = pvm_root.sys_interface_object.data;
    ref_saturate_p(pvm_root.sys_interface_object.data);

    pvm_root.null_object.data = pvm_object_alloc( 0 ); // da does not exist
    pvm_root.null_object.interface = pvm_root.sys_interface_object.data;


    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        struct pvm_object_storage *curr = pvm_object_alloc( sizeof( struct data_area_4_class ) );
        ref_saturate_p(curr);
        struct data_area_4_class *da = (struct data_area_4_class *)curr->da;

        da->sys_table_id 		= i; // so simple
        da->object_flags 		= pvm_internal_classes[i].flags;
        da->object_data_area_size       = pvm_internal_classes[i].da_size;

        pvm_internal_classes[i].class_object.data           = curr;
        pvm_internal_classes[i].class_object.interface      = pvm_root.sys_interface_object.data;
    }

    set_root_from_table();

    pvm_root.null_object.data->_class = pvm_root.null_class;
    pvm_root.null_object.data->_flags = 0;

    pvm_root.sys_interface_object.data->_class = pvm_root.interface_class;
    pvm_root.sys_interface_object.data->_flags = PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE;

    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        struct pvm_object_storage *curr = pvm_internal_classes[i].class_object.data;
        struct data_area_4_class *da = (struct data_area_4_class *)curr->da;

        da->class_parent 		= pvm_root.null_class;
        da->object_default_interface    = pvm_root.sys_interface_object;

        curr->_class = pvm_root.class_class;
        curr->_flags = PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS;
    }


    // It must be ok to create strings in usual way now

    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        struct pvm_object_storage *curr = pvm_internal_classes[i].class_object.data;
        struct data_area_4_class *da = (struct data_area_4_class *)curr->da;

        da->class_name 			= pvm_create_string_object(pvm_internal_classes[i].name);
    }


    pvm_fill_syscall_interface( pvm_root.sys_interface_object, N_SYS_METHODS );


    // -------------------------------------------------------------------

    pvm_root.threads_list = pvm_create_object( pvm_get_array_class() );
    pvm_root.kernel_environment = pvm_create_object(pvm_get_array_class());
    //pvm_root.os_entry = pvm_create_null_object();
}



static void set_root_from_table()
{
#define SET_ROOT_CLASS(cn,ri) (pvm_root.cn##_class = pvm_internal_classes[pvm_iclass_by_root_index(PVM_ROOT_OBJECT_##ri##_CLASS)].class_object)


    SET_ROOT_CLASS(null,NULL);
    SET_ROOT_CLASS(class,CLASS);
    SET_ROOT_CLASS(interface,INTERFACE);
    SET_ROOT_CLASS(code,CODE);
    SET_ROOT_CLASS(int,INT);
    SET_ROOT_CLASS(string,STRING);
    SET_ROOT_CLASS(array,ARRAY);
    SET_ROOT_CLASS(page,PAGE);
    SET_ROOT_CLASS(thread,THREAD);
    SET_ROOT_CLASS(call_frame,CALL_FRAME);
    SET_ROOT_CLASS(istack,ISTACK);
    SET_ROOT_CLASS(ostack,OSTACK);
    SET_ROOT_CLASS(estack,ESTACK);
    SET_ROOT_CLASS(boot,BOOT);
    SET_ROOT_CLASS(binary,BINARY);
    SET_ROOT_CLASS(bitmap,BITMAP);
    SET_ROOT_CLASS(closure,CLOSURE);
    SET_ROOT_CLASS(world,WORLD);
}



/**
 *
 * Fast class accessors.
 *
**/

__inline__ struct pvm_object     pvm_get_null_class() { return pvm_root.null_class; }
__inline__ struct pvm_object     pvm_get_class_class() { return pvm_root.class_class; }
__inline__ struct pvm_object     pvm_get_interface_class() { return pvm_root.interface_class; }
__inline__ struct pvm_object     pvm_get_code_class() { return pvm_root.code_class; }
__inline__ struct pvm_object     pvm_get_int_class() { return pvm_root.int_class; }
__inline__ struct pvm_object     pvm_get_string_class() { return pvm_root.string_class; }
__inline__ struct pvm_object     pvm_get_array_class() { return pvm_root.array_class; }
__inline__ struct pvm_object     pvm_get_page_class() { return pvm_root.page_class; }
__inline__ struct pvm_object     pvm_get_thread_class() { return pvm_root.thread_class; }
__inline__ struct pvm_object     pvm_get_call_frame_class() { return pvm_root.call_frame_class; }
__inline__ struct pvm_object     pvm_get_istack_class() { return pvm_root.istack_class; }
__inline__ struct pvm_object     pvm_get_ostack_class() { return pvm_root.ostack_class; }
__inline__ struct pvm_object     pvm_get_estack_class() { return pvm_root.estack_class; }
__inline__ struct pvm_object     pvm_get_boot_class() { return pvm_root.boot_class; }
__inline__ struct pvm_object     pvm_get_binary_class() { return pvm_root.binary_class; }
__inline__ struct pvm_object     pvm_get_bitmap_class() { return pvm_root.bitmap_class; }
__inline__ struct pvm_object     pvm_get_closure_class() { return pvm_root.closure_class; }
__inline__ struct pvm_object     pvm_get_world_class() { return pvm_root.world_class; }


/**
 *
 * Boot code - pull in the very first user code.
 * This code runs just once in system instance's life.
 *
**/

static void pvm_boot()
{
    //struct pvm_object system_boot = pvm_object_create_fixed( pvm_get_boot_class() );
    struct pvm_object system_boot = pvm_create_object( pvm_get_boot_class() );

    struct pvm_object user_boot_class;

    if( pvm_load_class_from_module(".ru.dz.phantom.system.boot", &user_boot_class))
        pvm_exec_throw("Unable to load user boot class");

    struct pvm_object user_boot = pvm_create_object( user_boot_class );




    struct pvm_object new_cf = pvm_create_call_frame_object();
    struct data_area_4_call_frame* cfda = pvm_object_da( new_cf, call_frame );

    pvm_ostack_push( pvm_object_da(cfda->ostack, object_stack), system_boot );
    pvm_istack_push( pvm_object_da(cfda->istack, integer_stack), 1); // pass him real number of parameters

    struct pvm_object_storage *code = pvm_exec_find_method( user_boot, 8 );
    pvm_exec_set_cs( cfda, code );
    cfda->this_object = user_boot;

    struct pvm_object thread = pvm_create_thread_object( new_cf );
    gc_root_add( thread.data );

    // debug only
    //ref_saturate_p( thread.data );
    //printf("root thread 0x%X\n", thread.data );

    // GOGOGO!
    pvm_exec(thread);

    gc_root_rm( thread.data );
    ref_dec_o( thread );

}


/**
 *
 * Kernel environment access.
 * TODO: locks!
 *
 */

static int get_env_name_pos( const char *name )
{
	int items = get_array_size(pvm_root.kernel_environment.data);

	int i;
	for( i = 0; i < items; i++ )
	{

	}

	return -1;
}

#define MAX_ENV_KEY 256
void phantom_setenv( const char *name, const char *value )
{
	char ename[MAX_ENV_KEY];

	strncpy( ename, name, MAX_ENV_KEY-2 );
	ename[MAX_ENV_KEY-2] = '\0';
	strcat( ename, "=" );

	pvm_object_t s = pvm_create_string_object_binary_cat( ename, strlen(ename), value, strlen(value) );

	int pos = get_env_name_pos( name );

	if(pos < 0)
		pvm_append_array(pvm_root.kernel_environment.data,s);
	else
		pvm_set_array_ofield(pvm_root.kernel_environment.data,pos,s);
}

int phantom_getenv( const char *name, char *value, int vsize )
{
	int pos = get_env_name_pos( name );
	if( pos < 0 ) return 0;
	pvm_object_t o = pvm_get_array_ofield(pvm_root.kernel_environment.data,pos);
	if( o.data == 0 ) return 0;
	char *ed = pvm_get_str_data(o);
	int el = pvm_get_str_len(o);

	if( vsize > el ) vsize = el;

	strncpy( value, ed, vsize );

	return 1;
}


