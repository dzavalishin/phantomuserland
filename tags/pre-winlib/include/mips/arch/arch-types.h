
#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__

typedef	__signed char          int8_t;
typedef	unsigned char        u_int8_t;
typedef	short                 int16_t;
typedef	unsigned short      u_int16_t;

#ifndef __INT32_DEFINED__
#define __INT32_DEFINED__
typedef	signed int            int32_t;
#endif

typedef	unsigned int        u_int32_t;

typedef	signed long long      int64_t;
typedef	unsigned long long  u_int64_t;

#endif


typedef void *  		    vmem_ptr_t;
typedef unsigned long 	    addr_t;

// physical mem address 	
typedef u_int32_t       	physaddr_t;
// linear mem address
typedef u_int32_t       	linaddr_t;


#ifndef _STDINT_H
//typedef	u_int32_t		    uintptr_t;
typedef	int32_t			    ptrdiff_t;
typedef u_int32_t 		    register_t;
#endif



#ifndef _SIZE_T
#define _SIZE_T     	
#define	_SIZE_T_DECLARED
typedef unsigned        intsize_t;
typedef int             ssize_t;
typedef unsigned int    size_t;
#endif //_SIZE_T

#ifndef _OFF_T
#define _OFF_T
typedef int			off_t;
typedef int			_off_t;
#endif //_OFF_T




// ------------ From Mach

/*
 * A natural_t is the type for the native
 * integer type, e.g. 32 or 64 or.. whatever
 * register size the machine has.  Unsigned, it is
 * used for entities that might be either
 * unsigned integers or pointers, and for
 * type-casting between the two.
 * For instance, the IPC system represents
 * a port in user space as an integer and
 * in kernel space as a pointer.
 */
typedef unsigned int            natural_t;


/*
 * A vm_offset_t is a type-neutral pointer,
 * e.g. an offset into a virtual memory space.
 */
typedef        natural_t        vm_offset_t;

/*
 * A vm_size_t is the proper type for e.g.
 * expressing the difference between two
 * vm_offset_t entities.
 */
typedef        natural_t        vm_size_t;


/* Integer types capable of holding object pointers */

#ifndef __intptr_t_defined
#define __intptr_t_defined
typedef int intptr_t;
typedef unsigned int uintptr_t;
#endif

