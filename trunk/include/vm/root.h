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

int pvm_connect_object(pvm_object_t o, struct data_area_4_thread *tc);
int pvm_disconnect_object(pvm_object_t o, struct data_area_4_thread *tc);




struct pvm_root_t
{
    struct pvm_object           null_class;             // Default superclass for any class

    struct pvm_object           class_class;            // Class of a class (and of itself)
    struct pvm_object           interface_class;
    struct pvm_object           code_class;

    struct pvm_object           int_class;
    struct pvm_object           string_class;

    struct pvm_object           array_class;
    struct pvm_object           page_class;

    struct pvm_object           thread_class;
    struct pvm_object           call_frame_class;
    struct pvm_object           istack_class;
    struct pvm_object           ostack_class;
    struct pvm_object           estack_class;

    struct pvm_object           boot_class;

    struct pvm_object           binary_class;
    struct pvm_object           bitmap_class;

    struct pvm_object           world_class;
    struct pvm_object           closure_class;

    struct pvm_object           weakref_class;
    struct pvm_object           window_class;

    struct pvm_object           directory_class;
    struct pvm_object           connection_class;

    struct pvm_object           null_object;
    struct pvm_object           sys_interface_object;   // Each method is a consecutive syscall (sys 0 first, sys 1 second etc) + return
    struct pvm_object           class_loader;           // Root class loader (user code)
    struct pvm_object           threads_list;           // Array? of threads
    struct pvm_object           restart_list;           // Array of weak refs to objects that need attention at restart - XXX func called?
    struct pvm_object           users_list;           	// Array? of users - NOT IMPLEMENTED
    struct pvm_object           kernel_environment;     // Array? of users - NOT IMPLEMENTED
    struct pvm_object           os_entry;               // Main OS services entry point
    struct pvm_object           root_dir;               // Main OS services entry point


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


#define PVM_ROOT_OBJECTS_COUNT (PVM_ROOT_OBJECT_ROOT_DIR+32)



#endif
