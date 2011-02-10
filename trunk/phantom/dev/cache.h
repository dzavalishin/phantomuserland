/*!     \file include/arch/cache.h
 *      \brief Cache parameters.
 *      \author Andrea Righi <drizzt@inwind.it>
 *      \date Last update: 2004-01-12
 *      \note Copyright (&copy;) 2003 Andrea Righi
 */

#ifndef CACHE_H
#define CACHE_H

//#include <kernel/config.h>

#define CONFIG_X86_L1_CACHE_SHIFT       (5)

//! The L1 cache line size (how many bits are used).
#ifdef CONFIG_M386
#define CONFIG_X86_L1_CACHE_SHIFT       (4)
#endif
#ifdef CONFIG_M486
#define CONFIG_X86_L1_CACHE_SHIFT       (4)
#endif
#ifdef CONFIG_M486
#define CONFIG_X86_L1_CACHE_SHIFT       (4)
#endif
#ifdef CONFIG_M586
#define CONFIG_X86_L1_CACHE_SHIFT       (5)
#endif
#ifdef CONFIG_M586TSC
#define CONFIG_X86_L1_CACHE_SHIFT       (5)
#endif
#ifdef CONFIG_M586MMX
#define CONFIG_X86_L1_CACHE_SHIFT       (5)
#endif
#ifdef CONFIG_M686
#define CONFIG_X86_L1_CACHE_SHIFT       (5)
#endif
#ifdef CONFIG_MPENTIUMII
#define CONFIG_X86_L1_CACHE_SHIFT       (5)
#endif
#ifdef CONFIG_MPENTIUMIII
#define CONFIG_X86_L1_CACHE_SHIFT       (5)
#endif
#ifdef CONFIG_MPENTIUM4
#define CONFIG_X86_L1_CACHE_SHIFT       (7)
#endif
#ifdef CONFIG_MK6
#define CONFIG_X86_L1_CACHE_SHIFT       (5)
#endif
#ifdef CONFIG_MK7
#define CONFIG_X86_L1_CACHE_SHIFT       (6)
#endif
#ifdef CONFIG_MATHLONXP
#define CONFIG_X86_L1_CACHE_SHIFT       (6)
#endif

//! The L1 cache line size (how many bits are used).
#define L1_CACHE_SHIFT                  CONFIG_X86_L1_CACHE_SHIFT
//! The L1 cache line size (in bytes).
#define L1_CACHE_BYTES                  (1 << L1_CACHE_SHIFT)

#endif
