/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * ia32 timer code. 
 *
**/


#include <hal.h>
#include <ia32/proc_reg.h>


// returns time in CPU/arch dependent ticks, on ia32 it is RDTSC instruction

u_int64_t
hal_get_interrupt_profiling_time() 
{
    return rdtsc();
}

