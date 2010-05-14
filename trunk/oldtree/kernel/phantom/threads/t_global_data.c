/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Global vars.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include "thread_private.h"


phantom_thread_t *   percpu_current_thread[1];
phantom_thread_t *   percpu_idlest_thread[1];

// indexed by tid, val 0 = no such thread
phantom_thread_t *phantom_kernel_threads[MAX_THREADS];

