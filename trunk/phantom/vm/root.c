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

#include <kernel/boot.h>

//#include "vm/systable_id.h"

static void pvm_create_root_objects();

static void pvm_save_root_objects();

static void set_root_from_table();

static void pvm_boot();

struct pvm_root_t pvm_root;

static void load_kernel_boot_env(void);

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
        // Not an object in the beginning of the address space?
        // We have a fresh OS instance then. Set it up from scratch.

        //root->_ah.object_start_marker = PVM_OBJECT_START_MARKER;
        pvm_alloc_clear_mem();
        pvm_create_root_objects();
        pvm_save_root_objects();

        load_kernel_boot_env();

        /* test code
        {
            char buf[512];
            phantom_getenv("root.shell", buf, 511 );
            printf("Root shell env var is '%s'\n", buf );
            phantom_getenv("root.init", buf, 511 );
            printf("Root init env var is '%s'\n", buf );
            getchar();
        }
        */

        pvm_boot();

        return;
    }

    assert(pvm_object_is_allocated( root ));

    // internal classes
    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        pvm_internal_classes[i].class_object = pvm_get_field( root,
                       pvm_internal_classes[i].root_index
                       );
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
    }

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
    unsigned int flags = 0; // usual plain vanilla array
	// make sure refcount is disabled for all objects created here: 3-rd argument of pvm_object_alloc is true (obsolete: ref_saturate_p)

    // Allocate the very first object
    struct pvm_object_storage *root = get_root_object_storage();
    struct pvm_object_storage *roota = pvm_object_alloc( root_da_size, flags, 1 );

    // and make sure it is really the first
    assert(root == roota);

    // TODO set class later

    // Partially build interface object for internal classes

    flags = PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE ;
    pvm_root.sys_interface_object.data = pvm_object_alloc( N_SYS_METHODS * sizeof(struct pvm_object), flags, 0 );
    pvm_root.sys_interface_object.interface = pvm_root.sys_interface_object.data;

    pvm_root.null_object.data = pvm_object_alloc( 0, 0, 1 ); // da does not exist
    pvm_root.null_object.interface = pvm_root.sys_interface_object.data;


    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        flags = PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS ;
        struct pvm_object_storage *curr = pvm_object_alloc( sizeof( struct data_area_4_class ), flags, 1 );
        struct data_area_4_class *da = (struct data_area_4_class *)curr->da;

        da->sys_table_id 		= i; // so simple
        da->object_flags 		= pvm_internal_classes[i].flags;
        da->object_data_area_size       = pvm_internal_classes[i].da_size;

        pvm_internal_classes[i].class_object.data           = curr;
        pvm_internal_classes[i].class_object.interface      = pvm_root.sys_interface_object.data;
    }

    set_root_from_table();

    pvm_root.null_object.data->_class = pvm_root.null_class;

    pvm_root.sys_interface_object.data->_class = pvm_root.interface_class;

    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        struct pvm_object_storage *curr = pvm_internal_classes[i].class_object.data;
        struct data_area_4_class *da = (struct data_area_4_class *)curr->da;

        da->class_parent 		= pvm_root.null_class;
        da->object_default_interface    = pvm_root.sys_interface_object;

        curr->_class = pvm_root.class_class;
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
    //pvm_root.os_entry = pvm_get_null_object();
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

//#define GCINLINE __inline__
#define GCINLINE 

GCINLINE struct pvm_object     pvm_get_null_class() { return pvm_root.null_class; }
GCINLINE struct pvm_object     pvm_get_class_class() { return pvm_root.class_class; }
GCINLINE struct pvm_object     pvm_get_interface_class() { return pvm_root.interface_class; }
GCINLINE struct pvm_object     pvm_get_code_class() { return pvm_root.code_class; }
GCINLINE struct pvm_object     pvm_get_int_class() { return pvm_root.int_class; }
GCINLINE struct pvm_object     pvm_get_string_class() { return pvm_root.string_class; }
GCINLINE struct pvm_object     pvm_get_array_class() { return pvm_root.array_class; }
GCINLINE struct pvm_object     pvm_get_page_class() { return pvm_root.page_class; }
GCINLINE struct pvm_object     pvm_get_thread_class() { return pvm_root.thread_class; }
GCINLINE struct pvm_object     pvm_get_call_frame_class() { return pvm_root.call_frame_class; }
GCINLINE struct pvm_object     pvm_get_istack_class() { return pvm_root.istack_class; }
GCINLINE struct pvm_object     pvm_get_ostack_class() { return pvm_root.ostack_class; }
GCINLINE struct pvm_object     pvm_get_estack_class() { return pvm_root.estack_class; }
GCINLINE struct pvm_object     pvm_get_boot_class() { return pvm_root.boot_class; }
GCINLINE struct pvm_object     pvm_get_binary_class() { return pvm_root.binary_class; }
GCINLINE struct pvm_object     pvm_get_bitmap_class() { return pvm_root.bitmap_class; }
GCINLINE struct pvm_object     pvm_get_closure_class() { return pvm_root.closure_class; }
GCINLINE struct pvm_object     pvm_get_world_class() { return pvm_root.world_class; }

#undef GCINLINE


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


    struct pvm_object args[1] = { system_boot };
    pvm_exec_run_method(user_boot, 8, 1, args);
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
            pvm_object_t o = pvm_get_array_ofield(pvm_root.kernel_environment.data, i);
            char *ed = pvm_get_str_data(o);
            int el = pvm_get_str_len(o);

            char *eqpos = strchr( ed, '=' );
            if( eqpos == 0 )
            {
                // Strange...
                continue;
            }
            int keylen = eqpos - ed;

            if( 0 == strncmp( ed, name, keylen ) )
            {
                return i;
            }
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
		pvm_append_array(pvm_root.kernel_environment.data, s);
	else
		pvm_set_array_ofield(pvm_root.kernel_environment.data, pos, s);
}

int phantom_getenv( const char *name, char *value, int vsize )
{
	int pos = get_env_name_pos( name );
	if( pos < 0 ) return 0;

        assert( vsize > 1 ); // trailing zero and at least one char

	pvm_object_t o = pvm_get_array_ofield(pvm_root.kernel_environment.data, pos);
	if( o.data == 0 ) return 0;
	char *ed = pvm_get_str_data(o);
	int el = pvm_get_str_len(o);

        //printf("full '%.*s'\n", el, ed );

        char *eqpos = strchr( ed, '=' );
        if( eqpos == 0 )
        {
            *value == '\0';
            return;
        }

        eqpos++; // skip '='

        el -= (eqpos - ed);

        el++; // add place for trailing zero

	if( vsize > el ) vsize = el;

        strncpy( value, eqpos, vsize );
        value[vsize-1] = '\0';

	return 1;
}



static void load_kernel_boot_env(void)
{
    // Default env first
    phantom_setenv("root.shell",".ru.dz.phantom.system.shell");
    phantom_setenv("root.init",".ru.dz.phantom.system.init");


    int i = main_envc;
    char **ep = main_env;
    while( i-- )
    {
        const char *e = *ep++;

        const char *eq = index( e, '=' );

        if( eq == 0 )
        {
            printf("Warning: env without =, '%s'\n", e);
            continue;
        }

        int nlen = eq-e;
        eq++; // skip =

        char buf[128+1];
        if( nlen > 128 ) nlen=128;

        strncpy( buf, e, nlen );

        printf("Loading env '%s'='%s'\n", buf, eq );
        phantom_setenv( buf, eq );
    }
}

