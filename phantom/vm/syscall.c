/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal (native) classes implementation.
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <stdlib.h>

#include <phantom_libc.h>
#include <time.h>
#include <threads.h>

#include <kernel/snap_sync.h>
#include <kernel/vm.h>
#include <kernel/atomic.h>
#include <kernel/init.h> // cpu reset

#include <vm/syscall.h>
#include <vm/object.h>
#include <vm/object_flags.h>
#include <vm/root.h>
#include <vm/exec.h>
#include <vm/bulk.h>
#include <vm/alloc.h>
#include <vm/internal.h>

#include <vm/p2c.h>

#include <vm/wrappers.h>

#include <hashfunc.h>

#include <video/screen.h>
#include <video/vops.h>
#include <video/internal.h>

//extern syscall_func_t  pvm_exec_systables[PVM_SYSTABLE_SIZE][32];


static int debug_print = 0;


//static pvm_object_t root_os_interface_object;

//
//	Default syscalls.
//
//	Any class with internal implementation will present at least:
//
//	sys 0:	Construct. No args.
//
//	sys 1:	Destruct. No args.
//
//	sys 2:	GetClass. No args. Returns class object.
//
//	sys 3:	clone. No args. Returns copy of this, if possible.
//
//	sys 4:	equals. arg is object. Compares by value.
//
//	sys 5:	ToString. No args, returns string object, representing
//			contents of this object.
//
//	sys 6:  ToXML. Returns string object, representing
//			contents of this object in XML form.
//
//	sys 7:	fromXML.
//
//	sys 8:	default activity. depends on class
//
//	sys 9:	secondary activity, depends on class
//
//	sys 10:	third activity
//
//	sys 11:	fourth activity
//
//	sys 15:	int hashCode - returns int
//






// --------- invalid method stub --------------------------------------------

int invalid_syscall( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
printf("invalid syscal for object: "); dumpo( (addr_t)(o) );//pvm_object_print( o ); printf("\n");
//printf("invalid value's class: "); pvm_object_print( o->_class); printf("\n");
    SYSCALL_THROW_STRING( "invalid syscall called" );
}


// --------- void aka object ------------------------------------------------

int si_void_0_construct( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;

    DEBUG_INFO;
    SYSCALL_RETURN_NOTHING;
}

int si_void_1_destruct( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    //(void)tc;

    DEBUG_INFO;
    SYSCALL_RETURN_NOTHING;
}

int si_void_2_class(pvm_object_t this_obj, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    //ref_inc_o( this_obj->_class );  //increment if class is refcounted
    SYSCALL_RETURN(this_obj->_class);
}

int si_void_3_clone( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_THROW_STRING( "void clone called" );
}

int si_void_4_equals( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;

    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];

    int iret = (me == him);

    SYS_FREE_O(him);

    SYSCALL_RETURN(pvm_create_int_object( iret ) );
}

int si_void_5_tostring( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(void)" ));
}

int si_void_6_toXML( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "<void>" ));
}

int si_void_7_fromXML( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(void)" ));
}
/*
int si_void_8_def_op_1( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_THROW_STRING( "void default op 1 called" );
}
*/
int si_void_14_to_immutable( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;

    
    CHECK_PARAM_COUNT(0);

    o->_flags |= PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE;

    SYSCALL_RETURN(pvm_create_int_object( 0 ) );
}


int si_void_15_hashcode( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    size_t os = me->_da_size;
    void *oa = me->da;

    //SYSCALL_RETURN(pvm_create_int_object( ((addr_t)me)^0x3685A634^((addr_t)&si_void_15_hashcode) ));
    SYSCALL_RETURN(pvm_create_int_object( calc_hash( oa, oa+os) ));
}



syscall_func_t	syscall_table_4_void[16] =
//pvm_exec_systables[PVM_SYSTABLE_ID_NULL][] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_void_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &si_void_14_to_immutable,        &si_void_15_hashcode

};
DECLARE_SIZE(void);





// --------- int ------------------------------------------------------------

static int si_int_3_clone( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_int_object( pvm_get_int(me) ));
}

static int si_int_4_equals( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];

    int same_class = me->_class == him->_class;
    int same_value = pvm_get_int(me) == pvm_get_int(him);

    SYS_FREE_O(him);

    SYSCALL_RETURN(pvm_create_int_object( same_class && same_value));
}

static int si_int_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    char buf[32];
    snprintf( buf, sizeof(buf), "%d", pvm_get_int(me) );
    SYSCALL_RETURN(pvm_create_string_object( buf ));
}

static int si_int_6_toXML( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    char buf[32];
    snprintf( buf, 31, "%d", pvm_get_int(me) );
	//SYSCALL_RETURN(pvm_create_string_object( "<void>" ));
    SYSCALL_THROW_STRING( "int toXML called" );
}


syscall_func_t	syscall_table_4_int[16] =
{
    &si_void_0_construct,           	&si_void_1_destruct,
    &si_void_2_class,               	&si_int_3_clone,
    &si_int_4_equals,                   &si_int_5_tostring,
    &si_int_6_toXML,                    &si_void_7_fromXML,
    // 8
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&invalid_syscall,
    &si_void_14_to_immutable,           &si_void_15_hashcode
};
//int	n_syscall_table_4_int =	(sizeof syscall_table_4_int) / sizeof(syscall_func_t);
DECLARE_SIZE(int);










// --------- long ------------------------------------------------------------

static int si_long_3_clone( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_long_object( pvm_get_long(me) ));
}

static int si_long_4_equals( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);

    pvm_object_t him = args[0];

    int same_class = me->_class == him->_class;
    int same_value = pvm_get_long(me) == pvm_get_long(him);

    SYS_FREE_O(him);

    SYSCALL_RETURN(pvm_create_int_object( same_class && same_value));
}

static int si_long_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    char buf[100];
    snprintf( buf, sizeof(buf), "%Ld", pvm_get_long(me) ); // TODO right size?
    SYSCALL_RETURN(pvm_create_string_object( buf ));
}

