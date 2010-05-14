#ifndef HAL_PRIVATE_H
#define HAL_PRIVATE_H

#include <hal.h>
#include <kernel/vm.h>


struct hal_sem
{
    volatile int 	posted;
    hal_cond_t      	c;
    hal_mutex_t 	m;
};



#endif // HAL_PRIVATE_H


