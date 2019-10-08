/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Virtual machine startup and root objects creation
 *
**/

#define DEBUG_MSG_PREFIX "vm.root"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

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
#include <kernel/debug.h>
#include <kernel/stats.h>
#include <kernel/snap_sync.h>

//#include "vm/systable_id.h"

static void pvm_create_root_objects();

static void pvm_save_root_objects();

static void set_root_from_table();

static void pvm_boot();

struct pvm_root_t pvm_root;

static void load_kernel_boot_env(void);

static void handle_object_at_restart( pvm_object_t o );

static void runclass(int, char **);

static void process_generic_restarts(pvm_object_t root);
static void process_specific_restarts(void);


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
    vm_lock_persistent_memory();

    pvm_object_t root = get_root_object_storage();

    dbg_add_command( runclass, "runclass", "runclass class [method ordinal] - create object of given class and run method (ord 8 by default)");

    if(root->_ah.object_start_marker != PVM_OBJECT_START_MARKER)
    {
        // Not an object in the beginning of the address space?
        // We have a fresh OS instance then. Set it up from scratch.

        //root->_ah.object_start_marker = PVM_OBJECT_START_MARKER;
        pvm_alloc_clear_mem();
        pvm_create_root_objects();
        pvm_save_root_objects();

        load_kernel_boot_env();

        pvm_boot();

        vm_unlock_persistent_memory(); // We return to main, rest of code assumed to not to have objects access
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
    pvm_root.restart_list = pvm_get_field( root, PVM_ROOT_OBJECT_RESTART_LIST );
    pvm_root.users_list = pvm_get_field( root, PVM_ROOT_OBJECT_USERS_LIST );
    pvm_root.class_loader = pvm_get_field( root, PVM_ROOT_OBJECT_CLASS_LOADER );
    pvm_root.kernel_environment = pvm_get_field( root, PVM_ROOT_OBJECT_KERNEL_ENVIRONMENT );
    pvm_root.os_entry = pvm_get_field( root, PVM_ROOT_OBJECT_OS_ENTRY );
    pvm_root.root_dir = pvm_get_field( root, PVM_ROOT_OBJECT_ROOT_DIR );
    pvm_root.kernel_stats = pvm_get_field( root, PVM_ROOT_KERNEL_STATISTICS );
    pvm_root.class_dir = pvm_get_field( root, PVM_ROOT_CLASS_DIR );


    process_specific_restarts();
    process_generic_restarts(root);

    vm_unlock_persistent_memory(); // We return to main, rest of code assumed to not to have objects access
}

static void process_generic_restarts(pvm_object_t root)
{
    int i;

// Must create and assign empty one and kill old one after executing
// to clear refs and delete orphans

    pvm_object_t prev_restart_list = pvm_root.restart_list;
    ref_inc_o( prev_restart_list ); // Below we set array slot, erasing ptr to prev_restart_list, which leads to ref dec to us

    pvm_root.restart_list = pvm_create_object( pvm_get_array_class() );
    pvm_set_field( root, PVM_ROOT_OBJECT_RESTART_LIST, pvm_root.restart_list );



    //cycle through restart objects here and call restart func
//#if COMPILE_EXPERIMENTAL
    int items = get_array_size(prev_restart_list);

    if( !pvm_is_null( prev_restart_list ) )
    {
        printf("Processing restart list: %d items.\n", items);
        for( i = 0; i < items; i++ )
        {
            pvm_object_t o = pvm_get_array_ofield(prev_restart_list, i);

            if(!pvm_is_null(o) )
                handle_object_at_restart(o);

            // Orphans will be killed below
        }

        printf("Done processing restart list.\n");
        ref_dec_o(prev_restart_list); // now kill it and 1-link children
    }

//#endif
}


static void start_persistent_stats(void)
{
    struct data_area_4_binary *bda = pvm_data_area( pvm_root.kernel_stats, binary );

    if( bda->data_size < STAT_CNT_PERSISTENT_DA_SIZE )
    {
        //SHOW_ERROR( 0, "persistent stats data < %d", STAT_CNT_PERSISTENT_DA_SIZE );
        printf( "persistent stats data < %ld\n", (long)STAT_CNT_PERSISTENT_DA_SIZE );
        // TODO resize object!
    }
    else
    {
        stat_set_persistent_storage( (struct persistent_kernel_stats *)bda->data );
    }
}