static int si_long_6_toXML( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    char buf[100];
    snprintf( buf, sizeof(buf), "%Ld", pvm_get_long(me) );
	//SYSCALL_RETURN(pvm_create_string_object( "<void>" ));
    SYSCALL_THROW_STRING( "int toXML called" );
}


syscall_func_t	syscall_table_4_long[16] =
{
    &si_void_0_construct,       &si_void_1_destruct,
    &si_void_2_class,           &si_long_3_clone,
    &si_long_4_equals,     	    &si_long_5_tostring,
    &si_long_6_toXML,      	    &si_void_7_fromXML,
    // 8
    &invalid_syscall,           &invalid_syscall,
    &invalid_syscall,           &invalid_syscall,
    &invalid_syscall,           &invalid_syscall,
    &si_void_14_to_immutable,   &si_void_15_hashcode
};
DECLARE_SIZE(long);




// --------- float ------------------------------------------------------------

static int si_float_3_clone( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_float_object( pvm_get_float(me) ));
}

static int si_float_4_equals( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];

    int same_class = me->_class == him->_class;
    int same_value = pvm_get_float(me) == pvm_get_float(him);

    SYS_FREE_O(him);

    SYSCALL_RETURN(pvm_create_int_object( same_class && same_value));
}

static int si_float_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    char buf[100];
    snprintf( buf, sizeof(buf), "%f", pvm_get_float(me) ); // TODO right size?
    SYSCALL_RETURN(pvm_create_string_object( buf ));
}

static int si_float_6_toXML( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    char buf[100];
    snprintf( buf, sizeof(buf), "<float>%f</float>", pvm_get_float(me) );
	SYSCALL_RETURN(pvm_create_string_object( buf ));
    //SYSCALL_THROW_STRING( "int toXML called" );
}


syscall_func_t	syscall_table_4_float[16] =
{
    &si_void_0_construct,       &si_void_1_destruct,
    &si_void_2_class,           &si_float_3_clone,
    &si_float_4_equals,         &si_float_5_tostring,
    &si_float_6_toXML,          &si_void_7_fromXML,
    // 8
    &invalid_syscall,           &invalid_syscall,
    &invalid_syscall,           &invalid_syscall,
    &invalid_syscall,           &invalid_syscall,
    &si_void_14_to_immutable,   &si_void_15_hashcode
};
DECLARE_SIZE(float);





// --------- double ------------------------------------------------------------

static int si_double_3_clone( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_double_object( pvm_get_double(me) ));
}

static int si_double_4_equals( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];

    int same_class = me->_class == him->_class;
    int same_value = pvm_get_double(me) == pvm_get_double(him);

    SYS_FREE_O(him);

    SYSCALL_RETURN(pvm_create_int_object( same_class && same_value));
}

static int si_double_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    char buf[100];
    snprintf( buf, sizeof(buf), "%f", pvm_get_double(me) ); // TODO right size?
    SYSCALL_RETURN(pvm_create_string_object( buf ));
}

static int si_double_6_toXML( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    char buf[100];
    snprintf( buf, sizeof(buf), "<double>%f</double>", pvm_get_double(me) );
	SYSCALL_RETURN(pvm_create_string_object( buf ));
    //SYSCALL_THROW_STRING( "int toXML called" );
}


syscall_func_t	syscall_table_4_double[16] =
{
    &si_void_0_construct,       &si_void_1_destruct,
    &si_void_2_class,           &si_double_3_clone,
    &si_double_4_equals,        &si_double_5_tostring,
    &si_double_6_toXML,         &si_void_7_fromXML,
    // 8
    &invalid_syscall,           &invalid_syscall,
    &invalid_syscall,           &invalid_syscall,
    &invalid_syscall,           &invalid_syscall,
    &si_void_14_to_immutable,   &si_void_15_hashcode
};
DECLARE_SIZE(double);

















// --------- string ---------------------------------------------------------

static int si_string_3_clone( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_string *meda = pvm_object_da( me, string );
    SYSCALL_RETURN(pvm_create_string_object_binary( (char *)meda->data, meda->length ));
}

static int si_string_4_equals( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];

    int iret = 0;
    if( !pvm_is_null(him) )
    {
        ASSERT_STRING(him);

        struct data_area_4_string *meda = pvm_object_da( me, string );
        struct data_area_4_string *himda = pvm_object_da( him, string );

        iret =
            me->_class == him->_class &&
            meda->length == himda->length &&
            0 == strncmp( (const char*)meda->data, (const char*)himda->data, meda->length )
            ;
    }
    SYS_FREE_O(him);

    // BUG - can compare just same classes
    SYSCALL_RETURN(pvm_create_int_object( iret ) );
}

static int si_string_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    SYSCALL_RETURN(me);
}

static int si_string_8_substring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_string *meda = pvm_object_da( me, string );
    
    CHECK_PARAM_COUNT(2);

    int parmlen = AS_INT(args[1]);
    int index = AS_INT(args[0]);


    if( index < 0 || index >= meda->length )
        SYSCALL_THROW_STRING( "string.substring index is out of bounds" );

    int len = meda->length - index;
    if( parmlen < len ) len = parmlen;

    if( len < 0 )
        SYSCALL_THROW_STRING( "string.substring length is negative" );


    //printf("substr inx %x len %d parmlen %d\n", index, len, parmlen);

    SYSCALL_RETURN(pvm_create_string_object_binary( (char *)meda->data + index, len ));
}

static int si_string_9_charat( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_string *meda = pvm_object_da( me, string );
    
    CHECK_PARAM_COUNT(1);
    int index = AS_INT(args[0]);

    int len = meda->length;

    if(index > len-1 )
        SYSCALL_THROW_STRING( "string.charAt index is out of bounds" );

    SYSCALL_RETURN(pvm_create_int_object( meda->data[index]  ));
}


