#ifndef TIME_H
#define TIME_H

#include <phantom_types.h>
#include <phantom_time.h>

// microseconds!
bigtime_t 		hal_system_time(void); // uptime
bigtime_t 		hal_local_time(void);  // real time/date

extern int (*arch_get_tick_rate)(void);
extern bigtime_t (*arch_get_time_delta)(void);

// seconds
time_t 			time(time_t *timer);

#define phantom_spinwait_msec phantom_spinwait

/* wait by spinning: number of milliseconds to delay */
void 			phantom_spinwait(int millis);

// Wait by spinning, ~10 microsec
void			tenmicrosec(void); 

// Updated by kernel time code, readonly. Read by copying pointer
// and reading then. Updated by pointer change once a second.
//
// Example: struct tm *tmp, myTime; tmp = current_time; myTime = *tmp;
//
struct tm *		current_time;

// Called once a second to update per second ststistics
void stat_update_second_stats(void);

#include <vm/internal_da.h>
void phantom_wakeup_after_msec(int msec, struct data_area_4_thread *tc);


#endif // TIME_H

