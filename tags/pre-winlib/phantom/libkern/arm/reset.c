#include <kernel/init.h>
#include <hal.h>


void
hal_cpu_reset_real(void) //__attribute__((noreturn))
{
    hal_cli();

	// TODO write me

    /* NOTREACHED */
    while(1)
		;
}