static int si_string_10_concat( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];
    ASSERT_STRING(him);

    struct data_area_4_string *meda = pvm_object_da( me, string );
    struct data_area_4_string *himda = pvm_object_da( him, string );

    pvm_object_t oret = pvm_create_string_object_binary_cat(
    		(char *)meda->data, meda->length,
                (char *)himda->data, himda->length );

    SYS_FREE_O(him);

    SYSCALL_RETURN( oret );
}


static int si_string_11_length( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_string *meda = pvm_object_da( me, string );

    
    CHECK_PARAM_COUNT(0);

    SYSCALL_RETURN(pvm_create_int_object( meda->length ));
}

static int si_string_12_find( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];
    ASSERT_STRING(him);

    struct data_area_4_string *meda = pvm_object_da( me, string );
    struct data_area_4_string *himda = pvm_object_da( him, string );

    unsigned char * cret = (unsigned char *)strnstrn(
    		(char *)meda->data, meda->length,
                (char *)himda->data, himda->length );

    SYS_FREE_O(him);

    int pos = -1;

    if( cret != 0 )
        pos = cret - (meda->data);

    SYSCALL_RETURN(pvm_create_int_object( pos ));
}


static int si_string_16_toint( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(0);

    struct data_area_4_string *meda = pvm_object_da( me, string );

    SYSCALL_RETURN(pvm_create_int_object( atoin( (char *)meda->data, meda->length ) ) );
}


static int si_string_17_tolong( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(0);

    struct data_area_4_string *meda = pvm_object_da( me, string );

    SYSCALL_RETURN(pvm_create_long_object( atoln( (char *)meda->data, meda->length ) ) );
}



syscall_func_t	syscall_table_4_string[18] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_string_3_clone,
    &si_string_4_equals,            &si_string_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_string_8_substring, 		&si_string_9_charat,
    &si_string_10_concat,           &si_string_11_length,
    &si_string_12_find,             &invalid_syscall,
    &si_void_14_to_immutable,       &si_void_15_hashcode,
    // 16
    &si_string_16_toint,            &si_string_17_tolong,
    //&si_string_18_tofloat,          &si_string_19_todouble,
};
DECLARE_SIZE(string);





// --------- thread ---------------------------------------------------------

static int si_thread_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "thread" ));
}

/*
static int si_thread_8_start( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    //phantom_activate_thread(me);
    //SYSCALL_RETURN(pvm_create_string_object( "thread" ));
    SYSCALL_RETURN_NOTHING;
}
*/

static int si_thread_10_pause( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_thread *meda = pvm_object_da( me, thread );

    if(meda != tc)
    	SYSCALL_THROW_STRING("Thread can pause itself only");

//#if OLD_VM_SLEEP
//    SYSCALL_PUT_THIS_THREAD_ASLEEP(0);
//#else
    SYSCALL_THROW_STRING("Not this way");
//#endif

    SYSCALL_RETURN_NOTHING;
}

static int si_thread_11_continue( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
//#if OLD_VM_SLEEP
//    struct data_area_4_thread *meda = pvm_object_da( me, thread );

    //hal_spin_lock(&meda->spin);
//    if( !meda->sleep_flag )
//    	SYSCALL_THROW_STRING("Thread is not sleeping in continue");
    //hal_spin_unlock(&meda->spin);

//    SYSCALL_WAKE_THREAD_UP(meda);
//#else
    SYSCALL_THROW_STRING("Not this way");
//#endif

    SYSCALL_RETURN_NOTHING;
}

static int si_thread_14_getOsInterface( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    pvm_object_t root = get_root_object_storage();
    pvm_object_t o = pvm_get_field( root, PVM_ROOT_OBJECT_OS_ENTRY );
    SYSCALL_RETURN( ref_inc_o( o ) );
}

static int si_thread_13_getUser( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    struct data_area_4_thread *meda = pvm_object_da( me, thread );

    SYSCALL_RETURN(meda->owner);
}


static int si_thread_12_getEnvironment( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    struct data_area_4_thread *meda = pvm_object_da( me, thread );

    if( pvm_is_null(meda->environment) )
    {
        pvm_object_t env = pvm_create_string_object(".phantom.environment");
        pvm_object_t cl = pvm_exec_lookup_class_by_name( env );
        meda->environment = pvm_create_object(cl);
        ref_dec_o(env);
        //ref_dec_o(cl);  // object keep class ref
    }

    SYSCALL_RETURN(meda->environment);
}


syscall_func_t	syscall_table_4_thread[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_thread_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall,               &invalid_syscall,
    &si_thread_10_pause,            &si_thread_11_continue,
    &si_thread_12_getEnvironment,   &si_thread_13_getUser,
    &si_thread_14_getOsInterface,   &si_void_15_hashcode
};

DECLARE_SIZE(thread);



// --------- call frame -----------------------------------------------------

static int si_call_frame_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "call_frame" ));
}


syscall_func_t	syscall_table_4_call_frame[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_call_frame_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
};

DECLARE_SIZE(call_frame);


// --------- istack ---------------------------------------------------------

static int si_istack_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "istack" ));
}


syscall_func_t	syscall_table_4_istack[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_istack_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
};

DECLARE_SIZE(istack);


// --------- ostack ---------------------------------------------------------

static int si_ostack_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "ostack" ));
}


syscall_func_t	syscall_table_4_ostack[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_ostack_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
};

DECLARE_SIZE(ostack);


// --------- estack ---------------------------------------------------------

static int si_estack_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "estack" ));
}


syscall_func_t	syscall_table_4_estack[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_estack_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
};

DECLARE_SIZE(estack);

// --------- class class ---------------------------------------------------------

static int si_class_class_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "class" ));
}

static int si_class_class_8_new_class( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;

    CHECK_PARAM_COUNT(3);

    pvm_object_t class_name = args[0];
    int n_object_slots = AS_INT(args[0]);
    pvm_object_t iface = args[0];

    ASSERT_STRING(class_name);

    pvm_object_t new_class = pvm_create_class_object(class_name, iface, sizeof(pvm_object_t ) * n_object_slots);

    //SYS_FREE_O(class_name);  //linked in class object
    //SYS_FREE_O(iface);  //linked in class object

    SYSCALL_RETURN( new_class );
}

