#if 1

// redo with connections

/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#include "vm/object.h"
#include "vm/internal.h"
#include "vm/internal_da.h"
#include "vm/syscall.h"
#include "vm/root.h"
#include "vm/p2c.h"


int si_tcp_ready_to_send(struct data_area_4_tcp *da);
int si_tcp_ready_to_recv(struct data_area_4_tcp *da);


#endif
