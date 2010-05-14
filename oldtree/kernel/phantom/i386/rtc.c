// From FreeBSD clock.c

#include <x86/isa.h>
#include <i386/pio.h>


#include <phantom_libc.h>
#include <hal.h>
#include <time.h>

#include "rtc.h"

// TODO header
// TODO hal spinlocks

#define RTC_LOCK
#define RTC_UNLOCK


static	int	rtc_reg = -1;


/*
 * RTC support routines
 */

int
rtcin(int reg)
{
	u_char val;

	RTC_LOCK;
	if (rtc_reg != reg) {
		inb(0x84);
		outb(IO_RTC, reg);
		rtc_reg = reg;
		inb(0x84);
	}
	val = inb(IO_RTC + 1);
	RTC_UNLOCK;
	return (val);
}

void
writertc(int reg, u_char val)
{

	RTC_LOCK;
	if (rtc_reg != reg) {
		inb(0x84);
		outb(IO_RTC, reg);
		rtc_reg = reg;
		inb(0x84);
	}
	outb(IO_RTC + 1, val);
	inb(0x84);
	RTC_UNLOCK;
}

// satisfy old code
void
rtcout(unsigned char reg, unsigned char val)
{
    writertc( reg, val );
}


static __inline int
readrtc(int port)
{
    return(bcd2bin(rtcin(port)));
}




enum {
    RTC_SEC   = 0,
    RTC_SECALRM	= 1,	/* seconds alarm */
    RTC_MIN   = 2,
    RTC_MINALRM	= 3,	/* minutes alarm */
    RTC_HOUR  = 4,
    RTC_HRSALRM	= 5,	/* hours alarm */
    RTC_WDAY	= 6,	/* week day */
    RTC_DAY   = 7,
    RTC_MONTH = 8,
    RTC_YEAR  = 9,

    RTC_BASELO	= 0x15,	/* low byte of basemem size */
    RTC_BASEHI	= 0x16,	/* high byte of basemem size */
    RTC_EXTLO	= 0x17,	/* low byte of extended mem size */
    RTC_EXTHI	= 0x18,	/* low byte of extended mem size */

    RTC_CENTURY = 0x32
};


const bigtime_t usecs_per_day = 86400000000LL;
const int month_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const int months[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

static int is_leap_year(int year)
{
    if(year % 4 == 0 && year % 400 != 0) {
        return 1;
    }
    return 0;
}


bigtime_t arch_get_rtc_delta(void)
{
    int oldsec, sec, min, hour;
    int day, month, year, century;
    bigtime_t val;

retry:
    min = readrtc(RTC_MIN);
    if(min < 0 || min > 59)
        min = 0;
    hour = readrtc(RTC_HOUR);
    if(hour < 0 || hour > 23)
        hour = 0;
    month = readrtc(RTC_MONTH);
    if(month < 1 || month > 12)
        month = 1;
    day = readrtc(RTC_DAY);
    if(day < 1 || day > month_days[month-1])
        day = 1;
    year = readrtc(RTC_YEAR);
    century = readrtc(RTC_CENTURY);
    if(century > 0) {
        year += century * 100;
    } else {
        if(year < 80)
            year += 2000;
        else
            year += 1900;
    }

    // keep reading the second counter until it changes
    oldsec = readrtc(RTC_SEC);
    while((sec = readrtc(RTC_SEC)) == oldsec)
        ;
    if(sec == 0) {
        // we just wrapped around, and potentially changed
        // all of the other counters, retry everything
        goto retry;
    }

    // convert these values into usecs since Jan 1, 1 AD
    val = (365 * (year - 1) - (year / 4) + (year / 400)) * usecs_per_day;
    val += ((day - 1) + months[month - 1] + ((month > 2) ? is_leap_year(year) : 0)) * usecs_per_day;
    val += (((hour * 60) + min) * 60 + sec) * 1000000;

    printf("arch_get_rtc_delta: GMT %d:%d:%d %d.%d.%d\n",
           hour, min, sec, day, month, year );

    return val;
}


void rtc_read_tm( struct tm *out )
{
retry:
    out->tm_min = readrtc(RTC_MIN);
    out->tm_hour = readrtc(RTC_HOUR);
    out->tm_mon = readrtc(RTC_MONTH);
    out->tm_mday = readrtc(RTC_DAY);

    int year = readrtc(RTC_YEAR);
    int century = readrtc(RTC_CENTURY);
    if(century > 0) {
        year += century * 100;
    } else {
        if(year < 80)
            year += 2000;
        else
            year += 1900;
    }

    out->tm_year = year - 1900;

    out->tm_yday = 0; // TODO calc me

    out->tm_wday = readrtc(RTC_WDAY);

    // keep reading the second counter until it changes
    int sec;
    int oldsec = readrtc(RTC_SEC);
    while((sec = readrtc(RTC_SEC)) == oldsec)
        ;
    if(sec == 0) {
        // we just wrapped around, and potentially changed
        // all of the other counters, retry everything
        goto retry;
    }
    out->tm_sec=sec;
}