static int si_class_10_set_static( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_class *meda = pvm_object_da( me, class );

    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(2);

    pvm_object_t static_val = args[1];
    int n_slot = AS_INT(args[0]);

    pvm_set_ofield( meda->static_vars, n_slot, static_val );

    SYSCALL_RETURN_NOTHING;
}

static int si_class_11_get_static( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_class *meda = pvm_object_da( me, class );
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);

    int n_slot = AS_INT(args[0]);

    pvm_object_t iret = pvm_get_ofield( meda->static_vars, n_slot );
    ref_inc_o( iret );
    SYSCALL_RETURN( iret );
}


// Check if given object is instance of this class
static int si_class_14_instanceof( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    //struct data_area_4_class *meda = pvm_object_da( me, class );
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);

    pvm_object_t instance = args[0];

#if VM_INSTOF_RECURSIVE
    int is = pvm_object_class_is_or_child( instance, me );
#else
    int is = pvm_object_class_exactly_is( instance, me );
#endif // VM_INSTOF_RECURSIVE
    SYS_FREE_O(instance);

    SYSCALL_RETURN(pvm_create_int_object( is ));
}


syscall_func_t	syscall_table_4_class[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_class_class_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_class_class_8_new_class,    &invalid_syscall,
    &si_class_10_set_static,        &si_class_11_get_static,
    &invalid_syscall,               &invalid_syscall,
    &si_class_14_instanceof,        &si_void_15_hashcode
};

DECLARE_SIZE(class);


// --------- interface ---------------------------------------------------------

static int si_interface_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "interface" ));
}


syscall_func_t	syscall_table_4_interface[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_interface_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
};

DECLARE_SIZE(interface);


// --------- code ---------------------------------------------------------

static int si_code_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "code" ));
}


syscall_func_t	syscall_table_4_code[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_code_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
};

DECLARE_SIZE(code);


// --------- page ---------------------------------------------------------

static int si_page_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "page" ));
}


syscall_func_t	syscall_table_4_page[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_page_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
};

DECLARE_SIZE(page);


// ---------------------------------------------------------------------------
// --------- bootstrap -------------------------------------------------------
// ---------------------------------------------------------------------------

#define CACHED_CLASSES 1
#define DEBUG_CACHED_CLASSES 0


/**
 *
 * \brief Lookup class by name in persistent class cache.
 *
 * \param[in]  name       Name of class to lookup
 * \param[in]  name_len   Length of name
 * \param[out] new_class  Class found
 * 
 * \return 0 if found, ENOENT if not found
 *
 * Stores directory in pvm_root.class_dir
 *
**/


errno_t pvm_class_cache_lookup(const char *name, int name_len, pvm_object_t *new_class)
{
#if !CACHED_CLASSES
    return ENOENT;
#else
    if(DEBUG_CACHED_CLASSES) printf("---- pvm_class_cache_lookup %.*s\n", name_len, name );
    //if( pvm_is_null(dir) )        dir = pvm_create_directory_object();

    struct data_area_4_directory *da = pvm_object_da( pvm_root.class_dir, directory );

    errno_t rc = hdir_find( da, name, name_len, new_class, 0 );

    if( DEBUG_CACHED_CLASSES && (rc == 0) )
    {
        printf("---- pvm_class_cache_lookup %.*s FOUND\n", name_len, name );
        pvm_object_dump( *new_class );
    }
    return rc;
#endif
}

/**
 *
 * \brief Add class to persistent class cache.
 *
 * \param[in]  name       Name of class
 * \param[in]  name_len   Length of name
 * \param[out] new_class  Class object
 * 
 * \return 0 if ok, nonzero (errno) on error.
 *
 * Stores directory in pvm_root.class_dir
 *
**/


errno_t pvm_class_cache_insert(const char *name, int name_len, pvm_object_t new_class)
{
#if !CACHED_CLASSES
    return 0;
#else
    if(DEBUG_CACHED_CLASSES) printf("---- pvm_class_cache_insert %.*s\n", name_len, name );
    struct data_area_4_directory *da = pvm_object_da( pvm_root.class_dir, directory );
    errno_t rc = hdir_add( da, name, name_len, new_class );
    return rc;
#endif
}


static int si_bootstrap_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "bootstrap" ));
}


static int si_bootstrap_8_load_class( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;

    
    CHECK_PARAM_COUNT(1);

    const int bufs = 1024;
    char buf[bufs+1];
    int len;

    {
    pvm_object_t name = args[0];
    ASSERT_STRING(name);

    struct data_area_4_string *nameda = pvm_object_da( name, string );


    len = nameda->length > bufs ? bufs : nameda->length;
    memcpy( buf, nameda->data, len );
    buf[len] = '\0';

    SYS_FREE_O(name);
    }

    // BUG! Need some diagnostics from loader here

    pvm_object_t new_class;

    if( 0 == pvm_class_cache_lookup(buf, len, &new_class) )
    {
        printf("got from cache class '%.*s' @%p\n", len, buf, new_class );
        ref_inc_o(new_class);
    	SYSCALL_RETURN(new_class);
    }

    if( pvm_load_class_from_module(buf, &new_class))
    {
        const char *msg = " - class load error";
        if( strlen(buf) >= bufs - 2 - strlen(msg) )
        {
            SYSCALL_THROW_STRING( msg+3 );
        }
        else
        {
            strcat( buf, msg );
            SYSCALL_THROW_STRING( buf );
        }
    }
    else
    {
        pvm_class_cache_insert(buf, len, new_class);
    	SYSCALL_RETURN(new_class);
    }
}

