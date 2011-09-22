/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_NET_NET_TIMER_H
#define _NEWOS_KERNEL_NET_NET_TIMER_H

#include <phantom_libc.h>

#include <hal.h>
//#include "newos.h"
//#include <compat/newos.h>

typedef void (*net_timer_callback)(void *);

typedef struct net_timer_event {
	struct net_timer_event *next;
	struct net_timer_event *prev;

	net_timer_callback func;
	void *args;

	bigtime_t sched_time;

	bool pending;
} net_timer_event;

#define NET_TIMER_PENDING_IGNORE 0x1

//int net_timer_init(void);

int set_net_timer(net_timer_event *e, unsigned int delay_ms, net_timer_callback callback, void *args, int flags);
int cancel_net_timer(net_timer_event *e);

void clear_net_timer(net_timer_event *e);
extern inline void clear_net_timer(net_timer_event *e)
{
	e->prev = e->next = NULL;
	e->pending = 0;
}

#endif

