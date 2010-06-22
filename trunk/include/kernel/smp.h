#ifndef SMP_H
#define SMP_H

#define MAX_CPUS	16


#ifndef ASSEMBLER

// Actually starts secondary CPUs
int	imps_probe(void);


int GET_CPU_ID(void);
void phantom_import_cpu_thread(int ncpu);
void phantom_load_cpu_tss(int ncpu);


#endif // !ASSEMBLER



#endif // SMP_H