#if 0
static int si_bootstrap_9_load_code( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;

    
    CHECK_PARAM_COUNT(1);

    pvm_object_t name = args[0];
    ASSERT_STRING(name);

#if 0
    const int bufs = 1024;
    char buf[bufs+1];


    struct data_area_4_string *nameda = pvm_object_da( name, string );

    int len = nameda->length > bufs ? bufs : nameda->length;
    memcpy( buf, nameda->data, len );
    buf[len] = '\0';
    SYS_FREE_O(name);

    code_seg cs = load_code(buf);

    SYSCALL_RETURN(pvm_object_storage::create_code( cs.get_code_size(), cs.get_code() ));
#else
    SYS_FREE_O(name);
    SYSCALL_THROW_STRING( "load code is deprecated" );
#endif
}
#endif

static int si_bootstrap_16_print( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;

    while( n_args-- )
        {
    	pvm_object_t o = args[n_args];
        pvm_object_print( o );
        SYS_FREE_O( o );
        }

    SYSCALL_RETURN_NOTHING;
}

static int si_bootstrap_17_register_class_loader( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);

    pvm_object_t loader = args[0];

    pvm_root.class_loader = loader;
    pvm_object_storage_t *root = get_root_object_storage();
    pvm_set_field( root, PVM_ROOT_OBJECT_CLASS_LOADER, pvm_root.class_loader );

    // Don't need do SYS_FREE_O(loader) since we store it

    SYSCALL_RETURN_NOTHING;
}


static int si_bootstrap_18_thread( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);

    pvm_object_t object = args[0];

    // Don't need do SYS_FREE_O(object) since we store it as 'this'

#if 1
    // TODO check object class to be runnable or subclass

    {
    pvm_object_t new_cf = pvm_create_call_frame_object();
    struct data_area_4_call_frame* cfda = pvm_object_da( new_cf, call_frame );

    pvm_ostack_push( pvm_object_da(cfda->ostack, object_stack), me );
    pvm_istack_push( pvm_object_da(cfda->istack, integer_stack), 1); // pass him real number of parameters

    pvm_object_t code = pvm_exec_find_method( object, 8, tc );
    pvm_exec_set_cs( cfda, code );
    cfda->this_object = object;

    pvm_object_t thread = pvm_create_thread_object( new_cf );

    //printf("here?\n");

    phantom_activate_thread(thread);
    }
#endif


    SYSCALL_RETURN_NOTHING;
}


// THIS IS JUST A TEMP SHORTCUT!
static int si_bootstrap_19_create_binary( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);

    int nbytes = AS_INT(args[0]);

    SYSCALL_RETURN( pvm_create_binary_object(nbytes, NULL) );
}

#define BACK_WIN 1

#if BACK_WIN && !VIDEO_NEW_BG_WIN
static window_handle_t back_win = 0;
#endif

static int si_bootstrap_20_set_screen_background( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);

    pvm_object_t _bmp = args[0];

#if !BACK_WIN
    if( drv_video_bmpblt(_bmp,0,0,0) )
    	SYSCALL_THROW_STRING( "not a bitmap" );


    drv_video_window_repaint_all();
#else
    // TODO black screen :(

    if( !pvm_object_class_exactly_is( _bmp, pvm_get_bitmap_class() ) )
    	SYSCALL_THROW_STRING( "not a bitmap" );

    struct data_area_4_bitmap *bmp = pvm_object_da( _bmp, bitmap );
    struct data_area_4_binary *bin = pvm_object_da( bmp->image, binary );


#if !VIDEO_NEW_BG_WIN
    if(back_win == 0)
    	back_win = drv_video_window_create( scr_get_xsize(), scr_get_ysize(), 0, 0, COLOR_BLACK, "Background", WFLAG_WIN_DECORATED );

    back_win->flags &= ~WFLAG_WIN_DECORATED;
    back_win->flags |= WFLAG_WIN_NOFOCUS;

    w_to_bottom(back_win);

    //drv_video_bitblt( (void *)bin->data, 0, 0, bmp->xsize, bmp->ysize, (zbuf_t)zpos );
#else
    window_handle_t back_win = w_get_bg_window();
#endif

    bitmap2bitmap(
    		back_win->w_pixel, back_win->xsize, back_win->ysize, 0, 0,
                     (void *)bin->data, bmp->xsize, bmp->ysize, 0, 0,
                     bmp->xsize, bmp->ysize
                    );

    //drv_video_winblt(back_win);
    w_update( back_win );
    //scr_repaint_all();
#endif
    // Remove it if will store bmp!
    SYS_FREE_O(_bmp);

    SYSCALL_RETURN_NOTHING;
}

static int si_bootstrap_21_sleep( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;

    
    CHECK_PARAM_COUNT(1);

    int msec = AS_INT(args[0]);
//#if OLD_VM_SLEEP
//    phantom_wakeup_after_msec(msec,tc);

    //#warning to kill
//    SHOW_ERROR0( 0, "si_bootstrap_21_sleep used" );

//    if(phantom_is_a_real_kernel())
//        SYSCALL_PUT_THIS_THREAD_ASLEEP(0);
//#else
    (void) msec;
    SHOW_ERROR0( 0, "si_bootstrap_21_sleep used" );
//#endif
    SYSCALL_RETURN_NOTHING;
}

static int si_bootstrap_22_set_os_interface( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;

    CHECK_PARAM_COUNT(1);

    pvm_root.os_entry = args[0];

    ref_saturate_o(pvm_root.os_entry); // make sure refcount is disabled for this object
    pvm_object_t root = get_root_object_storage();
    pvm_set_field( root, PVM_ROOT_OBJECT_OS_ENTRY, pvm_root.os_entry );
    // No ref dec - we store it.

    SYSCALL_RETURN_NOTHING;
}

static int si_bootstrap_23_getenv( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN( ref_inc_o( pvm_root.kernel_environment ) );
}

