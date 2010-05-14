
#include <phantom_types.h>
#include <time.h>

int rtcin(int reg);
void rtcout(unsigned char reg, unsigned char val);


bigtime_t arch_get_rtc_delta(void);
// read rtc as struct tm
void rtc_read_tm( struct tm *out );


