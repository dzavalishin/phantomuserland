#ifndef TIME_H
#define TIME_H

#include <phantom_types.h>
#include <phantom_time.h>

// microseconds!
bigtime_t 		hal_system_time(void); // uptime
bigtime_t 		hal_local_time(void);  // real time/date

// seconds
time_t 			time(time_t *timer);

/* wait by spinning: number of milliseconds to delay */
void 			phantom_spinwait(int millis);


#endif // TIME_H

