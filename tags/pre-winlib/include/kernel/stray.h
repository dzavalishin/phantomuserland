#ifndef KERNEL_STRAY_H
#define KERNEL_STRAY_H

#include <string.h>

#define STRAY_CATCH_SIZE (128)

void phantom_debug_register_stray_catch( void *buf, int bufs, const char *name );

// define in each source an array of zeroes to try to catch stray pointers.
// even if we can't really catch 'em, do it so that enabling and disabling
// these arrays will, possibly, affect bugs and, if so, show us that bug is
// caused by stray pointer.
static char stray_pointer_catch_bss[STRAY_CATCH_SIZE];
static char stray_pointer_catch_data[STRAY_CATCH_SIZE] = "0000"; // enforce it to be in data seg

static void register_stray_catch_buf(void) __attribute__ ((constructor));
static void register_stray_catch_buf(void) 
{
    memset( stray_pointer_catch_data+4, 0, STRAY_CATCH_SIZE-4 );
	
    phantom_debug_register_stray_catch( stray_pointer_catch_bss, STRAY_CATCH_SIZE, " bss" );
    phantom_debug_register_stray_catch( stray_pointer_catch_data, STRAY_CATCH_SIZE, " data" );
}

#endif // KERNEL_STRAY_H

