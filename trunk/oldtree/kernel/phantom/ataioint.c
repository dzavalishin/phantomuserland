#ifdef ARCH_ia32

//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOINT.C
//
// by Hale Landis (www.ata-atapi.com)
//
// There is no copyright and there are no restrictions on the use
// of this ATA Low Level I/O Driver code.  It is distributed to
// help other programmers understand how the ATA device interface
// works and it is distributed without any warranty.  Use this
// code at your own risk.
//
// This code is based on the ATA-x, and ATA/ATAPI-x standards and
// on interviews with various ATA controller and drive designers.
//
// This code has been run on many ATA (IDE) drives and
// MFM/RLL controllers.  This code may be a little
// more picky about the status it sees at various times.  A real
// BIOS probably would not check the status as carefully.
//
// Compile with one of the Borland C or C++ compilers.
//
// This C source contains the low level interrupt set up and
// interrupt handler functions.
//********************************************************************

#define DEBUG_MSG_PREFIX "disk"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include "hal.h"

#include <i386/pio.h>
#include <phantom_libc.h>

#include "ataio.h"

//*************************************************************
//
// Global interrupt mode data
//
//*************************************************************

int int_use_intr_flag = 0;    // use INT mode if != 0

volatile int int_intr_cntr;   // interrupt counter, incremented
                              // each time there is an interrupt

volatile int int_intr_flag;   // interrupt flag, incremented
                              // for each device interrupt

unsigned int int_bmide_addr;  // BMIDE status reg i/o address

volatile unsigned char int_bm_status;     // BMIDE status

unsigned int int_ata_addr;    // ATA status reg I/O address

volatile unsigned char int_ata_status;    // ATA status

//*************************************************************
//
// Local data
//
//*************************************************************

// interrupt handler function info...

//static void interrupt ( * int_org_int_vect ) ();  // save area for the
                                                      // system's INT vector

static void int_handler( void * );        // our INT handler

// our interrupt handler's data...

static int int_got_it_now = 0;      // != 0, our interrupt handler
                                    //is installed now

static int int_irq_number = 0;      // IRQ number in use, 1 to 15
//static int int_int_vector = 0;      // INT vector in use,
                                    // INT 8h-15h and INT 70H-77H.
//static int int_shared = 0;          // shared flag


//*************************************************************
//
// Enable interrupt mode -- get the IRQ number we are using.
//
// The function MUST be called before interrupt mode can
// be used!
//
// If this function is called then the int_disable_irq()
// function MUST be called before exiting to DOS.
//
//*************************************************************

int int_enable_irq( int shared, int irqNum,
                    unsigned int bmAddr, unsigned int ataAddr )

//  shared: 0 is not shared, != 0 is shared
//  irqNum: 1 to 15
//  bmAddr: i/o address for BMIDE Status register
//  ataAddr: i/o address for the ATA Status register

{

   // error if interrupts enabled now
   // error if invalid irq number
   // error if bmAddr is < 100H
   // error if shared and bmAddr is 0
   // error if ataAddr is < 100H

   if ( int_use_intr_flag )      		return 1;
   if ( ( irqNum < 1 ) || ( irqNum > 15 ) )     return 2;
   if ( irqNum == 2 )      			return 2;
   if ( bmAddr && ( bmAddr < 0x0100 ) )      	return 3;
   if ( shared && ( ! bmAddr ) )      		return 4;
   if ( ataAddr < 0x0100 )      		return 5;

   // save the input parameters

   //int_shared     = shared;
   int_irq_number = irqNum;
   int_bmide_addr  = bmAddr;
   int_ata_addr   = ataAddr;

   SHOW_INFO( 0, "IDE Set IRQ %d\n", irqNum );

   if( hal_irq_alloc( irqNum, int_handler, 0, HAL_IRQ_SHAREABLE ) )
       return 2;

   // interrupts use is now enabled

   int_use_intr_flag = 1;
   int_got_it_now = 0;

   // Done.

   return 0;
}


//*************************************************************
//
// Install our interrupt handler.
//
// Interrupt mode MUST be setup by calling int_enable_irq()
// before calling this function.
//
//*************************************************************

void int_save_int_vect( void )

{
   // Do nothing if interrupt use not enabled now.
   // Do nothing if our interrupt handler is installed now.

   if ( ! int_use_intr_flag )
      return;
   if ( int_got_it_now )
      return;
#if 0
#error rewrite

   // Disable interrupts.
   // Save the interrupt vector.

   disable();
   int_org_int_vect = getvect( int_int_vector );

   // install our interrupt handler

   setvect( int_int_vector, int_handler );

   // Our interrupt handler is installed now.
#endif
   int_got_it_now = 1;

   // Reset the interrupt flag.

   //int_intr_cntr = 0;
   int_intr_flag = 0;

   // Enable interrupts.

   //enable();
}

//*************************************************************
//
// Restore the interrupt vector.
//
// Interrupt mode MUST be setup by calling int_enable_irq()
// before calling this function.
//
//*************************************************************

void int_restore_int_vect( void )

{
//#error rewrite

   // Do nothing if interrupt useis disabled now.
   // Do nothing if our interrupt handler is not installed.

   if ( ! int_use_intr_flag )
      return;
   if ( ! int_got_it_now )
      return;

   // Disable interrupts.
   // Restore the interrupt vector.
   // Enable interrupts.

   //disable();
   //setvect( int_int_vector, int_org_int_vect );
   //enable();

   // Our interrupt handler is not installed now.

   int_got_it_now = 0;
}

//*************************************************************
//
// ATADRVR's Interrupt Handler.
//
//*************************************************************

static void int_handler( void *arg )
{
    (void) arg;

    // increment the interrupt counter

   int_intr_cntr ++ ;

   // if BMIDE present read the BMIDE status
   // else just read the device status.

   if ( int_bmide_addr )
   {
      // PCI ATA controller...
      // ... read BMIDE status
      int_bm_status = inb( int_bmide_addr );
      //... check if Interrupt bit = 1
      if ( int_bm_status & BM_SR_MASK_INT )
      {
         // ... Interrupt=1...
         // ... increment interrupt flag,
         // ... read ATA status,
         // ... reset Interrupt bit.
         int_intr_flag ++ ;
         int_ata_status = inb( int_ata_addr );
         outb( int_bmide_addr, BM_SR_MASK_INT );
      }
   }
   else
   {
      // legacy ATA controller...
      // ... increment interrupt flag,
      // ... read ATA status.
      int_intr_flag ++ ;
      int_ata_status = inb( int_ata_addr );
   }

   // IRET here (return from interrupt)
}


void dump_ide_stats()
{
    printf("IDE interrupts: %d", int_intr_cntr );
}

// end ataioint.c
#endif // ARCH_ia32
