/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_CBUF_H
#define _KERNEL_CBUF_H

#include <phantom_libc.h>
#include <phantom_types.h>

#include "hal.h"


#define CBUF_LEN 2048

#define CBUF_FLAG_CHAIN_HEAD 1
#define CBUF_FLAG_CHAIN_TAIL 2

typedef struct cbuf {
	struct cbuf *next;
	size_t len;
	size_t total_len;
	void *data;
	int flags;

	/* used by the network stack to chain a list of these together */
	struct cbuf *packet_next;

	char dat[CBUF_LEN - 2*sizeof(struct cbuf *) - 2*sizeof(size_t) - sizeof(void *) - sizeof(int)];
} cbuf;

//int cbuf_init(void);
cbuf *cbuf_get_chain(size_t len);
cbuf *cbuf_get_chain_noblock(size_t len);
void cbuf_free_chain_noblock(cbuf *buf);
void cbuf_free_chain(cbuf *buf);

size_t cbuf_get_len(cbuf *buf);
void *cbuf_get_ptr(cbuf *buf, size_t offset);
int cbuf_is_contig_region(cbuf *buf, size_t start, size_t end);

int cbuf_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len);
int cbuf_memcpy_from_chain(void *dest, cbuf *chain, size_t offset, size_t len);

//int cbuf_user_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len);
//int cbuf_user_memcpy_from_chain(void *dest, cbuf *chain, size_t offset, size_t len);

u_int16_t cbuf_ones_cksum16(cbuf *chain, size_t offset, size_t len);
u_int16_t cbuf_ones_cksum16_2(cbuf *chain, size_t offset, size_t len, void *buf, size_t buf_len);

cbuf *cbuf_merge_chains(cbuf *chain1, cbuf *chain2);
cbuf *cbuf_duplicate_chain(cbuf *chain, size_t offset, size_t len, size_t leading_space);

cbuf *cbuf_truncate_head(cbuf *chain, size_t trunc_bytes, bool free_unused);
int cbuf_truncate_tail(cbuf *chain, size_t trunc_bytes, bool free_unused);

int cbuf_extend_head(cbuf **chain, size_t extend_bytes);
int cbuf_extend_tail(cbuf *chain, size_t extend_bytes);

void cbuf_test(void);



u_int16_t ones_sum16(u_int32_t sum, const void *_buf, int len);
u_int16_t cksum16(void *_buf, int len);
u_int16_t cksum16_2(void *buf1, int len1, void *buf2, int len2);



#endif

