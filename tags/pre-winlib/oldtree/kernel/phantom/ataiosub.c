#ifdef ARCH_ia32
//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOSUB.C
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
// This C source contains common subroutines to support
// the PIO and DMA command execution and trace functions.
//********************************************************************

#include <ia32/pio.h>
#include <phantom_libc.h>

#include "ataio.h"

//*************************************************************
//
// sub_zero_return_data() -- zero the return data areas.
//
//*************************************************************

void sub_zero_return_data( void )

{
   unsigned int ndx;

   for ( ndx = 0; ndx < sizeof( ata->reg_cmd_info ); ndx ++ )
      ( (unsigned char *) & ata->reg_cmd_info )[ndx] = 0;
}

//*************************************************************
//
// sub_setup_command() -- setup the command parameters
//                        in FR, SC, SN, CL, CH and DH.
//
//*************************************************************

void sub_setup_command( void )

{
   unsigned char dev75;
   unsigned char fr48[2];
   unsigned char sc48[2];
   unsigned char lba48[8];

   // determine value of Device (Drive/Head) register bits 7 and 5

   dev75 = 0;                    // normal value
   if ( ata->reg_incompat_flags & REG_INCOMPAT_DEVREG )
      dev75 = CB_DH_OBSOLETE;    // obsolete value

   // WARNING: THIS CODE IS DESIGNED FOR A STUPID PROCESSOR
   // LIKE INTEL X86 THAT IS Little-Endian, THAT IS, A
   // PROCESSOR THAT STORES DATA IN MEMORY IN THE WRONG
   // BYTE ORDER !!!

   * (u_int16_t *) fr48 = ata->reg_cmd_info.fr1;
   * (u_int16_t *) sc48 = ata->reg_cmd_info.sc1;
   * (u_int32_t *) ( lba48 + 4 ) = ata->reg_cmd_info.lbaHigh1;
   * (u_int32_t *) ( lba48 + 0 ) = ata->reg_cmd_info.lbaLow1;

   pio_outbyte( CB_DC, ata->reg_cmd_info.dc1 );

   if ( ata->reg_cmd_info.lbaSize == LBA28 )
   {
      // in ATA LBA28 mode
      ata->reg_cmd_info.fr1 = fr48[0];
      pio_outbyte( CB_FR, fr48[0] );
      ata->reg_cmd_info.sc1 = sc48[0];
      pio_outbyte( CB_SC, sc48[0] );
      ata->reg_cmd_info.sn1 = lba48[0];
      pio_outbyte( CB_SN, lba48[0] );
      ata->reg_cmd_info.cl1 = lba48[1];
      pio_outbyte( CB_CL, lba48[1] );
      ata->reg_cmd_info.ch1 = lba48[2];
      pio_outbyte( CB_CH, lba48[2] );
      ata->reg_cmd_info.dh1 = ( ata->reg_cmd_info.dh1 & 0xf0 ) | ( lba48[3] & 0x0f );
      pio_outbyte( CB_DH, ata->reg_cmd_info.dh1 | dev75 );
   }
   else
   if ( ata->reg_cmd_info.lbaSize == LBA48 )
   {
      // in ATA LBA48 mode
      pio_outbyte( CB_FR, fr48[1] );
      pio_outbyte( CB_SC, sc48[1] );
      pio_outbyte( CB_SN, lba48[3] );
      pio_outbyte( CB_CL, lba48[4] );
      pio_outbyte( CB_CH, lba48[5] );
      ata->reg_cmd_info.fr1 = fr48[0];
      pio_outbyte( CB_FR, fr48[0] );
      ata->reg_cmd_info.sc1 = sc48[0];
      pio_outbyte( CB_SC, sc48[0] );
      ata->reg_cmd_info.sn1 = lba48[0];
      pio_outbyte( CB_SN, lba48[0] );
      ata->reg_cmd_info.cl1 = lba48[1];
      pio_outbyte( CB_CL, lba48[1] );
      ata->reg_cmd_info.ch1 = lba48[2];
      pio_outbyte( CB_CH, lba48[2] );
      pio_outbyte( CB_DH, ata->reg_cmd_info.dh1 | dev75 );
   }
   else
   {
      // in ATA CHS or ATAPI LBA32 mode
      pio_outbyte( CB_FR, ata->reg_cmd_info.fr1  );
      pio_outbyte( CB_SC, ata->reg_cmd_info.sc1  );
      pio_outbyte( CB_SN, ata->reg_cmd_info.sn1  );
      pio_outbyte( CB_CL, ata->reg_cmd_info.cl1  );
      pio_outbyte( CB_CH, ata->reg_cmd_info.ch1  );
      pio_outbyte( CB_DH, ata->reg_cmd_info.dh1 | dev75 );
   }
}

