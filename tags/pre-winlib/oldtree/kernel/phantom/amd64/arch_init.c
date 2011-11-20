/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * machdep cpu/system init
 *
**/

#define DEBUG_MSG_PREFIX "arch.init"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/init.h>

void arch_init_early(void)
{
#if 0
    // TODO Enable superpage support if we have it.
    if (cpu.feature_flags & CPUF_4MB_PAGES)
    {
        set_cr4(get_cr4() | CR4_PSE);
    }
#endif
}

void arch_threads_init()
{
}
