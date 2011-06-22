#ifndef COMPAT_SEABIOS_H
#define COMPAT_SEABIOS_H


#include <compat/shorttype-def.h>
#include <phantom_types.h>
#include <hal.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>


#define DEBUG_MSG_PREFIX "uhci"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10



#define dprintf SHOW_FLOW
#define msleep hal_sleep_msec
#define yield()




#ifndef NULL
#  define NULL 0
#endif

#define PACKED __attribute__((packed))

#define warn_noalloc() panic("no mem")
#define warn_internalerror() panic("internal error")
#define warn_timeout() panic("timeout")

#define ASSERT32FLAT() do {} while(0)
#define MODE16 0
#define ASSERT16() panic("16 bit USB code called")

// TODO check - Used by 16 bit only code?
#define FLATPTR_TO_SEG(v) (v)
#define FLATPTR_TO_OFFSET(v) (v)

#define CONFIG_USB 1
#define CONFIG_USB_EHCI 1
#define CONFIG_USB_OHCI 1
#define CONFIG_USB_UHCI 1

#define mutex_lock hal_mutex_lock
#define mutex_unlock hal_mutex_unlock





#define GET_FARVAR(seg, var) \
    (*((typeof(&(var)))MAKE_FLATPTR((seg), &(var))))
#define SET_FARVAR(seg, var, val) \
    do { GET_FARVAR((seg), (var)) = (val); } while (0)
#define GET_VAR(seg, var) (var)
#define SET_VAR(seg, var, val) do { (var) = (val); } while (0)
#define SET_SEG(SEG, value) ((void)(value))
#define GET_SEG(SEG) 0
#define GET_FLATPTR(ptr) (ptr)
#define SET_FLATPTR(ptr, val) do { (ptr) = (val); } while (0)

// TODO check use!
#define MAKE_FLATPTR(seg,off) ((void*)(((u32)(seg)<<4)+(u32)(off)))




#define noinline __attribute__((noinline))





static inline u32 __ffs(u32 word)
{
    asm("bsf %1,%0"
        : "=r" (word)
        : "rm" (word));
    return word;
}
static inline u32 __fls(u32 word)
{
    asm("bsr %1,%0"
        : "=r" (word)
        : "rm" (word));
    return word;
}


#define ALIGN(x,a)              __ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN_DOWN(x,a)         ((x) & ~((typeof(x))(a)-1))

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define DIV_ROUND_CLOSEST(x, divisor)({                 \
            typeof(divisor) __divisor = divisor;        \
            (((x) + ((__divisor) / 2)) / (__divisor));  \
        })


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})



#define barrier() __asm__ __volatile__("": : :"memory")



#endif // COMPAT_SEABIOS_H
