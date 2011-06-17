#include <sys/cdefs.h>
#include <sys/libkern.h>

/*
 * Find First Reset bit
 */
int
ffr(int mask)
{
	int bit;

	if (mask == 0)
		return 1;

	if (mask == ~0)
		return 0;

	for (bit = 1; (mask & 1); bit++)
		mask = (unsigned int)mask >> 1;
	return (bit);
}
