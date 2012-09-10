#include <device.h>

#define u8_t u_int8_t
#define u16_t u_int16_t
#define u32_t u_int32_t

#define uint8_t u_int8_t
#define uint16_t u_int16_t
#define uint32_t u_int32_t

#define CONST const

#define HANDLE hal_mutex_t


#define NUTDEVICE phantom_device_t
#define dev_dcb drv_private



/*
static inline int NutUdpReceiveFrom( void *prot_data, uint32_t *raddr, uint16_t *rport, void *buf, size_t bufsz, int flags )
{
    i4sockaddr saddr;

    //ssize_t rc = udp_recvfrom( prot_data, void *buf, ssize_t len, i4sockaddr *saddr, int flags, bigtime_t timeout);
    ssize_t rc = udp_recvfrom( prot_data, buf, bufsz, &saddr, 0, 0);

    return rc;
}

//ssize_t udp_sendto(void *prot_data, const void *buf, ssize_t len, i4sockaddr *addr);
*/


#define NutSleep hal_sleep_msec
#define NutDelay hal_sleep_msec


#define NutEventPostFromIrq hal_cond_broadcast


#define NutThreadCreate( __name, __f, __a, _stk) \
	hal_start_kernel_thread_arg( (__f), (__a))


#define THREAD(__f, __arg) void __f(void *__arg)





#define _NOP() __asm__ __volatile__ ("mov r0, r0  @ _NOP")




#define outb(_reg, _val)  (*((volatile unsigned char *)(_reg)) = (_val))
#define outw(_reg, _val)  (*((volatile unsigned short *)(_reg)) = (_val))
#define outr(_reg, _val)  (*((volatile unsigned int *)(_reg)) = (_val))

#define inb(_reg)   (*((volatile unsigned char *)(_reg)))
#define inw(_reg)   (*((volatile unsigned short *)(_reg)))
#define inr(_reg)   (*((volatile unsigned int *)(_reg)))

#define _BV(bit)    (1 << (bit))


#define sbi(_reg, _bit)         outr(_reg, inr(_reg) | _BV(_bit))
#define cbi(_reg, _bit)         outr(_reg, inr(_reg) & ~_BV(_bit))
#define bit_is_set(_reg, _bit)  ((inr(_reg) & _BV(_bit)) != 0)

#ifndef __BEGIN_DECLS
#define __BEGIN_DECLS
#endif

#ifndef __END_DECLS
#define __END_DECLS
#endif

