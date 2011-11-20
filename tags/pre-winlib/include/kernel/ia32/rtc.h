
#include <phantom_types.h>
#include <time.h>

//int rtcin(int reg);
unsigned int isa_rtc_read_reg(int reg);

//void rtcout(unsigned char reg, unsigned char val);
void isa_rtc_write_reg(int reg, unsigned char val);


void isa_rtc_nmi_on(void);
void isa_rtc_nmi_off(void);




bigtime_t arch_get_rtc_delta(void);
// read rtc as struct tm
void rtc_read_tm( struct tm *out );