static void process_specific_restarts(void)
{
    start_persistent_stats();
    // One last snapshot for each run is never counted in stats,
    // 'cause counter is incremented just after it was done. Fix it here.
    {
        struct data_area_4_binary *bda = pvm_data_area( pvm_root.kernel_stats, binary );
        struct persistent_kernel_stats *ps = (struct persistent_kernel_stats *)bda->data;
        ps[STAT_CNT_SNAPSHOT].total_prev_runs++;
        // Count reboots too
        //ps[STAT_CNT_OS_REBOOTS].total_prev_runs++;
        STAT_INC_CNT( STAT_CNT_OS_REBOOTS );
    }


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
    pvm_set_field( root, PVM_ROOT_OBJECT_RESTART_LIST, pvm_root.restart_list );
    pvm_set_field( root, PVM_ROOT_OBJECT_USERS_LIST, pvm_root.users_list );
    pvm_set_field( root, PVM_ROOT_OBJECT_KERNEL_ENVIRONMENT, pvm_root.kernel_environment );
    pvm_set_field( root, PVM_ROOT_OBJECT_OS_ENTRY, pvm_root.os_entry );
    pvm_set_field( root, PVM_ROOT_OBJECT_ROOT_DIR, pvm_root.root_dir);

    pvm_set_field( root, PVM_ROOT_KERNEL_STATISTICS, pvm_root.kernel_stats );
    pvm_set_field( root, PVM_ROOT_CLASS_DIR, pvm_root.class_dir );


}

#define CL_DA(name) (*((struct data_area_4_class *)&(pvm_root. name##_class ->da)))

static void pvm_create_root_objects()
{
    int root_da_size = PVM_ROOT_OBJECTS_COUNT * sizeof(pvm_object_t );
    unsigned int flags = 0; // usual plain vanilla array
    // make sure refcount is disabled for all objects created here: 3-rd argument of pvm_object_alloc is true (obsolete: ref_saturate_p)

    // Allocate the very first object
    pvm_object_t root = get_root_object_storage();
    pvm_object_t roota = pvm_object_alloc( root_da_size, flags, 1 );

    // and make sure it is really the first
    assert(root == roota);

    // TODO set class later

    // Partially build interface object for internal classes

    flags = PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE ;
    pvm_root.sys_interface_object = pvm_object_alloc( N_SYS_METHODS * sizeof(pvm_object_t), flags, 0 );
    //pvm_root.sys_interface_object.interface = pvm_root.sys_interface_object.data;

    pvm_root.null_object = pvm_object_alloc( 0, 0, 1 ); // da does not exist
    //pvm_root.null_object.interface = pvm_root.sys_interface_object.data;


    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        flags = PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS ;
        pvm_object_t curr = pvm_object_alloc( sizeof( struct data_area_4_class ), flags, 1 );
        struct data_area_4_class *da = (struct data_area_4_class *)curr->da;

        da->sys_table_id 		= i; // so simple
        da->object_flags 		= pvm_internal_classes[i].flags;
        da->object_data_area_size       = pvm_internal_classes[i].da_size;

        pvm_internal_classes[i].class_object           = curr;
        //pvm_internal_classes[i].class_object.interface      = pvm_root.sys_interface_object.data;
    }

    set_root_from_table();

    pvm_root.null_object->_class = pvm_root.null_class;

    pvm_root.sys_interface_object->_class = pvm_root.interface_class;

    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        pvm_object_t curr = pvm_internal_classes[i].class_object;
        struct data_area_4_class *da = (struct data_area_4_class *)curr->da;

        da->class_parent 		= pvm_root.null_class;
        da->object_default_interface    = pvm_root.sys_interface_object;

        curr->_class = pvm_root.class_class;
    }


    // It must be ok to create strings in usual way now

    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        pvm_object_t curr = pvm_internal_classes[i].class_object;
        struct data_area_4_class *da = (struct data_area_4_class *)curr->da;

        da->class_name 			= pvm_create_string_object(pvm_internal_classes[i].name);
    }


    pvm_fill_syscall_interface( pvm_root.sys_interface_object, N_SYS_METHODS );


    // -------------------------------------------------------------------

    pvm_root.threads_list  = pvm_create_object( pvm_get_array_class() );
    pvm_root.kernel_environment = pvm_create_object(pvm_get_array_class());
    pvm_root.restart_list  = pvm_create_object( pvm_get_array_class() );
    pvm_root.users_list    = pvm_create_object( pvm_get_array_class() );
    pvm_root.root_dir      = pvm_create_directory_object();

    pvm_root.kernel_stats  = pvm_create_binary_object( STAT_CNT_PERSISTENT_DA_SIZE, 0 );
    pvm_root.class_dir     = pvm_create_directory_object();


    ref_saturate_o(pvm_root.threads_list); //Need it?
    ref_saturate_o(pvm_root.kernel_environment); //Need it?
    //ref_saturate_o(pvm_root.restart_list); //Need it? definitely no - this list is recreated each restart
    ref_saturate_o(pvm_root.users_list); //Need it?

    ref_saturate_o(pvm_root.kernel_stats);
    start_persistent_stats();

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
    SET_ROOT_CLASS(long,LONG);
    SET_ROOT_CLASS(float,FLOAT);
    SET_ROOT_CLASS(double,DOUBLE);
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
#if COMPILE_WEAKREF
    SET_ROOT_CLASS(weakref, WEAKREF);
