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


#endif // PO_OBJECT_FLAGS_H


