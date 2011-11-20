#warning who uses me?
/* 
 * Copyright (c) 1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *      Author: Bryan Ford, University of Utah CSL
 */
#ifndef _IMPS_CPU_NUMBER_
#define _IMPS_CPU_NUMBER_

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif


#ifndef ASSEMBLER

#include <kernel/ia32/apic.h>

static inline int
cpu_number()
{
	return apic_local_unit.unit_id.r >> 24;
}

#else ASSEMBLER

//#include "impsasm.h"

#define	CPU_NUMBER(reg)		\
	movzbl	APIC_LOCAL_VA+APIC_LOCAL_UNIT_ID+3,reg

#endif ASSEMBLER


//#include "i386/cpu_number.h"


#endif _IMPS_CPU_NUMBER_