//*************************************************************
//
// sub_trace_command() -- trace the end of a command.
//
//*************************************************************

void sub_trace_command( void )

{
   unsigned long lba;
   unsigned char sc48[2];
   unsigned char lba48[8];

   ata->reg_cmd_info.st2 = pio_inbyte( CB_STAT );
   ata->reg_cmd_info.as2 = pio_inbyte( CB_ASTAT );
   ata->reg_cmd_info.er2 = pio_inbyte( CB_ERR );
   if ( ata->reg_cmd_info.lbaSize == LBA48 )
   {
      // read back ATA LBA48...
      sc48[0]  = pio_inbyte( CB_SC );
      lba48[0] = pio_inbyte( CB_SN );
      lba48[1] = pio_inbyte( CB_CL );
      lba48[2] = pio_inbyte( CB_CH );
      pio_outbyte( CB_DC, CB_DC_HOB );
      sc48[1]  = pio_inbyte( CB_SC );
      lba48[3] = pio_inbyte( CB_SN );
      ata->reg_cmd_info.sn2 = lba48[3];
      lba48[4] = pio_inbyte( CB_CL );
      ata->reg_cmd_info.cl2 = lba48[4];
      lba48[5] = pio_inbyte( CB_CH );
      ata->reg_cmd_info.ch2 = lba48[5];
      lba48[6] = 0;
      lba48[7] = 0;
      ata->reg_cmd_info.sc2 = * (u_int16_t *) sc48;
      ata->reg_cmd_info.lbaHigh2 = * (u_int32_t *) ( lba48 + 4 );
      ata->reg_cmd_info.lbaLow2  = * (u_int32_t *) ( lba48 + 0 );
      ata->reg_cmd_info.dh2 = pio_inbyte( CB_DH );
   }
   else
   {
      // read back ATA CHS, ATA LBA28 or ATAPI LBA32
      ata->reg_cmd_info.sc2 = pio_inbyte( CB_SC );
      ata->reg_cmd_info.sn2 = pio_inbyte( CB_SN );
      ata->reg_cmd_info.cl2 = pio_inbyte( CB_CL );
      ata->reg_cmd_info.ch2 = pio_inbyte( CB_CH );
      ata->reg_cmd_info.dh2 = pio_inbyte( CB_DH );
      ata->reg_cmd_info.lbaHigh2 = 0;
      ata->reg_cmd_info.lbaLow2 = 0;
      if ( ata->reg_cmd_info.lbaSize == LBA28 )
      {
         lba = ata->reg_cmd_info.dh2 & 0x0f;
         lba = lba << 8;
         lba = lba | ata->reg_cmd_info.ch2;
         lba = lba << 8;
         lba = lba | ata->reg_cmd_info.cl2;
         lba = lba << 8;
         lba = lba | ata->reg_cmd_info.sn2;
         ata->reg_cmd_info.lbaLow2 = lba;
      }
   }
   trc_cht();
}

//*************************************************************
//
// sub_select() - function used to select a drive.
//
// Function to select a drive making sure that BSY=0 DRQ=0.
//
//**************************************************************

int sub_select( int dev )

