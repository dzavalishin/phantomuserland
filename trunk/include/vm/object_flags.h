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

// flag marking object as having no pointers out - optimization for GC and refcount decr
POSF(IS_CHILDFREE,0x800)

// Object is thread. This is needed by refcount code to free stacks manually
POSF(IS_THREAD,0x400)

// Object is i/o/e stack. This is needed to allocate stacks efficiently
POSF(IS_STACK_FRAME,0x200)

// Object is call frame. This is needed by refcount code to free stacks manually
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

// flags:

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

#endif // PO_OBJECT_FLAGS_H


