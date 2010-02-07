#ifndef TIME_H
#define TIME_H

#include <phantom_types.h>
#include <phantom_time.h>

// microseconds!
bigtime_t 		hal_system_time(void); // uptime
bigtime_t 		hal_local_time(void);  // real time/date

// seconds
time_t 			time(time_t *timer);

#define phantom_spinwait_msec phantom_spinwait

/* wait by spinning: number of milliseconds to delay */
void 			phantom_spinwait(int millis);

// Updated by kernel time code, readonly. Read by copying pointer
// and reading then. Updated by pointer change once a second.
//
// Example: struct tm *tmp, myTime; tmp = current_time; myTime = *tmp;
//
struct tm *		current_time;


#endif // TIME_H

