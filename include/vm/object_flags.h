/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Object header flags
 *
 *
**/

#ifndef PO_OBJECT_FLAGS_H
#define PO_OBJECT_FLAGS_H

#if 0

#ifdef POSF_CONST_INIT
#define	POSF(name,val)	\
    unsigned int	PHANTOM_OBJECT_STORAGE_FLAG_##name = val;
#else
#define	POSF(name,val)	\
    extern unsigned int	PHANTOM_OBJECT_STORAGE_FLAG_##name;
#endif
/*
	 void POSF_SET_##name(unsigned int &v) { v |= val; } \
	 void POSF_RESET_##name(unsigned int &v) { v &= ~val; } \
	 unsigned int POSF_GET_##name(unsigned int v) { return v & val; }
*/

// object must not be modofied at all - NOT IMPLEMENTED
POSF(IS_IMMUTABLE, 0x2000)


// flag object as having finalizer
POSF(IS_FINALIZER, 0x1000)

// flag marking object as having no pointers out - optimization for GC and refcount decr
POSF(IS_CHILDFREE,0x800)

// Object is thread. For information purposes only
POSF(IS_THREAD,0x400)

// Object is i/o/e stack. This is needed to allocate stacks efficiently
POSF(IS_STACK_FRAME,0x200)

// Object is call frame. This is needed to allocate stacks efficiently
POSF(IS_CALL_FRAME,0x100)

// implementation is in VM/kernel
POSF(IS_INTERNAL,0x80)

// NOT IMPLEMENTED
// means not internal and can grow on out of bound store access
POSF(IS_RESIZEABLE,0x40)

// I am a string object
POSF(IS_STRING,0x20)

// I am an integer object
POSF(IS_INT,0x10)

// NOT IMPLEMENTED
// means compose and decompose can be used
POSF(IS_DECOMPOSEABLE,0x08)

// I am class object
POSF(IS_CLASS,0x04)

// my objects are code containetrs, call_method works differently
POSF(IS_INTERFACE,0x02)

// I AM the code container - do we need this flag?
POSF(IS_CODE,0x01)



#undef POSF

#endif

// TODO rename PHANTOM_OBJECT_STORAGE_FLAG_IS_ --> PHANTOM_OBJECT_FLAG_IS_

// flags: o->_flags
// These are set on creation by class
// NB - 0x100000 is used below

#define PHANTOM_OBJECT_STORAGE_FLAG_IS_ARENA         0x8000 // Allocation arena marker object
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_ROOT          0x4000 // I am Root
// object must not be modified at all - IMPLEMENTED PARTIALLY - can be set in run time
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE 0x2000
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_FINALIZER 0x1000
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE 0x800
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_THREAD 0x400
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME 0x200
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME 0x100
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL 0x80
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE 0x40
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING 0x20
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_INT 0x10
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_DECOMPOSEABLE 0x08
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS 0x04
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE 0x02
#define PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE 0x01

// NB! Runtime flags!
// These can be set/reset in runtime

// This object has week ref on it (must be on _satellites chain)
#define PHANTOM_OBJECT_STORAGE_FLAG_HAS_WEAKREF 0x100000




#define PHANTOM_ARENA_FOR_INT             (1<<1) // For allocation of ints
#define PHANTOM_ARENA_FOR_STACK           (1<<2) // For allocation of stack frames
#define PHANTOM_ARENA_FOR_STATIC          (1<<3) // For allocation of classes, interfaces, code, etc
#define PHANTOM_ARENA_FOR_SMALL           (1<<4) // For allocation of < 1K

#define PHANTOM_ARENA_FOR_THREAD_INT      (1<<7) // Thread personal ints
#define PHANTOM_ARENA_FOR_THREAD_STACK    (1<<8) // Thread personal stack frames
#define PHANTOM_ARENA_FOR_THREAD_SMALL    (1<<8) // Thread personal small objects

#define PHANTOM_ARENA_FOR_1K              (1<<10) // For [1,2[K
#define PHANTOM_ARENA_FOR_2K              (1<<11) // For [2,4[K
// And so on, must be allocated on request

#endif // PO_OBJECT_FLAGS_H


