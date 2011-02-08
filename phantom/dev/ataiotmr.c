#if 0
//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOTMR.C
//
// by Hale Landis (www.ata-atapi.com)
//
// There is no copyright and there are no restrictions on the use
// of this ATA Low Level I/O Driver code.  It is distributed to
// help other programmers understand how the ATA device interface
// works and it is distributed without any warranty.  Use this
// code at your own risk.
//
// This code is based on the ATA-2, ATA-3 and ATA-4 standards and
// on interviews with various ATA controller and drive designers.
//
// This code has been run on many ATA (IDE) drives and
// MFM/RLL controllers.  This code may be a little
// more picky about the status it sees at various times.  A real
// BIOS probably would not check the status as carefully.
//
// Compile with one of the Borland C or C++ compilers.
//
// This C source file contains functions to access the BIOS
// Time of Day information and to set and check the command
// time out period.
//********************************************************************

// TODO redo with void phantom_spinwait(int millis)


#include <time.h>

#include "ataio.h"

//**************************************************************



long tmr_1s_count;            // number of I/O port reads required
                              //    for a 1s delay
long tmr_1ms_count;           // number of I/O port reads required
                              //    for a 1ms delay
long tmr_1us_count;           // number of I/O port reads required
                              //    for a 1us delay
long tmr_500ns_count;         // number of I/O port reads required
                              //    for a 500ns delay



long tmr_time_out = 20L;      // max command execution time in seconds

long tmr_cmd_start_time;      // command start time - see the
                              // tmr_set_timeout() and
                              // tmr_chk_timeout() functions.


//**************************************************************
//
// tmr_set_timeout() - get the command start time
//
//**************************************************************

// TODO without using time() - it is too heavy
// TODO in fact driver must not need this but
//      rely on some timed callout instead

void tmr_set_timeout( void )

{
    // get the command start time
    tmr_cmd_start_time = time(0);
}

//**************************************************************
//
// tmr_chk_timeout() - check for command timeout.
//
// Gives non-zero return if command has timed out.
//
//**************************************************************

int tmr_chk_timeout( void )
{
    long curTime;

    assert(hal_is_sti());

    // get current time
    curTime = time(0);

    // timed out?
    if ( curTime >= ( tmr_cmd_start_time + tmr_time_out ) )
        return 1;      // yes

    // no timeout yet
    return 0;
}

//**************************************************************
//
// tmr_get_delay_counts - compute the number calls to
//    tmr_waste_time() required for 1s, 1ms, 1us and 500ns delays.
//
//**************************************************************

// our 'waste time' function (do some 32-bit multiply/divide)

static long tmr_waste_time( long p );

static long tmr_waste_time( long p )

{
   long lc = 100;

   return ( lc * lc ) / ( ( p * p ) + 1 );
}

// get the delay counts

void tmr_get_delay_counts( void )

{
   long count;
   long curTime, startTime, endTime;
   int loop;
   int retry;

   // do only once
   if ( tmr_1s_count )
      return;

   // outside loop to handle crossing midnight
   count = 0;
   retry = 1;
   while ( retry )
   {
      // wait until the timer ticks
      startTime = time(0);//tmr_read_bios_timer();
      while ( 1 )
      {
         curTime = time(0);//tmr_read_bios_timer();
         if ( curTime != startTime )
            break;
      }
      // waste time for 1 second
      endTime = curTime + 1;
      while ( 1 )
      {
         for ( loop = 0; loop < 100; loop ++ )
            tmr_waste_time( 7 );
         count += 100 ;
         // check timer
         curTime = time(0);//tmr_read_bios_timer();
         // pass midnight?
         /*if ( curTime < startTime )
         {
            count = 0;  // yes, zero count
            break;      // do again
         } */
         // one second yet?
         if ( curTime >= endTime )
         {
            retry = 0;  // yes, we have a count
            break;      // done
         }
      }
   }

   // save the 1s count
   tmr_1s_count = count;
   // divide by 1000 and save 1ms count
   tmr_1ms_count = count = count / 1000L;
   // divide by 1000 and save 1us count
   tmr_1us_count = count = count / 1000L;
   // divide by 2 and save 500ns count
   tmr_500ns_count = count / 2;

   // make sure 1us count is at least 2
   if ( tmr_1us_count < 2L )
      tmr_1us_count = 2L;
   // make sure 500ns count is at least 1
   if ( tmr_500ns_count < 1L )
      tmr_500ns_count = 1L;
}

//**************************************************************
//
// tmr_delay_1ms - delay approximately 'count' milliseconds
//
//**************************************************************

void tmr_delay_1ms( int count )
{
/*
   long loopcnt = tmr_1ms_count * count;

   while ( loopcnt > 0 )
   {
      tmr_waste_time( 7 );
      loopcnt -- ;
      }
      */
    phantom_spinwait(count);
}

//**************************************************************
//
// tmr_delay_1us - delay approximately 'count' microseconds
//
//**************************************************************
#if 0
void tmr_delay_1us( long count )

{
   long loopcnt = tmr_1us_count * count;

   while ( loopcnt > 0 )
   {
      tmr_waste_time( 7 );
      loopcnt -- ;
   }
}
#endif
//**************************************************************
//
// tmr_ata_delay - delay approximately 500 nanoseconds
//
//**************************************************************

void tmr_delay_ata( void )
{
#if 0
   long loopcnt = tmr_500ns_count;

   while ( loopcnt > 0 )
   {
      tmr_waste_time( 7 );
      loopcnt -- ;
   }
#else
   tenmicrosec();
#endif
}

//**************************************************************
//
// tmr_atapi_delay() - delay for ~80ms so that poorly designed
//                     atapi device have time to updated their
//                     status.
//
//**************************************************************

void tmr_delay_atapi( ataio_t *ata, int dev )
{

   if ( ata->reg_config_info[dev] == REG_CONFIG_TYPE_ATA )
      return;
   if ( ! ata->reg_atapi_delay_flag )
      return;
   trc_llt( 0, 0, TRC_LLT_DELAY1 );
   tmr_delay_1ms( 80L );
}

//**************************************************************
//
// tmr_xfer_delay() - random delay until the timer ticks
//
// TODO must be 0-55 msec, redo
//
//**************************************************************

void tmr_delay_xfer( void )

{
   //long lw;
   assert(hal_is_sti());
   trc_llt( 0, 0, TRC_LLT_DELAY2 );
   //lw = time(0); //tmr_read_bios_timer();
   //while ( lw == time(0) ) //tmr_read_bios_timer() )
   //   /* do nothing */ ;

   bigtime_t now = hal_system_time();
   while( now == hal_system_time() )
       ; // tight loop!


}

// end ataiotmr.c
#endif
