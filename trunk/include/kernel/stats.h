/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * In-kernel stat counters.
 *
 *
**/

#ifndef STATS_H
#define STATS_H

#include <errno.h>

#define KERNEL_STATS_MAX_NAME 64

struct kernel_stats
{
    char                name[KERNEL_STATS_MAX_NAME];

    int                 current_per_second;
    int                 average_per_second;
    int                 total;
};


#ifdef KERNEL

#define MAX_STAT_COUNTERS 128


//! Counts value during one second
extern int			*stat_sec_counters;

errno_t get_stats_record( int id, struct kernel_stats *out );



#define     STAT_CNT_INTERRUPT 			0
#define     STAT_CNT_BLOCK_IO  			1
#define     STAT_CNT_PAGEIN    			2
#define     STAT_CNT_PAGEOUT   			3
#define     STAT_CNT_SNAPSHOT  			4
#define     STAT_CNT_SOFTINT 			5
#define     STAT_CNT_TCP_RX 			6
#define     STAT_CNT_TCP_TX 			7
#define     STAT_CNT_UDP_RX 			8
#define     STAT_CNT_UDP_TX 			9

#define     STAT_CNT_PMEM_ALLOC			10
#define     STAT_CNT_PMEM_FREE			11
#define     STAT_CNT_VA_ALLOC			12
#define     STAT_CNT_VA_FREE			13
#define     STAT_CNT_LOMEM_ALLOC                    14
#define     STAT_CNT_LOMEM_FREE			15

#define     STAT_CNT_DNS_REQ			16
#define     STAT_CNT_DNS_ANS			17

#define     STAT_CNT_THREAD_SW			18
#define     STAT_CNT_THREAD_SAME                    19

#define	    STAT_CNT_WIRE                           20
#define     STAT_CNT_WIRE_PAGEIN                    21

#define     STAT_CNT_DISK_Q_SIZE                    22
#define     STAT_CNT_BLOCK_SYNC_IO                  23

#define     STAT_skip1                              24
#define     STAT_skip2                              25
#define     STAT_skip3                              26
#define     STAT_skip4                              27
#define     STAT_skip5                              28
#define     STAT_skip6                              29
#define     STAT_skip7                              30
#define     STAT_skip8                              31


#define     DEFERRED_REFDEC_RUNS                    32
#define     DEFERRED_REFDEC_REQS                    33
#define     DEFERRED_REFDEC_LOST                    34
#define     OBJECT_ALLOC                            35
#define     OBJECT_FREE                             36
#define     OBJECT_SATURATE                         37


void stat_increment_counter( int nCounter );

#define STAT_INC_CNT( ___nCounter ) do { \
	assert( ___nCounter < MAX_STAT_COUNTERS ); \
	stat_sec_counters[___nCounter]++; \
	} while(0)


#define STAT_INC_CNT_N( ___nCounter, ___val ) do { \
	assert( ___nCounter < MAX_STAT_COUNTERS ); \
	stat_sec_counters[___nCounter] += ___val; \
	} while(0)

#endif // KERNEL


#endif // STATS_H
