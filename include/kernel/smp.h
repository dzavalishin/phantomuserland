/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Multiprocessor support.
 *
 *
**/

#ifndef SMP_H
#define SMP_H

//#if HAVE_SMP
#  define MAX_CPUS	16
//#else
//#  define MAX_CPUS	1
//#endif // HAVE_SMP


#ifndef ASSEMBLER

// Actually starts secondary CPUs
int	imps_probe(void);


int GET_CPU_ID(void) __attribute__((no_instrument_function));
void phantom_import_cpu_thread(int ncpu);
void phantom_load_cpu_tss(int ncpu);

// intercpu messages
enum {
	SMP_MSG_INVL_PAGE_RANGE = 0,
	SMP_MSG_INVL_PAGE_LIST,
	SMP_MSG_GLOBAL_INVL_PAGE,
	SMP_MSG_RESCHEDULE,
	SMP_MSG_CPU_HALT,
	SMP_MSG_1,
};


enum {
	SMP_MSG_FLAG_ASYNC = 0,
	SMP_MSG_FLAG_SYNC,
};


int is_smp(void);
int ncpus(void);


#endif // !ASSEMBLER



#endif // SMP_H
