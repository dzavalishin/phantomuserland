#ifndef _E2K_SECTIONS_H
#define _E2K_SECTIONS_H

/* nothing to see, move along */
#ifndef __ASSEMBLY__
//#include <asm-generic/sections.h>
#endif	/* ! __ASSEMBLY__ */

#if (defined __e2k__) && (defined __LCC__)
#define __interrupt     __attribute__((__check_stack__))
#else
#define __interrupt     __attribute__((__interrupt__))
#endif

#ifndef	CONFIG_RECOVERY
#define	__init_recv			__init
#define	__initdata_recv			__initdata
#else
#define	__init_recv
#define	__initdata_recv
#endif	/* ! (CONFIG_RECOVERY) */

#if !defined(CONFIG_RECOVERY) && !defined(CONFIG_SERIAL_PRINTK) && \
	!defined(CONFIG_LMS_CONSOLE)
#define	__init_cons			__init
#else
#define	__init_cons
#endif	/* boot console used after init completion */

#ifndef __ASSEMBLY__
extern char	_start[];
extern char	__init_text_begin[], __init_text_end[];
extern char	__init_data_begin[], __init_data_end[];
extern char	__node_data_start[], __node_data_end[];
extern char	_ptext_start[], _ptext_end[];
#endif	/* ! __ASSEMBLY__ */

#ifdef	CONFIG_KERNEL_CODE_CONTEXT
#define __protect		__section(.protect.text)
#define __protectdata		__section(.protect.data)
/* For assembly routines */
#define __PROTECTED_TEXT	.section ".protect.text", "ax"
#define __PROTECTED_DATA	.section ".protect.data", "aw"
#else	/* ! CONFIG_KERNEL_CODE_CONTEXT */
#define __protect
#define __protectdata
/* For assembly routines */
#define __PROTECTED_TEXT	.section ".text", "ax"
#define __PROTECTED_DATA	.section ".data", "aw"
#endif	/* CONFIG_KERNEL_CODE_CONTEXT */

#ifdef	CONFIG_NUMA
#define __nodedata	__section(.node.data)
#define __NODEDATA	.section	".node.data","aw"
#else	/* ! CONFIG_NUMA */
#define __nodedata	__section(data)
#define __NODEDATA	.section	"data"
#endif	/* CONFIG_NUMA */

#endif	/* _E2K_SECTIONS_H */