{

   unsigned char dev75;
   unsigned char status;

   // determine value of Device (Drive/Head) register bits 7 and 5

   dev75 = 0;                    // normal value
   if ( ata->reg_incompat_flags & REG_INCOMPAT_DEVREG )
      dev75 = CB_DH_OBSOLETE;    // obsolete value

   // PAY ATTENTION HERE
   // The caller may want to issue a command to a device that doesn't
   // exist (for example, Exec Dev Diag), so if we see this,
   // just select that device, skip all status checking and return.
   // We assume the caller knows what they are doing!

   if ( ata->reg_config_info[ dev ] < REG_CONFIG_TYPE_ATA )
   {
      // select the device and return

      pio_outbyte( CB_DH, ( dev ? CB_DH_DEV1 : CB_DH_DEV0 ) | dev75 );
      ATA_DELAY();
      return 0;
   }

   // The rest of this is the normal ATA stuff for device selection
   // and we don't expect the caller to be selecting a device that
   // does not exist.
   // We don't know which drive is currently selected but we should
   // wait BSY=0 and DRQ=0. Normally both BSY=0 and DRQ=0
   // unless something is very wrong!

   trc_llt( 0, 0, TRC_LLT_PNBSY );
   while ( 1 )
   {
      status = pio_inbyte( CB_STAT );
      if ( ( status & ( CB_STAT_BSY | CB_STAT_DRQ ) ) == 0 )
         break;
      if ( tmr_chk_timeout() )
      {
         trc_llt( 0, 0, TRC_LLT_TOUT );
         ata->reg_cmd_info.to = 1;
         ata->reg_cmd_info.ec = 11;
         trc_llt( 0, ata->reg_cmd_info.ec, TRC_LLT_ERROR );
         ata->reg_cmd_info.st2 = status;
         ata->reg_cmd_info.as2 = pio_inbyte( CB_ASTAT );
         ata->reg_cmd_info.er2 = pio_inbyte( CB_ERR );
         ata->reg_cmd_info.sc2 = pio_inbyte( CB_SC );
         ata->reg_cmd_info.sn2 = pio_inbyte( CB_SN );
         ata->reg_cmd_info.cl2 = pio_inbyte( CB_CL );
         ata->reg_cmd_info.ch2 = pio_inbyte( CB_CH );
         ata->reg_cmd_info.dh2 = pio_inbyte( CB_DH );
         return 1;
      }
   }

   // Here we select the drive we really want to work with by
   // setting the DEV bit in the Drive/Head register.

   pio_outbyte( CB_DH, ( dev ? CB_DH_DEV1 : CB_DH_DEV0 ) | dev75 );
   ATA_DELAY();

   // Wait for the selected device to have BSY=0 and DRQ=0.
   // Normally the drive should be in this state unless
   // something is very wrong (or initial power up is still in
   // progress).

   trc_llt( 0, 0, TRC_LLT_PNBSY );
   while ( 1 )
   {
      status = pio_inbyte( CB_STAT );
      if ( ( status & ( CB_STAT_BSY | CB_STAT_DRQ ) ) == 0 )
         break;
      if ( tmr_chk_timeout() )
      {
         trc_llt( 0, 0, TRC_LLT_TOUT );
         ata->reg_cmd_info.to = 1;
         ata->reg_cmd_info.ec = 12;
         trc_llt( 0, ata->reg_cmd_info.ec, TRC_LLT_ERROR );
         ata->reg_cmd_info.st2 = status;
         ata->reg_cmd_info.as2 = pio_inbyte( CB_ASTAT );
         ata->reg_cmd_info.er2 = pio_inbyte( CB_ERR );
         ata->reg_cmd_info.sc2 = pio_inbyte( CB_SC );
         ata->reg_cmd_info.sn2 = pio_inbyte( CB_SN );
         ata->reg_cmd_info.cl2 = pio_inbyte( CB_CL );
         ata->reg_cmd_info.ch2 = pio_inbyte( CB_CH );
         ata->reg_cmd_info.dh2 = pio_inbyte( CB_DH );
         return 1;
      }
   }

   // All done.  The return values of this function are described in
   // ATAIO.H.

   if ( ata->reg_cmd_info.ec )
      return 1;
   return 0;
}

//***********************************************************
//
// functions used to read/write the BMIDE registers
//
//***********************************************************

unsigned char sub_readBusMstrCmd( void )

{
   unsigned char x;

   if ( ata->pio_bmide_base_addr < 0x0100 )
      return 0;
   x = inb(ata->pio_bmide_base_addr + BM_COMMAND_REG );
   trc_llt( 0, x, TRC_LLT_R_BM_CR );
   return x;
}


unsigned char sub_readBusMstrStatus( void )

{
   unsigned char x;

   if ( ata->pio_bmide_base_addr < 0x0100 )
      return 0;
   x = inb( ata->pio_bmide_base_addr + BM_STATUS_REG );
   trc_llt( 0, x, TRC_LLT_R_BM_SR );
   return x;
}


void sub_writeBusMstrCmd( unsigned char x )
{

   if ( ata->pio_bmide_base_addr < 0x0100 )
      return;
   trc_llt( 0, x, TRC_LLT_W_BM_CR );
   outb( ata->pio_bmide_base_addr + BM_COMMAND_REG, x );
}


void sub_writeBusMstrStatus( unsigned char x )

{

   if ( ata->pio_bmide_base_addr < 0x0100 )
      return;
   trc_llt( 0, x, TRC_LLT_W_BM_SR );
   outb( ata->pio_bmide_base_addr + BM_STATUS_REG, x );
}

// end ataiosub.c
#endif // ARCH_ia32
