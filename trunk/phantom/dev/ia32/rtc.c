/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * PC real time clock interface. Time of day and date.
 *
 *
 **/


#define DEBUG_MSG_PREFIX "rtc-tm"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

// From FreeBSD clock.c

#include <ia32/pc/isa.h>
#include <ia32/pio.h>


#include <phantom_libc.h>
#include <hal.h>
#include <time.h>

#include <kernel/ia32/rtc.h>
#include <kernel/ia32/rtc_regs.h>

// TODO header
// TODO hal spinlocks

#define RTC_LOCK      int ie = hal_save_cli()
#define RTC_UNLOCK    if(ie) hal_sti()


//static	int	rtc_reg = -1;
static unsigned int nmi_flag = 0; // high bit == 1 to off NMI

#define SPEND_TIME inb(0x84)


/*
 * RTC support routines
 */

unsigned int isa_rtc_read_reg(int reg)
{
    u_char val;

    RTC_LOCK;
    //if (rtc_reg != reg)
    {
        SPEND_TIME;
        outb(IO_RTC, reg|nmi_flag);
        //rtc_reg = reg;
        SPEND_TIME;
    }
    val = inb(IO_RTC + 1);
    RTC_UNLOCK;
    return (val);
}

void isa_rtc_write_reg(int reg, u_char val)
{

    RTC_LOCK;
    //if (rtc_reg != reg)
    {
        SPEND_TIME;
        outb(IO_RTC, reg|nmi_flag);
        //rtc_reg = reg;
        SPEND_TIME;
    }
    outb(IO_RTC + 1, val);
    SPEND_TIME;
    RTC_UNLOCK;
}


void isa_rtc_nmi_off(void)
{
    // We turn off NMI and reselect reg D as recommended
    nmi_flag = RTC_NMI_DIS;
    isa_rtc_read_reg(0x0D);
}

void isa_rtc_nmi_on(void)
{
    // We turn on NMI and reselect reg D as recommended
    nmi_flag = RTC_NMI_ENA;
    isa_rtc_read_reg(0x0D);
}




/*
// satisfy old code
void rtcout(unsigned char reg, unsigned char val)
{
    isa_rtc_write_reg( reg, val );
}
*/



static __inline int
readrtc(int port)
{
    return(bcd2bin(isa_rtc_read_reg(port)));
}


static void check_update_in_progress(void)
{
    int ntries = 1000;
    static int say_times = 10;

    // Check if update is in progress
    while( (isa_rtc_read_reg(0x0A) & RTC_A_UIP) && (ntries-- > 0) )
        SPEND_TIME;

    if( (ntries <= 0) && (say_times-- > 0) )
        printf("RTC read - timed out waiting for update end\n");
}






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
    //int oldsec
    int sec, min, hour;
    int day, month, year, century;
    bigtime_t val;

    isa_rtc_nmi_off();

    check_update_in_progress();
//retry:
    min = readrtc(RTC_MIN);
    if(min < 0 || min > 59)        min = 0;
    hour = readrtc(RTC_HOUR);
    if(hour < 0 || hour > 23)      hour = 0;
    month = readrtc(RTC_MONTH);
    if(month < 1 || month > 12)    month = 1;

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

    /*
    // keep reading the second counter until it changes
    oldsec = readrtc(RTC_SEC);
    while((sec = readrtc(RTC_SEC)) == oldsec)
        ;
    if(sec == 0) {
        // we just wrapped around, and potentially changed
        // all of the other counters, retry everything
        goto retry;
    }*/

    sec = readrtc(RTC_SEC);

    isa_rtc_nmi_on();

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
    isa_rtc_nmi_off();

    check_update_in_progress();

//retry:
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

    /*
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
    */

    out->tm_sec = readrtc(RTC_SEC);

    isa_rtc_nmi_on();
}

