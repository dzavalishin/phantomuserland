#ifndef _UAPI_E2K_API_H_
#define _UAPI_E2K_API_H_

#include <asm/mas.h>

#ifndef	__ASSEMBLY__
typedef unsigned char __e2k_u8_t;
typedef unsigned short int __e2k_u16_t;
typedef unsigned int __e2k_u32_t;
typedef unsigned long long __e2k_u64_t;
typedef void *__e2k_ptr_t;

typedef struct { __e2k_u64_t lo, hi; } e2k_reg128_t;
#endif /* __ASSEMBLY__ */


#define E2K_SET_REG(reg_no, val) \
({ \
	asm volatile ("adds \t0x0, %0, %%r" #reg_no \
		: \
		: "ri" ((__e2k_u32_t) (val))); \
})

#define E2K_SET_DREG(reg_no, val) \
({ \
	asm volatile ("addd \t0x0, %0, %%dr" #reg_no \
		: \
		: "ri" ((__e2k_u64_t) (val))); \
})

#define E2K_SET_DGREG(reg_no, val) \
({ \
	asm volatile ("addd \t0x0, %0, %%dg" #reg_no \
		: \
		: "ri" ((__e2k_u64_t) (val))); \
})
#define E2K_SET_DGREG_NV(reg_no, val) \
({ \
	asm ("addd \t%0, 0, %%dg" #reg_no \
		: \
		: "ri" ((__e2k_u64_t) (val))); \
})


#define E2K_GET_BREG(reg_no) \
({ \
	register __e2k_u32_t res; \
	asm volatile ("adds \t0x0, %%b[" #reg_no "], %0" \
		 : "=r" (res)); \
		res; \
})

#define E2K_GET_DBREG(reg_no) \
({ \
	register __e2k_u64_t res; \
	asm volatile ("addd \t0x0, %%db[" #reg_no "], %0" \
		: "=r" (res)); \
		res; \
})

#define E2K_SET_BREG(reg_no, val) \
({ \
	asm volatile ("adds \t0x0, %0, %%b[" #reg_no "]" \
		: \
		: "ri" ((__e2k_u32_t) (val))); \
})

#define E2K_SET_DBREG(reg_no, val) \
({ \
	asm volatile ("addd \t0x0, %0, %%db[" #reg_no "]" \
		: \
		: "ri" ((__e2k_u64_t) (val))); \
})

#define E2K_GET_SREG(reg_mnemonic) \
({ \
	register __e2k_u32_t res; \
	asm volatile ("rrs \t%%" #reg_mnemonic ", %0" \
		: "=r" (res)); \
	res; \
})

#define E2K_GET_DSREG(reg_mnemonic) \
({ \
	register __e2k_u64_t res; \
	asm volatile ("rrd \t%%" #reg_mnemonic ", %0" \
		: "=r" (res)); \
	res; \
})

#define E2K_SET_SREG(reg_mnemonic, val) \
({ \
	asm volatile ("rws \t%0, %%" #reg_mnemonic \
			: \
			: "ri" ((__e2k_u32_t) (val))); \
})

#define E2K_SET_DSREG(reg_mnemonic, val) \
({ \
	asm volatile ("rwd \t%0, %%" #reg_mnemonic \
			: \
			: "ri" ((__e2k_u64_t) (val))); \
})


#ifndef	__ASSEMBLY__

typedef unsigned long __e2k_syscall_arg_t;

#define E2K_SYSCALL_CLOBBERS \
		"ctpr1", "ctpr2", "ctpr3", \
		"b[0]", "b[1]", "b[2]", "b[3]", \
		"b[4]", "b[5]", "b[6]", "b[7]"

/* Transaction operation transaction of argument type
 * __e2k_syscall_arg_t */
#ifdef __ptr64__
#define __E2K_SYSCAL_ARG_ADD "addd,s"
#else
#define __E2K_SYSCAL_ARG_ADD "adds,s"
#endif

