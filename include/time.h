/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Time stuff. TODO move to kernel/.
 *
 *
**/

#ifndef TIME_H
#define TIME_H

#include <phantom_types.h>
#include <phantom_time.h>

#define _DAY_SEC           (24UL * 60UL * 60UL) /* secs in a day */
#define _YEAR_SEC          (365L * _DAY_SEC)    /* secs in a year */
#define _FOUR_YEAR_SEC     (1461L * _DAY_SEC)   /* secs in a 4 year interval */
#define _DEC_SEC           315532800UL  /* secs in 1970-1979 */
//#define _BASE_YEAR         70L  /* 1970 is the base year */
#define _BASE_DOW          4    /* 01-01-70 was a Thursday */
#define _LEAP_YEAR_ADJUST  17L  /* Leap years 1900 - 1970 */
//#define _MAX_YEAR          138L /* 2038 is the max year */

#define SECSPERMIN	60
#define MINSPERHOUR	60
#define HOURSPERDAY	24
#define DAYSPERWEEK	7
#define DAYSPERNYEAR	365
#define DAYSPERLYEAR	366
#define SECSPERHOUR	(SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	((long) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR	12

#define TM_YEAR_BASE	1900

#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))


// TODO check against POSIX
struct timespec
{
	long		seconds;
	long		nanoseconds;
};

// microseconds!
bigtime_t 		hal_system_time(void); // uptime
bigtime_t 		hal_local_time(void);  // real time/date

extern int (*arch_get_tick_rate)(void);
extern bigtime_t (*arch_get_time_delta)(void);

// seconds
time_t 			time(time_t *timer);
void 			set_time(time_t time);

//! Fast, but less accurate time, sec
time_t          fast_time(void);

//! Uptime in seconds
time_t          uptime(void);


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

// Called once a second to update per second statistics
void stat_update_second_stats(void);

// -----------------------------------------------------------------------
// Polled timeouts for drivers, interrupts must be enabled

typedef bigtime_t polled_timeout_t;

// Set timeout length
void set_polled_timeout( polled_timeout_t *timer, bigtime_t timeout_uSec );
// Returns true if timeout time passed
bool check_polled_timeout( polled_timeout_t *timer );


#include <vm/internal_da.h>
void phantom_wakeup_after_msec(int msec, struct data_area_4_thread *tc);


#endif // TIME_H