#endif
    SET_ROOT_CLASS(window, WINDOW);
    SET_ROOT_CLASS(directory, DIRECTORY);
    SET_ROOT_CLASS(connection, CONNECTION);

    SET_ROOT_CLASS(mutex,MUTEX);
    SET_ROOT_CLASS(cond,COND);
    SET_ROOT_CLASS(sema,SEMA);

    SET_ROOT_CLASS(tcp,TCP);
}



/**
 *
 * Fast class accessors.
 *
**/

//#define GCINLINE __inline__
#define GCINLINE

GCINLINE pvm_object_t     pvm_get_null_class() { return pvm_root.null_class; }
GCINLINE pvm_object_t     pvm_get_class_class() { return pvm_root.class_class; }
GCINLINE pvm_object_t     pvm_get_interface_class() { return pvm_root.interface_class; }
GCINLINE pvm_object_t     pvm_get_code_class() { return pvm_root.code_class; }
GCINLINE pvm_object_t     pvm_get_int_class() { return pvm_root.int_class; }
GCINLINE pvm_object_t     pvm_get_long_class() { return pvm_root.long_class; }
GCINLINE pvm_object_t     pvm_get_float_class() { return pvm_root.float_class; }
GCINLINE pvm_object_t     pvm_get_double_class() { return pvm_root.double_class; }
GCINLINE pvm_object_t     pvm_get_string_class() { return pvm_root.string_class; }
GCINLINE pvm_object_t     pvm_get_array_class() { return pvm_root.array_class; }
GCINLINE pvm_object_t     pvm_get_page_class() { return pvm_root.page_class; }
GCINLINE pvm_object_t     pvm_get_thread_class() { return pvm_root.thread_class; }
GCINLINE pvm_object_t     pvm_get_call_frame_class() { return pvm_root.call_frame_class; }
GCINLINE pvm_object_t     pvm_get_istack_class() { return pvm_root.istack_class; }
GCINLINE pvm_object_t     pvm_get_ostack_class() { return pvm_root.ostack_class; }
GCINLINE pvm_object_t     pvm_get_estack_class() { return pvm_root.estack_class; }
GCINLINE pvm_object_t     pvm_get_boot_class() { return pvm_root.boot_class; }
GCINLINE pvm_object_t     pvm_get_binary_class() { return pvm_root.binary_class; }
GCINLINE pvm_object_t     pvm_get_bitmap_class() { return pvm_root.bitmap_class; }
GCINLINE pvm_object_t     pvm_get_closure_class() { return pvm_root.closure_class; }
GCINLINE pvm_object_t     pvm_get_world_class() { return pvm_root.world_class; }
#if COMPILE_WEAKREF
GCINLINE pvm_object_t     pvm_get_weakref_class() { return pvm_root.weakref_class; }
#endif
GCINLINE pvm_object_t     pvm_get_window_class() { return pvm_root.window_class; }
GCINLINE pvm_object_t     pvm_get_directory_class() { return pvm_root.directory_class; }
GCINLINE pvm_object_t     pvm_get_connection_class() { return pvm_root.connection_class; }

GCINLINE pvm_object_t     pvm_get_mutex_class() { return pvm_root.mutex_class; }
GCINLINE pvm_object_t     pvm_get_cond_class() { return pvm_root.cond_class; }
GCINLINE pvm_object_t     pvm_get_sema_class() { return pvm_root.sema_class; }

GCINLINE pvm_object_t     pvm_get_tcp_class() { return pvm_root.sema_class; }


#undef GCINLINE


/**
 *
 * Boot code - pull in the very first user code.
 * This code runs just once in system instance's life.
 *
**/

