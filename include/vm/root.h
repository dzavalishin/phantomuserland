/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes?
 *
 *
 **/

#ifndef PVM_ROOT_H
#define PVM_ROOT_H

#include "vm/object.h"

// TODO - to be able to grow root object store at address 0 just a pointer to it
// (one-slot array?)

void pvm_root_init(void);

void phantom_setenv( const char *name, const char *value );
int phantom_getenv( const char *name, char *value, int vsize );

void pvm_add_object_to_restart_list( pvm_object_t o );
void pvm_remove_object_from_restart_list( pvm_object_t o );

int pvm_connect_object(pvm_object_t o, struct data_area_4_thread *tc);
int pvm_disconnect_object(pvm_object_t o, struct data_area_4_thread *tc);

// Persistent class cache
//
// Used twice - both in '.internal.boot' class loader method and
// in exec.c class name to class resolution. Possibly, we should remove
// its use from '.internal.boot'.
//
errno_t pvm_class_cache_lookup(const char *name, int name_len, pvm_object_t *new_class);
errno_t pvm_class_cache_insert(const char *name, int name_len, pvm_object_t new_class);



struct pvm_root_t
{
    pvm_object_t           null_class;             // Default superclass for any class

    pvm_object_t           class_class;            // Class of a class (and of itself)
    pvm_object_t           interface_class;
    pvm_object_t           code_class;

    pvm_object_t           int_class;
    pvm_object_t           long_class;
    pvm_object_t           float_class;
    pvm_object_t           double_class;
    pvm_object_t           string_class;

    pvm_object_t           array_class;
    pvm_object_t           page_class;

    pvm_object_t           thread_class;
    pvm_object_t           call_frame_class;
    pvm_object_t           istack_class;
    pvm_object_t           ostack_class;
    pvm_object_t           estack_class;

    pvm_object_t           boot_class;

    pvm_object_t           binary_class;
    pvm_object_t           bitmap_class;

    pvm_object_t           world_class;
    pvm_object_t           closure_class;

    pvm_object_t           weakref_class;
    pvm_object_t           window_class;

    pvm_object_t           directory_class;
    pvm_object_t           connection_class;

    pvm_object_t           mutex_class;
    pvm_object_t           cond_class;
    pvm_object_t           sema_class;

    pvm_object_t           tcp_class;
    pvm_object_t           udp_class;

    pvm_object_t           net_class;
    pvm_object_t           http_class;

    pvm_object_t           time_class;
    pvm_object_t           stat_class;

    pvm_object_t           port_class;
    pvm_object_t           io_class;

    pvm_object_t           ui_control_class;
    pvm_object_t           ui_font_class;


    pvm_object_t           null_object;
    pvm_object_t           sys_interface_object;   // Each method is a consecutive syscall (sys 0 first, sys 1 second etc) + return
    pvm_object_t           class_loader;           // Root class loader (user code)
    pvm_object_t           threads_list;           // Array? of threads
    pvm_object_t           restart_list;           // Array of weak refs to objects that need attention at restart - XXX func called?
    pvm_object_t           users_list;           	// Array? of users - NOT IMPLEMENTED
    pvm_object_t           kernel_environment;     // Array? of users - NOT IMPLEMENTED
    pvm_object_t           os_entry;               // Main OS services entry point
    pvm_object_t           root_dir;               // Root object directory
    pvm_object_t           kernel_stats;           // Persisent kernel statistics
    pvm_object_t           class_dir;              // .internal.directory of all classes used - class load cache - TODO must use weak refs or cleanup on ref cnt == 1

};

extern struct pvm_root_t pvm_root;

// Max number of methods in system class - determines size of interface class
#define N_SYS_METHODS 64

/**
 *
 * The very first (at the beginning of the address space) phantom
 * object is a runtime directory used to restart OS. It contains references
 * to the objects kernel needs to know for system to run.
 *
 * Here we have field numbers for those objects to extract them from root object.
 *
**/

// Pointer to the null object
#define PVM_ROOT_OBJECT_NULL_CLASS 0

// Pointer to class class object (used to create classes)
#define PVM_ROOT_OBJECT_CLASS_CLASS 1

// Pointer to interface class
#define PVM_ROOT_OBJECT_INTERFACE_CLASS 2

