// This is a fake .so library defining kernel enter points to link
// with. Actual library is kernel itself.

void kernel_so_lputs( const char *s )
{
	// contents are ignored in any case
}

void kernel_so_mutex_init( void * )
{
	// contents are ignored in any case
}