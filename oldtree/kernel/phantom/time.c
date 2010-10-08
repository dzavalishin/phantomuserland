/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Time keeping.
 *
**/


#include "hal.h"
#include "newos.h"
#include "misc.h"

#include "i386/rtc.h"

#include <time.h>
#include <string.h>

#include <kernel/timedcall.h>


static void update_tm();


const char *monNames[12] =
{
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
};

// microseconds!

static hal_spinlock_t sys_time_spinlock;

// The 'rough' counter of system up time, accurate to the rate at
// which the timer interrupts run (see TICK_RATE in timer.c).
static bigtime_t sys_time;

// The latest accurate system time, accurate to the resolution
// of the underlying hardware timer
static bigtime_t sys_time_accurate;

// The delta of usecs that needs to be added to sys_time to get real time
static bigtime_t real_time_delta;

// Any additional delta that might need to be added to the above two values
// to get real time. (in case the RTC isn't in UTC)
static bigtime_t tz_delta;

// The current name of the timezone, saved for all to see
static char tz_name[64];






int hal_time_init()
{
    hal_spin_init(&sys_time_spinlock);
    sys_time = 0;
    sys_time_accurate = 0;
    real_time_delta = 0;
    tz_delta = 0;
    strcpy(tz_name, "UTC");
    real_time_delta = arch_get_rtc_delta();

    return 0;
}






static long msecDivider = 0;
static long secDivider = 0;

void hal_time_tick(int tick_rate)
{
    int ei = hal_save_cli();
    hal_spin_lock(&sys_time_spinlock);
    sys_time += tick_rate;
    hal_spin_unlock(&sys_time_spinlock);
    if (ei) hal_sti();

    msecDivider += tick_rate;
    while(msecDivider > 1000)
    {
        msecDivider -= 1000;
        phantom_process_timed_calls();

        secDivider++;
        if( secDivider > 1000 )
        {
            secDivider -= 1000;
            // Once a sec
            update_tm();
            stat_update_second_stats();
        }
    }
}

int (*arch_get_tick_rate)(void);
bigtime_t (*arch_get_time_delta)(void);

bigtime_t hal_system_time(void)
{
    bigtime_t val;
    bigtime_t d;
    int tick_rate;

    if (arch_get_tick_rate && arch_get_time_delta)
    {
        d = arch_get_time_delta();
        tick_rate = arch_get_tick_rate();
    }

    int ei = hal_save_cli();
    hal_spin_lock(&sys_time_spinlock);
    val = sys_time;

    // ask the architectural specific layer to give us a little better precision if it can
    if (arch_get_tick_rate && arch_get_time_delta)
    {
        val += d;

        // if we're in cli and the tick is pending, compensate for it
        // if, not while: must not cli longer than for 2 ticks,
        // otherwise system time will lag
        if (val < sys_time_accurate)
            val += tick_rate;

        // lag detected, compensate for it
        if (val < sys_time_accurate)
        {
            sys_time += tick_rate;
            val += tick_rate;
        }

        // make sure system time is monotonous
        assert(val >= sys_time_accurate);

        sys_time_accurate = val;
    }

    hal_spin_unlock(&sys_time_spinlock);
    if (ei) hal_sti();

    return val;
}

bigtime_t hal_system_time_lores(void)
{
    bigtime_t val;

    int ei = hal_save_cli();
    hal_spin_lock(&sys_time_spinlock);
    val = sys_time;
    hal_spin_unlock(&sys_time_spinlock);
    if (ei) hal_sti();

    return val;
}

bigtime_t hal_local_time(void)
{
    return hal_system_time() + real_time_delta + tz_delta;
}




time_t time(time_t *timer)
{

    bigtime_t val = hal_local_time();
    //printf("time bigtime = %lld systime = %lld rt delta = %lld tzdelta = %lld\n", val, sys_time, real_time_delta, tz_delta );

    // ToDO last constant must be tuned
    time_t t = (time_t)(val/1000000LL) - 1920962713L;

    if(timer != (time_t*)0)
    {
        *timer = t;
    }
    return t;
}

// Uptime in seconds
time_t uptime(void)
{

    bigtime_t val = hal_system_time_lores();
    return (time_t)(val/1000000LL);
}


struct tm tm_a = { 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // Make sure time will be read ASAP
struct tm tm_b;
struct tm *current_time = &tm_a;

// Supposed to be called once a sec
static void update_tm()
{
    struct tm *tmold = current_time;
    struct tm *tmnew;

    if( current_time == &tm_a )        tmnew = &tm_b;
    else                               tmnew = &tm_a;

    *tmnew = *tmold;
    tmnew->tm_sec++;

    if( tmnew->tm_sec >= 60 )
        rtc_read_tm( tmnew );

    current_time = tmnew;
}






//static char* numbers[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
static int monthDay[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };


static int _getMonthFromYDay(int yDay, int isLeapYear)
{
    int i = 12;
    int month_first;
    do
    {
        i--;
        month_first = monthDay[i] + ((i < 2) ? 0 : isLeapYear);
    }
    while(yDay < month_first);
    return i;
}


static long _getLastDayOfYear(long year)
{
    return 365L * year
        + ((long)year)/4L
        - ((long)year)/400L;
}


static long _getYear(long day)
{
    long y1 = day / 366;

    for(; _getLastDayOfYear(y1) <= day; y1++){}

    return y1;
}

static int _isLeapYear(int year)
{
    if(year % 4 == 0 && year % 400 != 0)
    {
        return 1;
    }
    return 0;
}

/*
 static int _getDayOfYear(int year, int month, int dayOfMonth)
 {
 return dayOfMonth
 + monthDay[month]
 + ((month > 1) ? _isLeapYear(year): 0)
 - 1;
 }


 static long _getDay(int year, int month, int dayOfMonth)
 {
 return _getDayOfYear(year, month, dayOfMonth)
 + _getLastDayOfYear(year - 1);
 }
 */

static struct tm *localtime_helper(const time_t timer, struct tm *tmb)
{
    long long esec = timer / 1000000L;
    long eday = esec / 86400L;
    long daySec = esec % 86400L;
    long dayMin = daySec / 60;

    tmb->tm_year = _getYear(eday);
    tmb->tm_yday = eday - _getLastDayOfYear(tmb->tm_year - 1);
    tmb->tm_wday = (eday + 3) % 7;
    tmb->tm_mon  = _getMonthFromYDay(tmb->tm_yday, _isLeapYear(tmb->tm_year));
    tmb->tm_mday = tmb->tm_yday - monthDay[tmb->tm_mon] + 1;
    tmb->tm_hour = dayMin / 60;
    tmb->tm_min = dayMin % 60;
    tmb->tm_sec = daySec % 60;

    return tmb;
}




struct tm *localtime_rb(bigtime_t timer, struct tm *tmb)
{
    return localtime_helper(timer, tmb);
}
