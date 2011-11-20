
// called from libc. TODO fix it there!
void hal_disable_preemption() {}
void hal_enable_preemption() {}

// TODO userland stray catch?
void phantom_debug_register_stray_catch( void *a, int s, const char*n )
{
    // Ignore
    (void) a;
    (void) s;
    (void) n;
}
