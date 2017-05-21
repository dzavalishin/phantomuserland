#ifndef _UAPI_E2K_TYPES_H_
#define _UAPI_E2K_TYPES_H_

#include <asm-generic/types.h>

/*
 * This file is never included by application software unless
 * explicitly requested (e.g., via linux/types.h) in which case the
 * application is Linux specific so (user-) name space pollution is
 * not a major issue.  However, for interoperability, libraries still
 * need to be careful to avoid a name clashes.
 */

#ifndef __ASSEMBLY__

typedef	unsigned long	e2k_addr_t;	/* phys & virt address (64 bits) */
typedef	unsigned long	e2k_size_t;	/* size of objects (64 bits) */
		/* what should it be ????? */

/*
 * __xx is ok: it doesn't pollute the POSIX namespace. Use these in the
 * header files exported to user space
 */


#endif	/* !(__ASSEMBLY__) */

#endif /* _UAPI_E2K_TYPES_H_ */