static void pvm_boot()
{
    //pvm_object_t system_boot = pvm_object_create_fixed( pvm_get_boot_class() );
    pvm_object_t system_boot = pvm_create_object( pvm_get_boot_class() );

    pvm_object_t user_boot_class;

    char boot_class[128];
    if( !phantom_getenv("root.boot", boot_class, sizeof(boot_class) ) )
        strcpy( boot_class, ".ru.dz.phantom.system.boot" );

    if( pvm_load_class_from_module(boot_class, &user_boot_class))
    {
        printf("Unable to load boot class '%s'", boot_class );
        pvm_exec_panic0("Unable to load user boot class");
    }

    pvm_object_t user_boot = pvm_create_object( user_boot_class );


    pvm_object_t args[1] = { system_boot };
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
    int items = get_array_size(pvm_root.kernel_environment);

    int i;
    for( i = 0; i < items; i++ )
    {
        pvm_object_t o = pvm_get_array_ofield(pvm_root.kernel_environment, i);
        char *ed = pvm_get_str_data(o);
        //int el = pvm_get_str_len(o);

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
        pvm_append_array(pvm_root.kernel_environment, s);
    else
        pvm_set_array_ofield(pvm_root.kernel_environment, pos, s);
}

int phantom_getenv( const char *name, char *value, int vsize )
{
    int pos = get_env_name_pos( name );
    if( pos < 0 ) return 0;

    assert( vsize > 1 ); // trailing zero and at least one char

    pvm_object_t o = pvm_get_array_ofield(pvm_root.kernel_environment, pos);
    if( o == 0 ) return 0;
    char *ed = pvm_get_str_data(o);
    int el = pvm_get_str_len(o);

    //printf("full '%.*s'\n", el, ed );

    char *eqpos = strchr( ed, '=' );
    if( eqpos == 0 )
    {
        *value = '\0';
        return 0;
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
    phantom_setenv("root.boot",".ru.dz.phantom.system.boot");


    int i = main_envc;
    const char **ep = main_env;
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
        buf[nlen] = '\0';

        printf("Loading env '%s'='%s'\n", buf, eq );
        phantom_setenv( buf, eq );
    }
}


static o_restart_func_t find_restart_f( pvm_object_t _class )
{
    if(!(_class->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS))
        pvm_exec_panic0( "find_restart_f: not a class object" );

    struct data_area_4_class *da = (struct data_area_4_class *)&(_class->da);

    assert( da->sys_table_id < pvm_n_internal_classes );

    // TODO fix this back
    //if( syscall_index >= da->class_sys_table_size )                        pvm_exec_panic("find_syscall: syscall_index no out of table size" );

    return pvm_internal_classes[da->sys_table_id].restart;
}


static void handle_object_at_restart( pvm_object_t o )
{
//#if COMPILE_EXPERIMENTAL
    printf( "restart 0x%p\n", o );

    if(!(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL))
    {
        printf( "not internal object in restart list!\n" );
        return;
    }
    o_restart_func_t rf = find_restart_f( o->_class );

    if(rf) rf(o);
//#endif
}

// TODO interlock add and delete?
// TODO with interlock we can compact list too
void pvm_add_object_to_restart_list( pvm_object_t o )
{
    // TODO must cleanup inc/dec convention
    ref_inc_o( o );
    pvm_append_array( pvm_root.restart_list, o );
}

void pvm_remove_object_from_restart_list( pvm_object_t o )
{
    int i;
    //printf("!! unimpl pvm_remove_object_from_restart_list called !!\n");

    pvm_object_t curr_restart_list = pvm_root.restart_list;

    //cycle through restart objects

    int items = get_array_size(curr_restart_list);

    if( !pvm_is_null( curr_restart_list ) )
    {
        //printf("Processing restart list: %d items.\n", items);
        for( i = 0; i < items; i++ )
        {
            pvm_object_t o1 = pvm_get_array_ofield(curr_restart_list, i);

            if( pvm_is_eq(o,o1) )
            {
                pvm_set_array_ofield(curr_restart_list, i, pvm_create_null_object() );
                break;
            }
        }

        //printf("Done processing restart list.\n");
    }
}


void create_and_run_object(const char *class_name, int method )
{
    if( method < 0 )
        method = 8;

    pvm_object_t name = pvm_create_string_object(class_name);

    pvm_object_t user_class = pvm_exec_lookup_class_by_name(name);


    if( pvm_is_null(user_class))
    {
        //SHOW_ERROR( 0, "Unable to load class '%s'", class_name );
        printf( "Unable to load class '%s'\n", class_name );
        return;
    }

    pvm_object_t user_o = pvm_create_object( user_class );
    ref_dec_o(user_class);

    pvm_exec_run_method(user_o, method, 0, 0);
    ref_dec_o(user_o);
}


static void runclass(int ac, char **av)
{
    int method = 8;

    if( ac < 1 || ac > 2 )
    {
        printf("runclass class_name [method ordinal]\n");
        return;
    }

    const char *cname = *av++;

    if( ac > 1 )
    {
        method = atol( *av++ );
    }

    create_and_run_object(cname, method );
}





int pvm_connect_object(pvm_object_t o, struct data_area_4_thread *tc)
{
    struct data_area_4_connection *da = pvm_object_da( o, connection );

    // must be done in phantom_connect_object
    pvm_add_object_to_restart_list( o ); // TODO must check it's there

    return phantom_connect_object( da, tc);
}

int pvm_disconnect_object(pvm_object_t o, struct data_area_4_thread *tc)
{
    struct data_area_4_connection *da = pvm_object_da( o, connection );

    pvm_remove_object_from_restart_list( o );

    //return phantom_disconnect_object( da, tc);
    return phantom_disconnect_object( da );
}




