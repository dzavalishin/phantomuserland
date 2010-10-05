#include <assert.h>

void __stack_chk_fail(void)
{
	panic("__stack_chk_fail called");
}