static int si_bootstrap_24_reboot( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;

    //
    //CHECK_PARAM_COUNT(1);

    //pvm_object_t arg = args[0];
    
    // F11
    //phantom_shutdown(0);

    //case KEY_F12:
    hal_cpu_reset_real();

    SYSCALL_RETURN_NOTHING;
}


syscall_func_t	syscall_table_4_boot[25] =
{
    &si_void_0_construct,           	&si_void_1_destruct,
    &si_void_2_class,               	&si_void_3_clone,
    &si_void_4_equals,              	&si_bootstrap_5_tostring,
    &si_void_6_toXML,               	&si_void_7_fromXML,
    // 8
    &si_bootstrap_8_load_class,     	&invalid_syscall, //&si_bootstrap_9_load_code,
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&si_void_15_hashcode,
    // 16
    &si_bootstrap_16_print,             &si_bootstrap_17_register_class_loader,
    &si_bootstrap_18_thread,            &si_bootstrap_19_create_binary,
    &si_bootstrap_20_set_screen_background, &si_bootstrap_21_sleep,
    &si_bootstrap_22_set_os_interface,  &si_bootstrap_23_getenv,
    // 24
    &si_bootstrap_24_reboot
};
DECLARE_SIZE(boot);


// --------- array -------------------------------------------------------

static int si_array_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    // BUG? Recursively call tostring for all of them?
    SYSCALL_RETURN(pvm_create_string_object( "array" ));
}


static int si_array_8_get_iterator( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;

    
    CHECK_PARAM_COUNT(0);
    SYSCALL_THROW_STRING( "get iterator is not implemented yet" );
}


static int si_array_9_get_subarray( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;

    
    CHECK_PARAM_COUNT(2);

    /*
    int len = AS_INT(args[1]);
    int base = AS_INT(args[0]);
    SYSCALL_RETURN();
    */

    SYSCALL_THROW_STRING( "get subarray is not implemented yet" );
}


static int si_array_10_get( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;

    CHECK_PARAM_COUNT(1);

    unsigned int index = AS_INT(args[0]);

    struct data_area_4_array *da = (struct data_area_4_array *)me->da;

    if( index >= da->used_slots )
        SYSCALL_THROW_STRING( "array get - index is out of bounds" );

    pvm_object_t o = pvm_get_ofield( da->page, index);
    SYSCALL_RETURN( ref_inc_o( o ) );
}


static int si_array_11_set( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(2);

    int index = AS_INT(args[1]);
    pvm_object_t value = args[0];

    pvm_set_array_ofield( me, index, value );

    // we increment refcount and return object back.
    // it will possibly be dropped and refcount will decrement again then.
    SYSCALL_RETURN( ref_inc_o( value ) );
}

static int si_array_12_size( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(0);

    struct data_area_4_array *da = (struct data_area_4_array *)me->da;

    SYSCALL_RETURN(pvm_create_int_object( da->used_slots ) );
}




