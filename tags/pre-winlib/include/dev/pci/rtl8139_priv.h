/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _RTL8139_PRIV_H
#define _RTL8139_PRIV_H

#include <phantom_types.h>


typedef struct rtl8139 {
	int 		irq;
	physaddr_t 	phys_base;
	size_t 		phys_size;

	void * 		virt_base;

	u_int16_t 	io_port;

	u_int8_t 	mac_addr[6];

	int			softirq;
	int			rx_rq; // triggered in interrupt, processed in softint
	int			tx_rq; // triggered in interrupt, processed in softint

	int 		txbn;
	int 		last_txbn;

	void * 		rxbuf;
    hal_sem_t 	rx_sem;

	void * 		txbuf;
    hal_sem_t 	tx_sem;

	hal_mutex_t 	lock;
	hal_spinlock_t 	reg_spinlock;
} rtl8139;


//int rtl8139_detect(rtl8139 *nic);
int rtl8139_init(rtl8139 *nic);
void rtl8139_xmit(rtl8139 *nic, const char *ptr, int32_t len);
int32_t rtl8139_rx(rtl8139 *nic, char *buf, int32_t buf_len);


#endif
