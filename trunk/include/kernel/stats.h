#ifndef STATS_H
#define STATS_H

#define MAX_STAT_COUNTERS 128


//! Counts value during one second
extern int			*stat_sec_counters;



#define		STAT_CNT_INTERRUPT 			0
#define		STAT_CNT_BLOCK_IO  			1
#define		STAT_CNT_PAGEIN    			2
#define		STAT_CNT_PAGEOUT   			3
#define		STAT_CNT_SNAPSHOT  			4
#define		STAT_CNT_SOFTINT 			5
#define		STAT_CNT_TCP_RX 			6
#define		STAT_CNT_TCP_TX 			7
#define		STAT_CNT_UDP_RX 			8
#define		STAT_CNT_UDP_TX 			9

void stat_increment_counter( int nCounter );

#define STAT_INC_CNT( ___nCounter ) do { \
	assert( ___nCounter < MAX_STAT_COUNTERS ); \
	stat_sec_counters[___nCounter]++; \
	} while(0)




#endif // STATS_H