syscall_func_t	syscall_table_4_array[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_array_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_array_8_get_iterator,       &si_array_9_get_subarray,
    &si_array_10_get,               &si_array_11_set,
    &si_array_12_size,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(array);





// --------- binary -------------------------------------------------------

static int si_binary_5_tostring( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    // TODO hexdump

    if(1)
    {
        struct data_area_4_binary *da = pvm_object_da( o, binary );
        int size = o->_da_size - sizeof( struct data_area_4_binary );

        hexdump( da->data, size, "", 0);
    }

    SYSCALL_RETURN(pvm_create_string_object( "(binary)" ));
}


static int si_binary_8_getbyte( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_binary *da = pvm_object_da( me, binary );

    CHECK_PARAM_COUNT(1);
    unsigned int index = AS_INT(args[0]);

    int size = me->_da_size - sizeof( struct data_area_4_binary );

    //if( index < 0 || index >= size )
    if( index >= size )
        SYSCALL_THROW_STRING( "binary index out of bounds" );

    SYSCALL_RETURN(pvm_create_int_object( da->data[index] ));
}

static int si_binary_9_setbyte( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_binary *da = pvm_object_da( me, binary );

    CHECK_PARAM_COUNT(2);

    unsigned int byte = AS_INT(args[1]);
    unsigned int index = AS_INT(args[0]);

    int size = me->_da_size - sizeof( struct data_area_4_binary );

    //if( index < 0 || index >= size )
    if( index >= size )
        SYSCALL_THROW_STRING( "binary index out of bounds" );

    da->data[index] = byte;

    SYSCALL_RETURN_NOTHING;
}

// setrange( binary source, int from pos, int topos, int len )
static int si_binary_10_setrange( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_binary *da = pvm_object_da( me, binary );

    CHECK_PARAM_COUNT(4);

    unsigned int len = AS_INT(args[3]);
    unsigned int frompos = AS_INT(args[2]);
    unsigned int topos = AS_INT(args[1]);

    // TODO assert his class!!
    pvm_object_t _src = args[0];
    struct data_area_4_binary *src = pvm_object_da( _src, binary );


    int size = me->_da_size - sizeof( struct data_area_4_binary );

    //if( topos < 0 || topos+len > size )
    if( topos+len > size )
        SYSCALL_THROW_STRING( "binary copy dest index/len out of bounds" );

    int src_size = _src->_da_size - sizeof( struct data_area_4_binary );

    //if( frompos < 0 || frompos+len > src_size )
    if( frompos+len > src_size )
        SYSCALL_THROW_STRING( "binary copy src index/len out of bounds" );

    //da->data[index] = byte;
    memcpy( (da->data)+topos, (src->data)+frompos, len );

    SYS_FREE_O(_src);

    SYSCALL_RETURN_NOTHING;
}


syscall_func_t	syscall_table_4_binary[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_binary_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_binary_8_getbyte, &si_binary_9_setbyte,
    &si_binary_10_setrange, &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(binary);


// --------- closure -------------------------------------------------------



static int si_closure_9_getordinal( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_closure *da = pvm_object_da( me, closure );

    SYSCALL_RETURN(pvm_create_int_object( da->ordinal ));
}

static int si_closure_10_setordinal( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_closure *da = pvm_object_da( me, closure );

    CHECK_PARAM_COUNT(1);

    da->ordinal = AS_INT(args[0]);

    SYSCALL_RETURN_NOTHING;
}


static int si_closure_11_setobject( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_closure *da = pvm_object_da( me, closure );
    
    CHECK_PARAM_COUNT(1);

    // We do not decrement its refcount, 'cause we store it.
    da->object = args[0];

    SYSCALL_RETURN_NOTHING;
}


syscall_func_t	syscall_table_4_closure[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_binary_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall, 	    	    &si_closure_9_getordinal,
    &si_closure_10_setordinal, 	    &si_closure_11_setobject,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(closure);

// --------- bitmap -------------------------------------------------------

static int si_bitmap_5_tostring( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    // TODO hexdump
    SYSCALL_RETURN(pvm_create_string_object( "(bitmap)" ));
}


static int si_bitmap_8_fromstring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{

    DEBUG_INFO;
    struct data_area_4_bitmap *da = pvm_object_da( me, bitmap );

    CHECK_PARAM_COUNT(1);

//printf("Load from string\n");

    pvm_object_t _s = args[0];

    if( drv_video_string2bmp( da, pvm_object_da( _s, string)->data ) )
    	SYSCALL_THROW_STRING("can not parse graphics data");

    SYS_FREE_O(_s);

    SYSCALL_RETURN_NOTHING;
}

// TODO kill or move to tty/win?
static int si_bitmap_9_paintto( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_bitmap *da = pvm_object_da( me, bitmap );

    
    CHECK_PARAM_COUNT(3);

    int y = AS_INT(args[2]);
    int x = AS_INT(args[1]);
    pvm_object_t _tty = args[0];

    // TODO check class!
    struct data_area_4_tty *tty = pvm_object_da( _tty, tty );
    struct data_area_4_binary *pixels = pvm_object_da( da->image, binary );

    bitmap2bitmap(
    		tty->pixel, tty->w.xsize, tty->w.ysize, x, y,
    		(rgba_t *)pixels, da->xsize, da->ysize, 0, 0,
    		da->xsize, da->ysize
    );
    //drv_video_winblt( &(tty->w), tty->w.x, tty->w.y);
    w_update( &(tty->w) );

    SYS_FREE_O(_tty);

    SYSCALL_RETURN_NOTHING;
}


syscall_func_t	syscall_table_4_bitmap[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_bitmap_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_bitmap_8_fromstring,	    &si_bitmap_9_paintto,
    &invalid_syscall, 	    	    &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(bitmap);


// --------- world -------------------------------------------------------

static int si_world_5_tostring( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(world)" ));
}

//static struct pvm_object_storage	* thread_iface = 0;

static int si_world_8_getMyThread( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
/*
    // TODO spinlock!
    if(thread_iface == 0 )
    {
        struct data_area_4_class *cda = pvm_object_da( pvm_get_thread_class(), class );
        thread_iface = cda->object_default_interface;
    }
*/
    pvm_object_t out;

    out =
        (pvm_object_storage_t *)
        (tc - DA_OFFSET()); // TODO XXX HACK!
    //out.interface = thread_iface;

    SYSCALL_RETURN( ref_inc_o( out ) );
}


syscall_func_t	syscall_table_4_world[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_world_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_world_8_getMyThread,	    &invalid_syscall,
    &invalid_syscall, 	    	    &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(world);


#if COMPILE_WEAKREF
// --------- weakref -------------------------------------------------------

static int si_weakref_5_tostring( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(weakref)" ));
}


static int si_weakref_8_getMyObject( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
#if 0
    struct data_area_4_weakref *da = pvm_object_da( o, weakref );

    // All we do is return new reference to our object,
    // incrementing refcount before
    int ie = hal_save_cli();
    hal_spin_lock( &da->lock );

    pvm_object_t out = ref_inc_o( da->object );

    hal_spin_unlock( &da->lock );
    if( ie ) hal_sti();
#else
    pvm_object_t out = pvm_weakref_get_object( o );
#endif
    SYSCALL_RETURN( out );

}

errno_t si_weakref_9_resetMyObject(pvm_object_t o )
{
    struct data_area_4_weakref *da = pvm_object_da( o, weakref );

    errno_t rc = EWOULDBLOCK;

#if WEAKREF_SPIN
    wire_page_for_addr( &da->lock );
    int ie = hal_save_cli();
    hal_spin_lock( &da->lock );
#else
    hal_mutex_lock( &da->mutex );
#endif

    // As we are interlocked with above, no refcount inc can be from us
    // ERROR if more than one weakref is pointing to obj, possibility
    // exist than GC will reset some of them, and than will still let
    // object to exist. We need to make sure only one weakref exists.
    if(da->object->_ah.refCount == 0)
    {
        da->object = 0;
        //da->object.interface = 0;
        rc = 0;
    }

#if WEAKREF_SPIN
    hal_spin_unlock( &da->lock );
    if( ie ) hal_sti();
    unwire_page_for_addr( &da->lock );
#else
    hal_mutex_unlock( &da->mutex );
#endif

    return rc;
}





syscall_func_t	syscall_table_4_weakref[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_weakref_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_weakref_8_getMyObject,	    &invalid_syscall,
    &invalid_syscall, 	    	    &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(weakref);


#endif




// --------- directory -------------------------------------------------------



static int si_directory_4_equals( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_THROW_STRING( "dir.equals: not implemented" );
    //SYSCALL_RETURN(pvm_create_string_object( "(directory)" ));
}


static int si_directory_5_tostring( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(directory)" ));
}

static int si_directory_8_put( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_directory *da = pvm_object_da( o, directory );
    DEBUG_INFO;

    CHECK_PARAM_COUNT(2);

    pvm_object_t val = args[1];
    pvm_object_t key = args[0];
    ASSERT_STRING(key);

    errno_t rc = hdir_add( da, pvm_get_str_data(key), pvm_get_str_len(key), val );

    SYS_FREE_O(key); // dir code creates it's own binary object
    if( rc ) SYS_FREE_O( val ); // we didn't put it there

    SYSCALL_RETURN(pvm_create_int_object( rc ));
}

static int si_directory_9_get( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_directory *da = pvm_object_da( o, directory );
    DEBUG_INFO;

    CHECK_PARAM_COUNT(1);
    pvm_object_t key = args[0];
    ASSERT_STRING(key);

    pvm_object_t out;
    errno_t rc = hdir_find( da, pvm_get_str_data(key), pvm_get_str_len(key), &out, 0 );
    if( rc )
        SYSCALL_RETURN_NOTHING;
    else
        SYSCALL_RETURN(out);
}


static int si_directory_10_remove( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_directory *da = pvm_object_da( o, directory );
    DEBUG_INFO;

    CHECK_PARAM_COUNT(1);

    pvm_object_t key = args[0];
    ASSERT_STRING(key);

    pvm_object_t out; // unused
    errno_t rc = hdir_find( da, pvm_get_str_data(key), pvm_get_str_len(key), &out, 1 );
    SYSCALL_RETURN(pvm_create_int_object( rc ));
}

static int si_directory_11_size( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_directory *da = pvm_object_da( o, directory );
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_int_object( da->nEntries ));
}

// Returns iterator
static int si_directory_12_iterate( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    // TODO implement dir iterator

    SYSCALL_THROW_STRING( "dir.iterate: not implemented" );
    SYSCALL_RETURN_NOTHING;
    //return pvm_create_null_object();
}




syscall_func_t	syscall_table_4_directory[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_directory_4_equals,         &si_directory_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_directory_8_put, 	    	&si_directory_9_get,
    &si_directory_10_remove,        &si_directory_11_size,
    &si_directory_12_iterate,       &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode,

};
DECLARE_SIZE(directory);





// --------- connection -------------------------------------------------------

static int si_connection_5_tostring( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(connection)" ));
}

static int si_connection_8_connect( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_connection *da = pvm_object_da( o, connection );

    CHECK_PARAM_COUNT(1);

    pvm_object_t _s = args[0];

    if(!IS_PHANTOM_STRING(_s))
    {
        SYS_FREE_O(_s);
        SYSCALL_THROW_STRING( "connection.connect: not a string arg" );
    }

    int slen = pvm_get_str_len(_s);

    if( slen+1 > sizeof( da->name ) )
    {
        SYS_FREE_O(_s);
        SYSCALL_THROW_STRING( "string arg too long" );
    }

    strncpy( da->name, pvm_get_str_data(_s), slen );
    SYS_FREE_O(_s);

    printf(".internal.connection: Connect to '%s'\n", da->name );

    int iret = pvm_connect_object(o,tc);

    SYSCALL_RETURN(pvm_create_int_object( iret ) );
}


static int si_connection_9_disconnect( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    //struct data_area_4_connection *da = pvm_object_da( o, connection );

    
    CHECK_PARAM_COUNT(0);

    int iret = pvm_disconnect_object(o,tc);

    SYSCALL_RETURN(pvm_create_int_object( iret ) );
}

static int si_connection_10_check( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_connection *da = pvm_object_da( o, connection );

    CHECK_PARAM_COUNT(1);

    int op_index = AS_INT(args[0]);

#if !PVM_CONNECTION_WAKE
    (void) op_index;
    (void) da;
    int iret = ENXIO;
#else
    int iret = 0;

    if( da->kernel == 0 || da->kernel->check_operation || da->kernel->req_wake )
    {
        iret = ENXIO;
    }
    else
    {
        iret = da->kernel->check_operation( op_index, da, tc );
        if( iret )
        {
            // TODO races?
            ret = da->kernel->req_wake( op_index, da, tc );
            if( iret == 0 )
                SYSCALL_PUT_THIS_THREAD_ASLEEP();
        }
    }
#endif
    SYSCALL_RETURN(pvm_create_int_object( iret ) );
}

static int si_connection_11_do( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_connection *da = pvm_object_da( o, connection );

    CHECK_PARAM_COUNT(2);

    int op_index = AS_INT(args[1]);
    pvm_object_t arg = args[0];

    int iret = 0;

    if( (da->kernel == 0) || (da->kernel->do_operation == 0) )
    {
        iret = ENXIO;
    }
    else
    {
        iret = da->kernel->do_operation( op_index, da, tc, arg );
    }

    SYS_FREE_O(arg);
    SYSCALL_RETURN(pvm_create_int_object( iret ) );
}


static int si_connection_12_set_callback( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_connection *da = pvm_object_da( o, connection );
    
    CHECK_PARAM_COUNT(2);

    int nmethod = AS_INT(args[1]);
    pvm_object_t callback_object = args[0];

    int iret = !pvm_isnull( da->callback );

    // No sync - assume caller does it before getting real callbacks
    da->callback = callback_object;
    da->callback_method = nmethod;

    SYSCALL_RETURN(pvm_create_int_object( iret ) );
}


static int si_connection_13_blocking( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_connection *da = pvm_object_da( o, connection );

    CHECK_PARAM_COUNT(2);

    pvm_object_t (*syscall_worker)( pvm_object_t , struct data_area_4_thread *, int nmethod, pvm_object_t arg ) = da->blocking_syscall_worker;

    //SHOW_FLOW( 1, "blocking call to nmethod = %d", nmethod);
    return vm_syscall_block( o, tc, syscall_worker, ret, n_args, args );
}



syscall_func_t	syscall_table_4_connection[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_connection_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_connection_8_connect, 	    &si_connection_9_disconnect,
    &si_connection_10_check, 	    &si_connection_11_do,
    &si_connection_12_set_callback, &si_connection_13_blocking,
    &invalid_syscall,               &si_void_15_hashcode,

};
DECLARE_SIZE(connection);