// Pointer to code class
#define PVM_ROOT_OBJECT_CODE_CLASS 3

// Pointer to integer class
#define PVM_ROOT_OBJECT_INT_CLASS 4

// Pointer to string class
#define PVM_ROOT_OBJECT_STRING_CLASS 5

// Pointer to array class
#define PVM_ROOT_OBJECT_ARRAY_CLASS 6

// Pointer to page class
#define PVM_ROOT_OBJECT_PAGE_CLASS 7

// Pointer to thread class
#define PVM_ROOT_OBJECT_THREAD_CLASS 8

// Pointer to call frame class
#define PVM_ROOT_OBJECT_CALL_FRAME_CLASS 9

// Pointer to istack class
#define PVM_ROOT_OBJECT_ISTACK_CLASS 10

// Pointer to ostack class
#define PVM_ROOT_OBJECT_OSTACK_CLASS 11

// Pointer to estack class
#define PVM_ROOT_OBJECT_ESTACK_CLASS 12

// Pointer to long (64 bit) class
#define PVM_ROOT_OBJECT_LONG_CLASS 13

// Pointer to float class
#define PVM_ROOT_OBJECT_FLOAT_CLASS 14

// Pointer to double (64 bit) class
#define PVM_ROOT_OBJECT_DOUBLE_CLASS 15


// Special classes

// Pointer to botstrap class - used on a new fresh system creation
// to suck in the rest of the system
#define PVM_ROOT_OBJECT_BOOT_CLASS 16

// tty class
#define PVM_ROOT_OBJECT_TTY_CLASS 17

#define PVM_ROOT_OBJECT_CLASS_LOADER 18

//#define PVM_ROOT_OBJECT_DRIVER_CLASS 19

#define PVM_ROOT_OBJECT_MUTEX_CLASS 20

#define PVM_ROOT_OBJECT_COND_CLASS 21

#define PVM_ROOT_OBJECT_BINARY_CLASS 22

#define PVM_ROOT_OBJECT_BITMAP_CLASS 23

#define PVM_ROOT_OBJECT_WORLD_CLASS 24

#define PVM_ROOT_OBJECT_CLOSURE_CLASS 25


#define PVM_ROOT_OBJECT_UDP_CLASS 26

#define PVM_ROOT_OBJECT_TCP_CLASS 27

#define PVM_ROOT_OBJECT_WEAKREF_CLASS 28

#define PVM_ROOT_OBJECT_WINDOW_CLASS 29

#define PVM_ROOT_OBJECT_DIRECTORY_CLASS 30

#define PVM_ROOT_OBJECT_CONNECTION_CLASS 31

#define PVM_ROOT_OBJECT_SEMA_CLASS 32

#define PVM_ROOT_OBJECT_NET_CLASS 33

#define PVM_ROOT_OBJECT_HTTP_CLASS 34

#define PVM_ROOT_OBJECT_TIME_CLASS 35

#define PVM_ROOT_OBJECT_STAT_CLASS 36

#define PVM_ROOT_OBJECT_IO_CLASS 37

#define PVM_ROOT_OBJECT_PORT_CLASS 38

#define PVM_ROOT_OBJECT_UI_CONTROL_CLASS 39

#define PVM_ROOT_OBJECT_UI_FONT_CLASS 40




// Runtime restoration facilities





// NULL object (is_null() checks against this one)
#define PVM_ROOT_OBJECT_NULL 64

#define PVM_ROOT_OBJECT_SYSINTERFACE 65

// Pointer to actual thread list, used to restart threads
#define PVM_ROOT_OBJECT_THREAD_LIST 66

#define PVM_ROOT_OBJECT_USERS_LIST 67

#define PVM_ROOT_OBJECT_RESTART_LIST 68

#define PVM_ROOT_OBJECT_KERNEL_ENVIRONMENT 69

#define PVM_ROOT_OBJECT_OS_ENTRY 70

// Root object directory
#define PVM_ROOT_OBJECT_ROOT_DIR 71

#define PVM_ROOT_KERNEL_STATISTICS 72

#define PVM_ROOT_CLASS_DIR 73

#define PVM_ROOT_OBJECTS_COUNT (PVM_ROOT_CLASS_DIR+30)



#endif