#define __E2K_SYSCALL_0(_trap, _sys_num, _arg1) \
({ \
	register __e2k_syscall_arg_t __res; \
	asm volatile ("{\n" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[sys_num], %%b[0]\n\t" \
		"sdisp %%ctpr1, %[trap]\n\t" \
		"}\n" \
		"call  %%ctpr1, wbs = %#\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %%b[0], %[res]" \
		: [res]     "=r" (__res) \
		: [trap]    "i"  ((int) (_trap)), \
		  [sys_num] "ri" ((__e2k_syscall_arg_t) (_sys_num)) \
		: E2K_SYSCALL_CLOBBERS); \
	__res; \
})

#define __E2K_SYSCALL_1(_trap, _sys_num, _arg1) \
({ \
	register __e2k_syscall_arg_t __res; \
	asm volatile ("{\n" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[sys_num], %%b[0]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg1], %%b[1]\n\t" \
		"sdisp %%ctpr1, %[trap]\n\t" \
		"}\n" \
		"call  %%ctpr1, wbs = %#\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %%b[0], %[res]" \
		: [res]     "=r" (__res) \
		: [trap]    "i"  ((int) (_trap)), \
		  [sys_num] "ri" ((__e2k_syscall_arg_t) (_sys_num)), \
		  [arg1]    "ri" ((__e2k_syscall_arg_t) (_arg1)) \
		: E2K_SYSCALL_CLOBBERS); \
	__res; \
})

#define __E2K_SYSCALL_2(_trap, _sys_num, _arg1, _arg2) \
({ \
	register __e2k_syscall_arg_t __res; \
	asm volatile ("{\n" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[sys_num], %%b[0]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg1], %%b[1]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg2], %%b[2]\n\t" \
		"sdisp %%ctpr1, %[trap]\n\t" \
		"}\n" \
		"call  %%ctpr1, wbs = %#\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %%b[0], %[res]" \
		: [res]     "=r" (__res) \
		: [trap]    "i"  ((int) (_trap)), \
		  [sys_num] "ri" ((__e2k_syscall_arg_t) (_sys_num)), \
		  [arg1]    "ri" ((__e2k_syscall_arg_t) (_arg1)), \
		  [arg2]    "ri" ((__e2k_syscall_arg_t) (_arg2)) \
		: E2K_SYSCALL_CLOBBERS); \
	__res; \
})

#define __E2K_SYSCALL_3(_trap, _sys_num, _arg1, _arg2, _arg3) \
({ \
	register __e2k_syscall_arg_t __res; \
	asm volatile ("{\n" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[sys_num], %%b[0]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg1], %%b[1]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg2], %%b[2]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg3], %%b[3]\n\t" \
		"sdisp %%ctpr1, %[trap]\n\t" \
		"}\n" \
		"call  %%ctpr1, wbs = %#\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %%b[0], %[res]" \
		: [res]     "=r" (__res) \
		: [trap]    "i"  ((int) (_trap)), \
		  [sys_num] "ri" ((__e2k_syscall_arg_t) (_sys_num)), \
		  [arg1]    "ri" ((__e2k_syscall_arg_t) (_arg1)), \
		  [arg2]    "ri" ((__e2k_syscall_arg_t) (_arg2)), \
		  [arg3]    "ri" ((__e2k_syscall_arg_t) (_arg3)) \
		: E2K_SYSCALL_CLOBBERS); \
	__res; \
})

