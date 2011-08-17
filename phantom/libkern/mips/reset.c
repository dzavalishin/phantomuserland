#include <kernel/init.h>
#include <hal.h>


void
hal_cpu_reset_real(void) //__attribute__((noreturn))
{
#warning CPU reset?
    hal_cli();

	// TODO write me

    while(1)
		;
}



