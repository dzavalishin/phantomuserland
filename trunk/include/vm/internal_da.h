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

#ifndef PVM_INTERNAL_DA_H
#define PVM_INTERNAL_DA_H


#include "vm/object.h"
#include "vm/exception.h"
#include "drv_video_screen.h"

#include "hal.h"


#define pvm_object_da( o, type ) ((struct data_area_4_##type *)&(o.data->da))
#define pvm_data_area( o, type ) ((struct data_area_4_##type *)&(o.data->da))

// Num of slots in normal (noninternal) object
#define da_po_limit(o)	 (o->_da_size/sizeof(struct pvm_object))
// Slots access for noninternal object
#define da_po_ptr(da)  ((struct pvm_object *)&da)


void pvm_fill_syscall_interface( struct pvm_object iface, int syscall_count );

/**
 *
 * Internal objects data areas.
 *
**/

struct data_area_4_array
{
    struct pvm_object      	page;
    int                 	page_size; // current page size
    int                 	used_slots; // how many slots are used now
};



struct data_area_4_call_frame
{
    struct pvm_object		istack; // integer (fast calc) stack
    struct pvm_object		ostack; // main object stack
    struct pvm_object		estack; // exception catchers

    //struct pvm_object		cs; 	// code segment

    unsigned int		IP_max;	// size of code in bytes
    unsigned char *		code; 	// (byte)code itself

    unsigned int    		IP;

    // TODO add code object ref - just for gc

    struct pvm_object  		this_object;

    struct pvm_object		prev; // where to return!
};



struct data_area_4_code
{
    unsigned int		code_size; // bytes
    unsigned char		code[];
};




struct data_area_4_int
{
    int				value;
};

// TODO: make sure it is not an lvalue for security reasons!
#define pvm_get_int( o )  ( (int) (((struct data_area_4_int *)&(o.data->da))->value))



struct data_area_4_string
{
    int				length; // bytes! (chars are unicode?)
    unsigned char		data[];
};

#define pvm_get_str_len( o )  ( (int) (((struct data_area_4_string *)&(o.data->da))->length))
#define pvm_get_str_data( o )  ( (char *) (((struct data_area_4_string *)&(o.data->da))->data))



struct data_area_4_class
{
    unsigned int		object_flags;			// object of this class will have such flags
    unsigned int		object_data_area_size;	// object of this class will have data area of this size
    struct pvm_object		object_default_interface; // default one
    //syscall_func_t  *class_sys_table;
    //unsigned int	class_sys_table_size;
    unsigned int    		sys_table_id; // See above - index into the kernel's syscall tables table

    struct pvm_object		class_name;
    struct pvm_object		class_parent;

    // TODO add c'tor ptr and GC iterator func ptr for internal ones
};



struct pvm_code_handler
{
    const unsigned char *   	code;
    unsigned int            	IP;
    unsigned int            	IP_max;
};



// TODO - some sleep/wakeup support
struct data_area_4_thread
{
    struct pvm_code_handler     		code; 		// Loaded by load_fast_acc from frame

    //unsigned long   		thread_id; // Too hard to implement and nobody needs
    struct pvm_object  				call_frame; 	// current

    // some owner pointer?
    struct pvm_object 				owner;
    struct pvm_object 				environment;

    hal_spinlock_t 				spin; // used on manipulations with sleep_flag

    volatile int                                sleep_flag;     // Is true if thread is put asleep in userland
    hal_cond_t   				wakeup_cond;    // Will sleep here


// fast access copies

    struct pvm_object  				_this_object; 	// Loaded by load_fast_acc from frame

    struct data_area_4_integer_stack *		_istack;        // Loaded by load_fast_acc from frame
    struct data_area_4_object_stack *		_ostack;        // Loaded by load_fast_acc from frame
    struct data_area_4_exception_stack *      	_estack;        // Loaded by load_fast_acc from frame

};

/*
// Thread factory is used to keep thread number
struct data_area_4_thread_factory
{
};
*/


struct pvm_stack_da_common
{
    struct pvm_object           	root;
    struct pvm_object           	curr;

    struct pvm_object  			prev;
    struct pvm_object  			next;

    unsigned int    			free_cell_ptr; // number of cells used
    unsigned int                        __sSize; // Page array has this many elements
};


#define PVM_INTEGER_STACK_SIZE 64

struct data_area_4_integer_stack
{
    struct pvm_stack_da_common          common;

    struct data_area_4_integer_stack *  curr_da;

    int         			stack[PVM_INTEGER_STACK_SIZE];
};




#define PVM_OBJECT_STACK_SIZE 64

struct data_area_4_object_stack
{
    struct pvm_stack_da_common          common;

    struct data_area_4_object_stack *  	curr_da;

    struct pvm_object			stack[PVM_OBJECT_STACK_SIZE];
};



#define PVM_EXCEPTION_STACK_SIZE 64

struct data_area_4_exception_stack
{
    struct pvm_stack_da_common          common;

    struct data_area_4_exception_stack *	curr_da;

    struct pvm_exception_handler       stack[PVM_EXCEPTION_STACK_SIZE];
};


struct data_area_4_boot
{
    // Empty?
};



//#define PVM_DEF_TTY_XSIZE 800
//#define PVM_DEF_TTY_YSIZE 600
#define PVM_DEF_TTY_XSIZE 400
#define PVM_DEF_TTY_YSIZE 300
//#define PVM_MAX_TTY_PIXELS (1280*1024)
#define PVM_MAX_TTY_PIXELS (PVM_DEF_TTY_XSIZE*PVM_DEF_TTY_YSIZE)

struct data_area_4_tty
{
    drv_video_window_t           	w;
    rgba_t       			pixel[PVM_MAX_TTY_PIXELS]; // this extends w and works as it's last field

    int                                 font_height; // Font is hardcoded yet - BUG - but we cant have ptr to kernel from object
    int                                 font_width;
    int 				xsize, ysize; // in chars
    int                                 x, y; // in pixels
    rgba_t                       	fg, bg; // colors
};



struct data_area_4_mutex
{
    //void *mutex; // hack - depends on mutex definition in OS KIT
    // TODO need queue!
    struct data_adrea_4_thread  *sleeping; // Which thread is sleeping
};



struct data_area_4_binary
{
    int                 placeholder; // GCC refuses to have only unsized array in a struct
    unsigned char 	data[];
};


struct data_area_4_closure
{
    struct pvm_object   object; // Which object to call
    int                 ordinal; // Which methos to call
};


struct data_area_4_bitmap
{
    struct pvm_object   image; // .internal.binary
    int                 xsize;
    int                 ysize;
};


struct data_area_4_tcp
{
    //int waiting for recv
};



struct data_area_4_world
{
    int                 placeholder[8]; // for any case
};















#endif // PVM_INTERNAL_DA_H