#define __E2K_SYSCALL_4(_trap, _sys_num, _arg1, _arg2, _arg3, _arg4) \
({ \
	register __e2k_syscall_arg_t __res; \
	asm volatile ("{\n" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[sys_num], %%b[0]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg1], %%b[1]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg2], %%b[2]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg3], %%b[3]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg4], %%b[4]\n\t" \
		"sdisp %%ctpr1, %[trap]\n\t" \
		"}\n" \
		"call  %%ctpr1, wbs = %#\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %%b[0], %[res]" \
		: [res]     "=r" (__res) \
		: [trap]    "i"  ((int) (_trap)), \
		 [sys_num] "ri" ((__e2k_syscall_arg_t) (_sys_num)), \
		 [arg1]    "ri" ((__e2k_syscall_arg_t) (_arg1)), \
		 [arg2]    "ri" ((__e2k_syscall_arg_t) (_arg2)), \
		 [arg3]    "ri" ((__e2k_syscall_arg_t) (_arg3)), \
		 [arg4]    "ri" ((__e2k_syscall_arg_t) (_arg4)) \
		: E2K_SYSCALL_CLOBBERS); \
	__res; \
})

#define __E2K_SYSCALL_5(_trap, _sys_num, _arg1, _arg2, _arg3, _arg4, _arg5) \
({ \
	register __e2k_syscall_arg_t __res; \
	asm volatile ("{\n" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[sys_num], %%b[0]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg1], %%b[1]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg2], %%b[2]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg3], %%b[3]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg4], %%b[4]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg5], %%b[5]\n\t" \
		"sdisp %%ctpr1, %[trap]\n\t" \
		"}\n" \
		"call  %%ctpr1, wbs = %#\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %%b[0], %[res]" \
		: [res]     "=r" (__res) \
		: [trap]    "i"  ((int) (_trap)), \
		  [sys_num] "ri" ((__e2k_syscall_arg_t) (_sys_num)), \
		  [arg1]    "ri" ((__e2k_syscall_arg_t) (_arg1)), \
		  [arg2]    "ri" ((__e2k_syscall_arg_t) (_arg2)), \
		  [arg3]    "ri" ((__e2k_syscall_arg_t) (_arg3)), \
		  [arg4]    "ri" ((__e2k_syscall_arg_t) (_arg4)), \
		  [arg5]    "ri" ((__e2k_syscall_arg_t) (_arg5)) \
		: E2K_SYSCALL_CLOBBERS); \
	__res; \
})

#define __E2K_SYSCALL_6(_trap, _sys_num, _arg1, \
			_arg2, _arg3, _arg4, _arg5, _arg6) \
({ \
	register __e2k_syscall_arg_t __res; \
	asm volatile ("{\n" \
		"sdisp %%ctpr1, %[trap]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[sys_num], %%b[0]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg1], %%b[1]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg2], %%b[2]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg3], %%b[3]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg4], %%b[4]\n\t" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg5], %%b[5]\n\t" \
		"}\n" \
		"{\n" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %[arg6], %%b[6]\n\t" \
		"call  %%ctpr1, wbs = %#\n\t" \
		"}\n" \
		__E2K_SYSCAL_ARG_ADD "  0x0, %%b[0], %[res]" \
		: [res]     "=r" (__res) \
		: [trap]    "i"  ((int) (_trap)), \
		  [sys_num] "ri" ((__e2k_syscall_arg_t) (_sys_num)), \
		  [arg1]    "ri" ((__e2k_syscall_arg_t) (_arg1)), \
		  [arg2]    "ri" ((__e2k_syscall_arg_t) (_arg2)), \
		  [arg3]    "ri" ((__e2k_syscall_arg_t) (_arg3)), \
		  [arg4]    "ri" ((__e2k_syscall_arg_t) (_arg4)), \
		  [arg5]    "ri" ((__e2k_syscall_arg_t) (_arg5)), \
		  [arg6]    "ri" ((__e2k_syscall_arg_t) (_arg6)) \
		: E2K_SYSCALL_CLOBBERS); \
	__res; \
})

#define E2K_SYSCALL(trap, sys_num, num_args, args...) \
	__E2K_SYSCALL_##num_args(trap, sys_num, args)

#endif /* !__ASSEMBLY__ */


#endif /* _UAPI_E2K_API_H_ */
