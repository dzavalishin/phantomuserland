#if 0

//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOREG.C
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
// This C source contains the main body of the driver code:
// Determine what devices are present, issue ATA Soft Reset,
// execute Non-Data, PIO data in and PIO data out commands.
//********************************************************************


#include "ataio.h"

#define DEBUG_REG 0x00  // not zero for debug
// 0x01 trace the interrupt flag

//*************************************************************
//
// ATAPI command packet,
// atapi register data,
// atapi delay flag,
// config information,
// slow transfer flag,
// incompatible flags.
//
//*************************************************************

//unsigned char reg_atapi_cp_data[16];
//int reg_atapi_cp_size;

//int reg_atapi_delay_flag;

//unsigned char reg_atapi_reg_fr;  // see reg_packet()
//unsigned char reg_atapi_reg_sc;  // see reg_packet()
//unsigned char reg_atapi_reg_sn;  // see reg_packet()
//unsigned char reg_atapi_reg_dh;  // see reg_packet()

//long reg_buffer_size;

//struct REG_CMD_INFO reg_cmd_info;

//int reg_config_info[2];

//void ( * reg_drq_block_call_back ) ( struct REG_CMD_INFO * );

//long reg_slow_xfer_flag;

//int reg_incompat_flags;

//*************************************************************
//
// reg_wait_poll() - wait for interrupt or poll for BSY=0
//
//*************************************************************

//static void reg_wait_poll( ataio_t *ata, int we, int pe );

static void reg_wait_poll( ataio_t *ata, struct REG_CMD_INFO *reg_cmd_info_p, int we, int pe )
{
    unsigned char status;

    // Wait for interrupt -or- wait for not BUSY -or- wait for time out.

    if ( we && ata->int_use_intr_flag )
    {
        trc_llt( 0, 0, TRC_LLT_WINT );
        //printf("wait 4 intr... ");
        while ( 1 )
        {
            if ( ata->int_intr_flag )                // interrupt ?
            {

                // slows alot on QEMU
                // TODO on real hardware - reenable?
                //sched_yield(); // driver is threaded, so just yeld

                trc_llt( 0, 0, TRC_LLT_INTRQ );  // yes
                if ( ata->int_bmide_addr )
                    trc_llt( 0, ata->int_bm_status, TRC_LLT_R_BM_SR );
                status = ata->int_ata_status;         // get status
                trc_llt( CB_STAT, status, TRC_LLT_INB );
                if ( ata->int_bmide_addr )
                    trc_llt( 0, 0x04, TRC_LLT_W_BM_SR );
                break;
            }
            if ( tmr_chk_timeout() )            // time out ?
            {
                trc_llt( 0, 0, TRC_LLT_TOUT );   // yes
                reg_cmd_info.to = 1;
                reg_cmd_info.ec = we;
                trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                break;
            }
            if(ata->intr_wait) ata->intr_wait(ata);
        }
    }
    else
    {
        trc_llt( 0, 0, TRC_LLT_PNBSY );
        //printf("poll... ");
        while ( 1 )
        {
            status = pio_inbyte( CB_ASTAT );       // poll for not busy
            if ( ( status & CB_STAT_BSY ) == 0 )
                break;
            if ( tmr_chk_timeout() )               // time out yet ?
            {
                trc_llt( 0, 0, TRC_LLT_TOUT );
                reg_cmd_info.to = 1;
                reg_cmd_info.ec = pe;
                trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                break;
            }
        }
    }
    //printf("end poll... ");
#if DEBUG_REG & 0x01
    trc_llt( 0, ata->int_intr_cntr, TRC_LLT_DEBUG );  // for debugging
#endif

    // Reset the interrupt flag.

    if ( ata->int_intr_flag > 1 )      // extra interrupt(s) ?
        reg_cmd_info.failbits |= FAILBIT15;
    ata->int_intr_flag = 0;
}

//*************************************************************
//
// reg_config() - Check the host adapter and determine the
//                number and type of drives attached.
//
// This process is not documented by any of the ATA standards.
//
// Infomation is returned by this function is in
// reg_config_info[] -- see ATAIO.H.
//
//*************************************************************

int reg_config( ataio_t *ata  )

{
    int numDev = 0;
    unsigned char dev75;
    unsigned char sc;
    unsigned char sn;
    unsigned char cl;
    unsigned char ch;
    unsigned char st;
    unsigned char devCtrl;

    // compute the 1ms, 1us and 500ns delay counts - the number of I/O reads
    // required to get an approximately 1ms, 1us and 500ns delay.

    tmr_get_delay_counts();

    // determine value of Device (Drive/Head) register bits 7 and 5

    dev75 = 0;                    // normal value
    if ( ata->reg_incompat_flags & REG_INCOMPAT_DEVREG )
        dev75 = CB_DH_OBSOLETE;    // obsolete value

    // setup register values

    devCtrl = CB_DC_HD15 | ( ata->int_use_intr_flag ? 0 : CB_DC_NIEN );

    // mark start of config in low level trace

    trc_llt( 0, 0, TRC_LLT_S_CFG );

    // reset Bus Master Error bit

    sub_writeBusMstrStatus( ata, BM_SR_MASK_ERR );

    // assume there are no devices

    ata->reg_config_info[0] = REG_CONFIG_TYPE_NONE;
    ata->reg_config_info[1] = REG_CONFIG_TYPE_NONE;

    // set up Device Control register

    pio_outbyte( CB_DC, devCtrl );

    // lets see if there is a device 0

    pio_outbyte( CB_DH, CB_DH_DEV0 | dev75 );
    ATA_DELAY();
    pio_outbyte( CB_SC, 0x55 );
    pio_outbyte( CB_SN, 0xaa );
    pio_outbyte( CB_SC, 0xaa );
    pio_outbyte( CB_SN, 0x55 );
    pio_outbyte( CB_SC, 0x55 );
    pio_outbyte( CB_SN, 0xaa );
    sc = pio_inbyte( CB_SC );
    sn = pio_inbyte( CB_SN );
    if ( ( sc == 0x55 ) && ( sn == 0xaa ) )
        ata->reg_config_info[0] = REG_CONFIG_TYPE_UNKN;

    // lets see if there is a device 1

    pio_outbyte( CB_DH, CB_DH_DEV1 | dev75 );
    ATA_DELAY();
    pio_outbyte( CB_SC, 0x55 );
    pio_outbyte( CB_SN, 0xaa );
    pio_outbyte( CB_SC, 0xaa );
    pio_outbyte( CB_SN, 0x55 );
    pio_outbyte( CB_SC, 0x55 );
    pio_outbyte( CB_SN, 0xaa );
    sc = pio_inbyte( CB_SC );
    sn = pio_inbyte( CB_SN );
    if ( ( sc == 0x55 ) && ( sn == 0xaa ) )
        ata->reg_config_info[1] = REG_CONFIG_TYPE_UNKN;

    // quit if no devices found

    if ( ( ata->reg_config_info[0] == REG_CONFIG_TYPE_NONE )
         &&
         ( ata->reg_config_info[1] == REG_CONFIG_TYPE_NONE )
       )
    {
        trc_llt( 0, 0, TRC_LLT_E_CFG );
        return numDev;    // no devices found
    }

    // now we think we know which devices, if any are there,
    // so lets try a soft reset (ignoring any errors).

    pio_outbyte( CB_DH, CB_DH_DEV0 | dev75 );
    ATA_DELAY();
    reg_reset( ata, 0, 0 );

    // lets check device 0 again, is device 0 really there?
    // is it ATA or ATAPI?

    pio_outbyte( CB_DH, CB_DH_DEV0 | dev75 );
    ATA_DELAY();
    sc = pio_inbyte( CB_SC );
    sn = pio_inbyte( CB_SN );
    st = pio_inbyte( CB_STAT );
    if ( st == 0x7f )
    {
        ata->reg_config_info[0] = REG_CONFIG_TYPE_NONE;
    }
    else
        if ( ( sc == 0x01 ) && ( sn == 0x01 ) )
        {
            // yep, there is a device, what kind is it?
            numDev ++ ;
            cl = pio_inbyte( CB_CL );
            ch = pio_inbyte( CB_CH );
            if ( ( ( cl == 0x14 ) && ( ch == 0xeb ) )       // PATAPI
                 ||
                 ( ( cl == 0x69 ) && ( ch == 0x96 ) )       // SATAPI
               )
            {
                ata->reg_config_info[0] = REG_CONFIG_TYPE_ATAPI;
            }
            else
                if ( ( ( cl == 0x00 ) && ( ch == 0x00 ) )     // PATA
                     ||
                     ( ( cl == 0x3c ) && ( ch == 0xc3 ) )     // SATA
                   )
                {
                    ata->reg_config_info[0] = REG_CONFIG_TYPE_ATA;
                }
        }

    // lets check device 1 again, is device 1 really there?
    // is it ATA or ATAPI?

    pio_outbyte( CB_DH, CB_DH_DEV1 | dev75 );
    ATA_DELAY();
    sc = pio_inbyte( CB_SC );
    sn = pio_inbyte( CB_SN );
    st = pio_inbyte( CB_STAT );
    if ( st == 0x7f )
    {
        ata->reg_config_info[1] = REG_CONFIG_TYPE_NONE;
    }
    else
        if ( ( sc == 0x01 ) && ( sn == 0x01 ) )
        {
            // yep, there is a device, what kind is it?
            numDev ++ ;
            cl = pio_inbyte( CB_CL );
            ch = pio_inbyte( CB_CH );
            if ( ( ( cl == 0x14 ) && ( ch == 0xeb ) )       // PATAPI
                 ||
                 ( ( cl == 0x69 ) && ( ch == 0x96 ) )       // SATAPI
               )
            {
                ata->reg_config_info[1] = REG_CONFIG_TYPE_ATAPI;
            }
            else
                if ( ( ( cl == 0x00 ) && ( ch == 0x00 ) )     // PATA
                     ||
                     ( ( cl == 0x3c ) && ( ch == 0xc3 ) )     // SATA
                   )
                {
                    ata->reg_config_info[1] = REG_CONFIG_TYPE_ATA;
                }
        }

    // If possible, select a device that exists

    if ( ata->reg_config_info[1] != REG_CONFIG_TYPE_NONE )
    {
        pio_outbyte( CB_DH, CB_DH_DEV1 | dev75 );
        ATA_DELAY();
    }
    if ( ata->reg_config_info[0] != REG_CONFIG_TYPE_NONE )
    {
        pio_outbyte( CB_DH, CB_DH_DEV0 | dev75 );
        ATA_DELAY();
    }

    // BMIDE Error=1?

    if ( sub_readBusMstrStatus(ata) & BM_SR_MASK_ERR )
    {
        reg_cmd_info.ec = 78;                  // yes
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
    }

    // mark end of config in low level trace

    trc_llt( 0, 0, TRC_LLT_E_CFG );

    // return the number of devices found

    return numDev;
}

//*************************************************************
//
// reg_reset() - Execute a Software Reset.
//
// See ATA-2 Section 9.2, ATA-3 Section 9.2, ATA-4 Section 8.3.
//
//*************************************************************

int reg_reset( ataio_t *ata, int skipFlag, int devRtrn )
{
    unsigned char dev75;
    unsigned char sc;
    unsigned char sn;
    unsigned char status;
    unsigned char devCtrl;

    // determine value of Device (Drive/Head) register bits 7 and 5

    dev75 = 0;                    // normal value
    if ( ata->reg_incompat_flags & REG_INCOMPAT_DEVREG )
        dev75 = CB_DH_OBSOLETE;    // obsolete value

    // setup register values

    devCtrl = CB_DC_HD15 | ( ata->int_use_intr_flag ? 0 : CB_DC_NIEN );

    // mark start of reset in low level trace

    trc_llt( 0, 0, TRC_LLT_S_RST );

    // reset Bus Master Error bit

    sub_writeBusMstrStatus( ata, BM_SR_MASK_ERR );

    // Reset error return data.

    sub_zero_return_data(ata);
    reg_cmd_info.flg = TRC_FLAG_SRST;
    reg_cmd_info.ct  = TRC_TYPE_ASR;

    // initialize the command timeout counter

    tmr_set_timeout();

    // Set and then reset the soft reset bit in the Device Control
    // register.  This causes device 0 be selected.

    if ( ! skipFlag )
    {
        // set SRST=1
        pio_outbyte( CB_DC, devCtrl | CB_DC_SRST );
        ATA_DELAY();      // for trace of Alternate Status
        // delay ~10us
        //tmr_delay_1us( 10L );
        tmr_delay_1ms( 1 );
        // set SRST=0
        pio_outbyte( CB_DC, devCtrl );
        ATA_DELAY();      // for trace of Alternate Status
    }
    ATAPI_DELAY( ata, 0 );
    ATAPI_DELAY( ata, 1 );

    // If there is a device 0, wait for device 0 to set BSY=0.

    if ( ata->reg_config_info[0] != REG_CONFIG_TYPE_NONE )
    {
        trc_llt( 0, 0, TRC_LLT_PNBSY );
        while ( 1 )
        {
            status = pio_inbyte( CB_STAT );
            if ( ( status & CB_STAT_BSY ) == 0 )
                break;
            if ( tmr_chk_timeout() )
            {
                trc_llt( 0, 0, TRC_LLT_TOUT );
                reg_cmd_info.to = 1;
                reg_cmd_info.ec = 1;
                trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                break;
            }
        }
    }

    // Check that drive 0 has valid BSY=0 status

    if ( reg_cmd_info.ec == 0 )
    {
        status = pio_inbyte( CB_STAT );
        if ( ( status == 0x7f ) || ( status & CB_STAT_BSY ) )
        {
            reg_cmd_info.ec = 1;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        }
    }

    // If there is a device 1, wait until device 1 allows
    // register access.

    if ( ata->reg_config_info[1] != REG_CONFIG_TYPE_NONE )
    {
        trc_llt( 0, 0, TRC_LLT_PNBSY );
        while ( 1 )
        {
            pio_outbyte( CB_DH, CB_DH_DEV1 | dev75 );
            ATA_DELAY();
            sc = pio_inbyte( CB_SC );
            sn = pio_inbyte( CB_SN );
            if ( ( sc == 0x01 ) && ( sn == 0x01 ) )
                break;
            if ( tmr_chk_timeout() )
            {
                trc_llt( 0, 0, TRC_LLT_TOUT );
                reg_cmd_info.to = 1;
                reg_cmd_info.ec = 2;
                trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                break;
            }
        }

        // Check if drive 1 has valid BSY=0 status

        if ( reg_cmd_info.ec == 0 )
        {
            status = pio_inbyte( CB_STAT );
            if ( ( status == 0x7f ) || ( status & CB_STAT_BSY ) )
            {
                reg_cmd_info.ec = 3;
                trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            }
        }
    }

    // RESET_DONE:

    // We are done but now we must select the device the caller
    // requested before we trace the command.  This will cause
    // the correct data to be returned in reg_cmd_info.

    pio_outbyte( CB_DH, ( devRtrn ? CB_DH_DEV1 : CB_DH_DEV0 ) | dev75 );
    ATA_DELAY();
    sub_trace_command(ata);

    // If possible, select a device that exists

    if ( ata->reg_config_info[1] != REG_CONFIG_TYPE_NONE )
    {
        pio_outbyte( CB_DH, CB_DH_DEV1 | dev75 );
        ATA_DELAY();
    }
    if ( ata->reg_config_info[0] != REG_CONFIG_TYPE_NONE )
    {
        pio_outbyte( CB_DH, CB_DH_DEV0 | dev75 );
        ATA_DELAY();
    }

    // BMIDE Error=1?

    if ( sub_readBusMstrStatus(ata) & BM_SR_MASK_ERR )
    {
        reg_cmd_info.ec = 78;                  // yes
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
    }

    // mark end of reset in low level trace

    trc_llt( 0, 0, TRC_LLT_E_RST );

    // All done.  The return values of this function are described in
    // ATAIO.H.

    if ( reg_cmd_info.ec )
        return 1;
    return 0;
}

//*************************************************************
//
// exec_non_data_cmd() - Execute a non-data command.
//
// Note special handling for Execute Device Diagnostics
// command when there is no device 0.
//
// See ATA-2 Section 9.5, ATA-3 Section 9.5,
// ATA-4 Section 8.8 Figure 12.  Also see Section 8.5.
//
//*************************************************************

//static int exec_non_data_cmd( ataio_t *ata, int dev );

static int exec_non_data_cmd( ataio_t *ata,  struct REG_CMD_INFO *reg_cmd_info_p, int dev )
{
    unsigned char dev75;
    unsigned char secCnt;
    unsigned char secNum;
    unsigned char status;
    int polled = 0;

//    struct REG_CMD_INFO reg_cmd_info = *reg_cmd_info_p; // OK?

//#define reg_cmd_info (*reg_cmd_info_p)


    // determine value of Device (Drive/Head) register bits 7 and 5

    dev75 = 0;                    // normal value
    if ( ata->reg_incompat_flags & REG_INCOMPAT_DEVREG )
        dev75 = CB_DH_OBSOLETE;    // obsolete value

    // mark start of ND cmd in low level trace

    trc_llt( 0, 0, TRC_LLT_S_ND );

    // reset Bus Master Error bit

    sub_writeBusMstrStatus( ata, BM_SR_MASK_ERR );

    // Set command time out.

    tmr_set_timeout();

    // PAY ATTENTION HERE
    // If the caller is attempting a Device Reset command, then
    // don't do most of the normal stuff.  Device Reset has no
    // parameters, should not generate an interrupt and it is the
    // only command that can be written to the Command register
    // when a device has BSY=1 (a very strange command!).  Not
    // all devices support this command (even some ATAPI devices
    // don't support the command.

    if ( reg_cmd_info.cmd != CMD_DEVICE_RESET )
    {
        // Select the drive - call the sub_select function.
        // Quit now if this fails.

        if ( sub_select( ata, dev ) )
        {
            sub_trace_command(ata);
            trc_llt( 0, 0, TRC_LLT_E_ND );
            return 1;
        }

        // Set up all the registers except the command register.

        sub_setup_command(ata);

        // For interrupt mode, install interrupt handler.

        //int_save_int_vect();
        ata->int_intr_flag = 0;
    }

    // Start the command by setting the Command register.  The drive
    // should immediately set BUSY status.

    pio_outbyte( CB_CMD, reg_cmd_info.cmd );

    // Waste some time by reading the alternate status a few times.
    // This gives the drive time to set BUSY in the status register on
    // really fast systems.  If we don't do this, a slow drive on a fast
    // system may not set BUSY fast enough and we would think it had
    // completed the command when it really had not even started the
    // command yet.
    //printf("ata delay... ");
    ATA_DELAY();
    ATAPI_DELAY( ata, 0 );
    ATAPI_DELAY( ata, 1 );

    // IF
    //    This is an Exec Dev Diag command (cmd=0x90)
    //    and there is no device 0 then
    //    there will be no interrupt. So we must
    //    poll device 1 until it allows register
    //    access and then do normal polling of the Status
    //    register for BSY=0.
    // ELSE
    // IF
    //    This is a Dev Reset command (cmd=0x08) then
    //    there should be no interrupt.  So we must
    //    poll for BSY=0.
    // ELSE
    //    Do the normal wait for interrupt or polling for
    //    completion.

    if ( ( reg_cmd_info.cmd == CMD_EXECUTE_DEVICE_DIAGNOSTIC )
         &&
         ( ata->reg_config_info[0] == REG_CONFIG_TYPE_NONE )
       )
    {
        polled = 1;
        trc_llt( 0, 0, TRC_LLT_PNBSY );
        while ( 1 )
        {
            pio_outbyte( CB_DH, CB_DH_DEV1 | dev75 );
            ATA_DELAY();
            secCnt = pio_inbyte( CB_SC );
            secNum = pio_inbyte( CB_SN );
            if ( ( secCnt == 0x01 ) && ( secNum == 0x01 ) )
                break;
            if ( tmr_chk_timeout() )
            {
                trc_llt( 0, 0, TRC_LLT_TOUT );
                reg_cmd_info.to = 1;
                reg_cmd_info.ec = 24;
                trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                break;
            }
        }
    }
    else
    {
        if ( reg_cmd_info.cmd == CMD_DEVICE_RESET )
        {
            // Wait for not BUSY -or- wait for time out.
            //printf("ata wait busy (reset)... ");

            polled = 1;
            reg_wait_poll( ata, reg_cmd_info_p, 0, 23 );
        }
        else
        {
            // Wait for interrupt -or- wait for not BUSY -or- wait for time out.

            //printf("ata wait... ");
            if ( ! ata->int_use_intr_flag )
                polled = 1;
            reg_wait_poll( ata, reg_cmd_info_p, 22, 23 );
        }
    }

    // If status was polled or if any error read the status register,
    // otherwise get the status that was read by the interrupt handler.
    //printf("ata read status... ");
    if ( ( polled ) || ( reg_cmd_info.ec ) )
        status = pio_inbyte( CB_STAT );
    else
        status = ata->int_ata_status;

    // Error if BUSY, DEVICE FAULT, DRQ or ERROR status now.

    if ( reg_cmd_info.ec == 0 )
    {
        if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_DRQ | CB_STAT_ERR ) )
        {
            reg_cmd_info.ec = 21;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        }
    }

    // read the output registers and trace the command.

    sub_trace_command(ata);

    // BMIDE Error=1?

    if ( sub_readBusMstrStatus(ata) & BM_SR_MASK_ERR )
    {
        reg_cmd_info.ec = 78;                  // yes
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
    }

    // NON_DATA_DONE:

    // For interrupt mode, remove interrupt handler.

    //int_restore_int_vect();

    // mark end of ND cmd in low level trace

    trc_llt( 0, 0, TRC_LLT_E_ND );

    // All done.  The return values of this function are described in
    // ATAIO.H.
    //printf("ata return, ec = %d... ", reg_cmd_info.ec);
    if ( reg_cmd_info.ec )
        return 1;
    return 0;

//#undef reg_cmd_info

}

//*************************************************************
//
// reg_non_data_chs() - Execute a non-data command.
//
// Note special handling for Execute Device Diagnostics
// command when there is no device 0.
//
// See ATA-2 Section 9.5, ATA-3 Section 9.5,
// ATA-4 Section 8.8 Figure 12.  Also see Section 8.5.
//
//*************************************************************

int reg_non_data_chs( ataio_t *ata, int dev, int cmd,
                      unsigned int fr, unsigned int sc,
                      unsigned int cyl, unsigned int head, unsigned int sect )

{
//    struct REG_CMD_INFO reg_cmd_info;

    // Setup command parameters.

    sub_zero_return_data(ata);
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_AND;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.sn1 = sect;
    reg_cmd_info.cl1 = cyl & 0x00ff;
    reg_cmd_info.ch1 = ( cyl & 0xff00 ) >> 8;
    reg_cmd_info.dh1 = ( dev ? CB_DH_DEV1 : CB_DH_DEV0 ) | ( head & 0x0f );
    reg_cmd_info.dc1 = ata->int_use_intr_flag ? 0 : CB_DC_NIEN;
    reg_cmd_info.ns  = sc;
    reg_cmd_info.lbaSize = LBACHS;

    // Execute the command.

    return exec_non_data_cmd( ata, &reg_cmd_info, dev );
}

//*************************************************************
//
// reg_non_data_lba28() - Easy way to execute a non-data command
//                        using an LBA sector address.
//
//*************************************************************

int reg_non_data_lba28( ataio_t *ata, int dev, int cmd,
                        unsigned int fr, unsigned int sc,
                        unsigned long lba )

{
//    struct REG_CMD_INFO reg_cmd_info;

    // Setup current command information.

    sub_zero_return_data(ata);
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_AND;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.dh1 = CB_DH_LBA | (dev ? CB_DH_DEV1 : CB_DH_DEV0 );
    reg_cmd_info.dc1 = ata->int_use_intr_flag ? 0 : CB_DC_NIEN;
    reg_cmd_info.ns  = sc;
    reg_cmd_info.lbaSize = LBA28;
    reg_cmd_info.lbaHigh1 = 0L;
    reg_cmd_info.lbaLow1 = lba;

    // Execute the command.

    return exec_non_data_cmd( ata, &reg_cmd_info, dev );
}

//*************************************************************
//
// reg_non_data_lba48() - Easy way to execute a non-data command
//                        using an LBA sector address.
//
//*************************************************************

int reg_non_data_lba48( ataio_t *ata, int dev, int cmd,
                        unsigned int fr, unsigned int sc,
                        unsigned long lbahi, unsigned long lbalo )

{
//    struct REG_CMD_INFO reg_cmd_info;

    // Setup current command infomation.

    sub_zero_return_data(ata);
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_AND;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.dh1 = CB_DH_LBA | (dev ? CB_DH_DEV1 : CB_DH_DEV0 );
    reg_cmd_info.dc1 = ata->int_use_intr_flag ? 0 : CB_DC_NIEN;
    reg_cmd_info.ns  = sc;
    reg_cmd_info.lbaSize = LBA48;
    reg_cmd_info.lbaHigh1 = lbahi;
    reg_cmd_info.lbaLow1 = lbalo;

    // Execute the command.

    return exec_non_data_cmd( ata, &reg_cmd_info, dev );
}

//*************************************************************
//
// exec_pio_data_in_cmd() - Execute a PIO Data In command.
//
// See ATA-2 Section 9.3, ATA-3 Section 9.3,
// ATA-4 Section 8.6 Figure 10.
//
//*************************************************************

static int exec_pio_data_in_cmd( ataio_t *ata, struct REG_CMD_INFO *reg_cmd_info_p, int dev,
                                 void *addr, long numSect, int multiCnt )
{
    unsigned char status;
    long wordCnt;
    //unsigned int saveSeg = seg;
    //unsigned int saveOff = off;
    void *saveAddr = addr;

//    struct REG_CMD_INFO reg_cmd_info = *reg_cmd_info_p;
//#define reg_cmd_info (*reg_cmd_info_p)

    // mark start of PDI cmd in low level trace

    trc_llt( 0, 0, TRC_LLT_S_PDI );

    // reset Bus Master Error bit

    sub_writeBusMstrStatus( ata, BM_SR_MASK_ERR );

    // Set command time out.

    tmr_set_timeout();

    // Select the drive - call the sub_select function.
    // Quit now if this fails.

    if ( sub_select( ata, dev ) )
    {
        sub_trace_command(ata);
        trc_llt( 0, 0, TRC_LLT_E_PDI );
        ata->reg_drq_block_call_back = (void *) 0;
        return 1;
    }

    // Set up all the registers except the command register.

    sub_setup_command(ata);

    // For interrupt mode, install interrupt handler.

    //int_save_int_vect();
    ata->int_intr_flag = 0;

    // Start the command by setting the Command register.  The drive
    // should immediately set BUSY status.

    pio_outbyte( CB_CMD, reg_cmd_info.cmd );

    // Waste some time by reading the alternate status a few times.
    // This gives the drive time to set BUSY in the status register on
    // really fast systems.  If we don't do this, a slow drive on a fast
    // system may not set BUSY fast enough and we would think it had
    // completed the command when it really had not even started the
    // command yet.

    ATA_DELAY();

    // Loop to read each sector.

    while ( 1 )
    {
        // READ_LOOP:
        //
        // NOTE NOTE NOTE ...  The primary status register (1f7) MUST NOT be
        // read more than ONCE for each sector transferred!  When the
        // primary status register is read, the drive resets IRQ 14.  The
        // alternate status register (3f6) can be read any number of times.
        // After interrupt read the the primary status register ONCE
        // and transfer the 256 words (REP INSW).  AS SOON as BOTH the
        // primary status register has been read AND the last of the 256
        // words has been read, the drive is allowed to generate the next
        // IRQ 14 (newer and faster drives could generate the next IRQ 14 in
        // 50 microseconds or less).  If the primary status register is read
        // more than once, there is the possibility of a race between the
        // drive and the software and the next IRQ 14 could be reset before
        // the system interrupt controller sees it.

        // Wait for interrupt -or- wait for not BUSY -or- wait for time out.

        ATAPI_DELAY( ata, dev );
        reg_wait_poll( ata, reg_cmd_info_p, 34, 35 );

        // If polling or error read the status, otherwise
        // get the status that was read by the interrupt handler.

        if ( ( ! ata->int_use_intr_flag ) || ( reg_cmd_info.ec ) )
            status = pio_inbyte( CB_STAT );
        else
            status = ata->int_ata_status;

        // If there was a time out error, go to READ_DONE.

        if ( reg_cmd_info.ec )
            break;   // go to READ_DONE

        // If BSY=0 and DRQ=1, transfer the data,
        // even if we find out there is an error later.

        if ( ( status & ( CB_STAT_BSY | CB_STAT_DRQ ) ) == CB_STAT_DRQ )
        {
            // do the slow data transfer thing

            if ( ata->reg_slow_xfer_flag )
            {
                if ( numSect <= ata->reg_slow_xfer_flag )
                {
                    tmr_delay_xfer();
                    ata->reg_slow_xfer_flag = 0;
                }
            }

            // increment number of DRQ packets

            reg_cmd_info.drqPackets ++ ;

            // determine the number of sectors to transfer

            wordCnt = multiCnt ? multiCnt : 1;
            if ( wordCnt > numSect )
                wordCnt = numSect;
            wordCnt = wordCnt * 256;

            // Quit if buffer overrun.
            // Adjust buffer address when DRQ block call back in use.

            if ( ata->reg_drq_block_call_back )
            {
                if ( ( wordCnt << 1 ) > ata->reg_buffer_size )
                {
                    reg_cmd_info.ec = 61;
                    trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                    break;   // go to READ_DONE
                }
                //seg = saveSeg;
                //off = saveOff;
                addr = saveAddr;
            }
            else
            {
                if ( ( reg_cmd_info.totalBytesXfer + ( wordCnt << 1 ) )
                     > ata->reg_buffer_size )
                {
                    reg_cmd_info.ec = 61;
                    trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                    break;   // go to READ_DONE
                }
            }

            // Do the REP INSW to read the data for one DRQ block.

            reg_cmd_info.totalBytesXfer += ( wordCnt << 1 );
            pio_drq_block_in( ata, CB_DATA, addr, wordCnt );

            ATA_DELAY();   // delay so device can get the status updated

            // Note: The drive should have dropped DATA REQUEST by now.  If there
            // are more sectors to transfer, BUSY should be active now (unless
            // there is an error).

            // Call DRQ block call back function.
            if ( ata->reg_drq_block_call_back )
            {
                reg_cmd_info.drqPacketSize = ( wordCnt << 1 );
                (* ata->reg_drq_block_call_back) ( & reg_cmd_info );
            }

            // Decrement the count of sectors to be transferred
            // and increment buffer address.

            numSect = numSect - ( multiCnt ? multiCnt : 1 );
            //seg = seg + ( 32 * ( multiCnt ? multiCnt : 1 ) );
            addr += 512 * ( multiCnt ? multiCnt : 1 );
        }

        // So was there any error condition?

        if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_ERR ) )
        {
            reg_cmd_info.ec = 31;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;   // go to READ_DONE
        }

        // DRQ should have been set -- was it?

        if ( ( status & CB_STAT_DRQ ) == 0 )
        {
            reg_cmd_info.ec = 32;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;   // go to READ_DONE
        }

        // If all of the requested sectors have been transferred, make a
        // few more checks before we exit.

        if ( numSect < 1 )
        {
            // Since the drive has transferred all of the requested sectors
            // without error, the drive should not have BUSY, DEVICE FAULT,
            // DATA REQUEST or ERROR active now.

            ATAPI_DELAY( ata, dev );
            status = pio_inbyte( CB_STAT );
            if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_DRQ | CB_STAT_ERR ) )
                //if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_ERR ) )
            {
                reg_cmd_info.ec = 33;
                trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                break;   // go to READ_DONE
            }

            // All sectors have been read without error, go to READ_DONE.

            break;   // go to READ_DONE

        }

        // This is the end of the read loop.  If we get here, the loop is
        // repeated to read the next sector.  Go back to READ_LOOP.

    }

    // read the output registers and trace the command.

    sub_trace_command(ata);

    // BMIDE Error=1?

    if ( sub_readBusMstrStatus(ata) & BM_SR_MASK_ERR )
    {
        reg_cmd_info.ec = 78;                  // yes
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
    }

    // READ_DONE:

    // For interrupt mode, remove interrupt handler.

    //int_restore_int_vect();

    // mark end of PDI cmd in low level trace

    trc_llt( 0, 0, TRC_LLT_E_PDI );

    // reset reg_drq_block_call_back to NULL (0)

    ata->reg_drq_block_call_back = (void *) 0;

    // All done.  The return values of this function are described in
    // ATAIO.H.

    if ( reg_cmd_info.ec )
        return 1;
    return 0;
}

//#undef reg_cmd_info


#if 0
//*************************************************************
//
// reg_pio_data_in_chs() - Execute a PIO Data In command.
//
// See ATA-2 Section 9.3, ATA-3 Section 9.3,
// ATA-4 Section 8.6 Figure 10.
//
//*************************************************************

int reg_pio_data_in_chs( ataio_t *ata, int dev, int cmd,
                         unsigned int fr, unsigned int sc,
                         unsigned int cyl, unsigned int head, unsigned int sect,
                         unsigned int seg, unsigned int off,
                         long numSect, int multiCnt )

{
//    struct REG_CMD_INFO reg_cmd_info;

    // Reset error return data.

    sub_zero_return_data();
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_APDI;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.sn1 = sect;
    reg_cmd_info.cl1 = cyl & 0x00ff;
    reg_cmd_info.ch1 = ( cyl & 0xff00 ) >> 8;
    reg_cmd_info.dh1 = ( dev ? CB_DH_DEV1 : CB_DH_DEV0 ) | ( head & 0x0f );
    reg_cmd_info.dc1 = int_use_intr_flag ? 0 : CB_DC_NIEN;
    reg_cmd_info.lbaSize = LBACHS;

    // these commands transfer only 1 sector
    if (    ( cmd == CMD_IDENTIFY_DEVICE )
            || ( cmd == CMD_IDENTIFY_DEVICE_PACKET )
       )
        numSect = 1;

    // adjust multiple count
    if ( multiCnt & 0x0800 )
    {
        // assume caller knows what they are doing
        multiCnt &= 0x00ff;
    }
    else
    {
        // only multiple commands use multiCnt
        if (    ( cmd != CMD_READ_MULTIPLE )
                && ( cmd != CMD_READ_MULTIPLE_EXT )
           )
            multiCnt = 1;
    }

    reg_cmd_info.ns  = numSect;
    reg_cmd_info.mc  = multiCnt;

    return exec_pio_data_in_cmd( ata, &reg_cmd_info, dev, seg, off, numSect, multiCnt );
}
#endif

//*************************************************************
//
// reg_pio_data_in_lba28() - Easy way to execute a PIO Data In command
//                           using an LBA sector address.
//
//*************************************************************

int reg_pio_data_in_lba28( ataio_t *ata, int dev, int cmd,
                           unsigned int fr, unsigned int sc,
                           unsigned long lba,
                           void *addr,
                           long numSect, int multiCnt )

{
//    struct REG_CMD_INFO reg_cmd_info;

    // Reset error return data.

    sub_zero_return_data(ata);
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_APDI;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.dh1 = CB_DH_LBA | (dev ? CB_DH_DEV1 : CB_DH_DEV0 );
    reg_cmd_info.dc1 = ata->int_use_intr_flag ? 0 : CB_DC_NIEN;
    reg_cmd_info.lbaSize = LBA28;
    reg_cmd_info.lbaHigh1 = 0L;
    reg_cmd_info.lbaLow1 = lba;

    // these commands transfer only 1 sector
    if (    ( cmd == CMD_IDENTIFY_DEVICE )
            || ( cmd == CMD_IDENTIFY_DEVICE_PACKET )
       )
        numSect = 1;

    // adjust multiple count
    if ( multiCnt & 0x0800 )
    {
        // assume caller knows what they are doing
        multiCnt &= 0x00ff;
    }
    else
    {
        // only multiple commands use multiCnt
        if (    ( cmd != CMD_READ_MULTIPLE )
                && ( cmd != CMD_READ_MULTIPLE_EXT )
           )
            multiCnt = 1;
    }

    reg_cmd_info.ns  = numSect;
    reg_cmd_info.mc  = multiCnt;

    return exec_pio_data_in_cmd( ata, &reg_cmd_info, dev, addr, numSect, multiCnt );
}
//*************************************************************
//
// reg_pio_data_in_lba48() - Easy way to execute a PIO Data In command
//                           using an LBA sector address.
//
//*************************************************************

int reg_pio_data_in_lba48( ataio_t *ata, int dev, int cmd,
                           unsigned int fr, unsigned int sc,
                           unsigned long lbahi, unsigned long lbalo,
                           //unsigned int seg, unsigned int off,
                           void *addr,
                           long numSect, int multiCnt )

{
//    struct REG_CMD_INFO reg_cmd_info;

    // Reset error return data.

    sub_zero_return_data(ata);
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_APDI;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.dh1 = CB_DH_LBA | (dev ? CB_DH_DEV1 : CB_DH_DEV0 );
    reg_cmd_info.dc1 = ata->int_use_intr_flag ? 0 : CB_DC_NIEN;
    reg_cmd_info.lbaSize = LBA48;
    reg_cmd_info.lbaHigh1 = lbahi;
    reg_cmd_info.lbaLow1 = lbalo;

    // adjust multiple count
    if ( multiCnt & 0x0800 )
    {
        // assume caller knows what they are doing
        multiCnt &= 0x00ff;
    }
    else
    {
        // only multiple commands use multiCnt
        if (    ( cmd != CMD_READ_MULTIPLE )
                && ( cmd != CMD_READ_MULTIPLE_EXT )
           )
            multiCnt = 1;
    }

    reg_cmd_info.ns  = numSect;
    reg_cmd_info.mc  = multiCnt;

    return exec_pio_data_in_cmd( ata, &reg_cmd_info, dev, addr, numSect, multiCnt );
}

//*************************************************************
//
// exec_pio_data_out_cmd() - Execute a PIO Data Out command.
//
// See ATA-2 Section 9.4, ATA-3 Section 9.4,
// ATA-4 Section 8.7 Figure 11.
//
//*************************************************************

static int exec_pio_data_out_cmd( ataio_t *ata, struct REG_CMD_INFO *reg_cmd_info_p, int dev,
                                  void *addr, long numSect, int multiCnt )
{
    unsigned char status;
    int loopFlag = 1;
    long wordCnt;
    //unsigned int saveSeg = seg;
    //unsigned int saveOff = off;
    void *saveAddr = addr;

    // mark start of PDO cmd in low level trace

    trc_llt( 0, 0, TRC_LLT_S_PDO );

    // reset Bus Master Error bit

    sub_writeBusMstrStatus( ata, BM_SR_MASK_ERR );

    // Set command time out.

    tmr_set_timeout();

    // Select the drive - call the sub_select function.
    // Quit now if this fails.

    if ( sub_select( ata, dev ) )
    {
        sub_trace_command(ata);
        trc_llt( 0, 0, TRC_LLT_E_PDO );
        ata->reg_drq_block_call_back = (void *) 0;
        return 1;
    }

    // Set up all the registers except the command register.

    sub_setup_command(ata);

    // For interrupt mode, instal interrupt handler.

    //int_save_int_vect(ata);
    ata->int_intr_flag = 0;

    // Start the command by setting the Command register.  The drive
    // should immediately set BUSY status.

    pio_outbyte( CB_CMD, reg_cmd_info.cmd );

    // Waste some time by reading the alternate status a few times.
    // This gives the drive time to set BUSY in the status register on
    // really fast systems.  If we don't do this, a slow drive on a fast
    // system may not set BUSY fast enough and we would think it had
    // completed the command when it really had not even started the
    // command yet.

    ATA_DELAY();
    ATAPI_DELAY( ata, dev );

    // Wait for not BUSY or time out.
    // Note: No interrupt is generated for the
    // first sector of a write command.  Well...
    // that's not really true we are working with
    // a PCMCIA PC Card ATA device.

    trc_llt( 0, 0, TRC_LLT_PNBSY );
    while ( 1 )
    {
        status = pio_inbyte( CB_ASTAT );
        if ( ( status & CB_STAT_BSY ) == 0 )
            break;
        if ( tmr_chk_timeout() )
        {
            trc_llt( 0, 0, TRC_LLT_TOUT );
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 47;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            loopFlag = 0;
            break;
        }
    }

    // If we are using interrupts and we just got an interrupt, this is
    // wrong.  The drive must not generate an interrupt at this time.

    if ( loopFlag && ata->int_use_intr_flag && ata->int_intr_flag )
    {
        reg_cmd_info.ec = 46;
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        loopFlag = 0;
    }

    // This loop writes each sector.

    while ( loopFlag )
    {
        // WRITE_LOOP:
        //
        // NOTE NOTE NOTE ...  The primary status register (1f7) MUST NOT be
        // read more than ONCE for each sector transferred!  When the
        // primary status register is read, the drive resets IRQ 14.  The
        // alternate status register (3f6) can be read any number of times.
        // For correct results, transfer the 256 words (REP OUTSW), wait for
        // interrupt and then read the primary status register.  AS
        // SOON as BOTH the primary status register has been read AND the
        // last of the 256 words has been written, the drive is allowed to
        // generate the next IRQ 14 (newer and faster drives could generate
        // the next IRQ 14 in 50 microseconds or less).  If the primary
        // status register is read more than once, there is the possibility
        // of a race between the drive and the software and the next IRQ 14
        // could be reset before the system interrupt controller sees it.

        // If BSY=0 and DRQ=1, transfer the data,
        // even if we find out there is an error later.

        if ( ( status & ( CB_STAT_BSY | CB_STAT_DRQ ) ) == CB_STAT_DRQ )
        {
            // do the slow data transfer thing

            if ( ata->reg_slow_xfer_flag )
            {
                if ( numSect <= ata->reg_slow_xfer_flag )
                {
                    tmr_delay_xfer();
                    ata->reg_slow_xfer_flag = 0;
                }
            }

            // increment number of DRQ packets

            reg_cmd_info.drqPackets ++ ;

            // determine the number of sectors to transfer

            wordCnt = multiCnt ? multiCnt : 1;
            if ( wordCnt > numSect )
                wordCnt = numSect;
            wordCnt = wordCnt * 256;

            // Quit if buffer overrun.
            // If DRQ call back in use:
            // a) Call DRQ block call back function.
            // b) Adjust buffer address.

            if ( ata->reg_drq_block_call_back )
            {
                if ( ( wordCnt << 1 ) > ata->reg_buffer_size )
                {
                    reg_cmd_info.ec = 61;
                    trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                    break;   // go to READ_DONE
                }
                reg_cmd_info.drqPacketSize = ( wordCnt << 1 );
                (* ata->reg_drq_block_call_back) ( & reg_cmd_info );
                //seg = saveSeg;
                //off = saveOff;
                addr = saveAddr;
            }
            else
            {
                if ( ( reg_cmd_info.totalBytesXfer + ( wordCnt << 1 ) )
                     > ata->reg_buffer_size )
                {
                    reg_cmd_info.ec = 61;
                    trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                    break;   // go to READ_DONE
                }
            }

            // Do the REP OUTSW to write the data for one DRQ block.

            reg_cmd_info.totalBytesXfer += ( wordCnt << 1 );
            pio_drq_block_out( ata, CB_DATA, addr, wordCnt );

            ATA_DELAY();   // delay so device can get the status updated

            // Note: The drive should have dropped DATA REQUEST and
            // raised BUSY by now.

            // Decrement the count of sectors to be transferred
            // and increment buffer address.

            numSect = numSect - ( multiCnt ? multiCnt : 1 );
            //seg = seg + ( 32 * ( multiCnt ? multiCnt : 1 ) );
            addr += 512 * ( multiCnt ? multiCnt : 1 );
        }

        // So was there any error condition?

        if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_ERR ) )
        {
            reg_cmd_info.ec = 41;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;   // go to WRITE_DONE
        }

        // DRQ should have been set -- was it?

        if ( ( status & CB_STAT_DRQ ) == 0 )
        {
            reg_cmd_info.ec = 42;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;   // go to WRITE_DONE
        }

        // Wait for interrupt -or- wait for not BUSY -or- wait for time out.

        ATAPI_DELAY( ata, dev );
        reg_wait_poll( ata, reg_cmd_info_p, 44, 45 );

        // If polling or error read the status, otherwise
        // get the status that was read by the interrupt handler.

        if ( ( ! ata->int_use_intr_flag ) || ( reg_cmd_info.ec ) )
            status = pio_inbyte( CB_STAT );
        else
            status = ata->int_ata_status;

        // If there was a time out error, go to WRITE_DONE.

        if ( reg_cmd_info.ec )
            break;   // go to WRITE_DONE

        // If all of the requested sectors have been transferred, make a
        // few more checks before we exit.

        if ( numSect < 1 )
        {
            // Since the drive has transferred all of the sectors without
            // error, the drive MUST not have BUSY, DEVICE FAULT, DATA REQUEST
            // or ERROR status at this time.

            if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_DRQ | CB_STAT_ERR ) )
            {
                reg_cmd_info.ec = 43;
                trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                break;   // go to WRITE_DONE
            }

            // All sectors have been written without error, go to WRITE_DONE.

            break;   // go to WRITE_DONE

        }

        //
        // This is the end of the write loop.  If we get here, the loop
        // is repeated to write the next sector.  Go back to WRITE_LOOP.

    }

    // read the output registers and trace the command.

    sub_trace_command(ata);

    // BMIDE Error=1?

    if ( sub_readBusMstrStatus(ata) & BM_SR_MASK_ERR )
    {
        reg_cmd_info.ec = 78;                  // yes
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
    }

    // WRITE_DONE:

    // For interrupt mode, remove interrupt handler.

    //int_restore_int_vect();

    // mark end of PDO cmd in low level trace

    trc_llt( 0, 0, TRC_LLT_E_PDO );

    // reset reg_drq_block_call_back to NULL (0)

    ata->reg_drq_block_call_back = (void *) 0;

    // All done.  The return values of this function are described in
    // ATAIO.H.

    if ( reg_cmd_info.ec )
        return 1;
    return 0;
}

#if 0
//*************************************************************
//
// reg_pio_data_out_chs() - Execute a PIO Data Out command.
//
// See ATA-2 Section 9.4, ATA-3 Section 9.4,
// ATA-4 Section 8.7 Figure 11.
//
//*************************************************************

int reg_pio_data_out_chs( ataio_t *ata, int dev, int cmd,
                          unsigned int fr, unsigned int sc,
                          unsigned int cyl, unsigned int head, unsigned int sect,
                          unsigned int seg, unsigned int off,
                          long numSect, int multiCnt )

{
//    struct REG_CMD_INFO reg_cmd_info;

    // Reset error return data.

    sub_zero_return_data();
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_APDO;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.sn1 = sect;
    reg_cmd_info.cl1 = cyl & 0x00ff;
    reg_cmd_info.ch1 = ( cyl & 0xff00 ) >> 8;
    reg_cmd_info.dh1 = ( dev ? CB_DH_DEV1 : CB_DH_DEV0 ) | ( head & 0x0f );
    reg_cmd_info.dc1 = int_use_intr_flag ? 0 : CB_DC_NIEN;
    reg_cmd_info.lbaSize = LBACHS;

    // adjust multiple count
    if ( multiCnt & 0x0800 )
    {
        // assume caller knows what they are doing
        multiCnt &= 0x00ff;
    }
    else
    {
        // only multiple commands use multiCnt
        if (    ( cmd != CMD_CFA_WRITE_MULTIPLE_WO_ERASE )
                && ( cmd != CMD_WRITE_MULTIPLE )
                && ( cmd != CMD_WRITE_MULTIPLE_EXT )
                && ( cmd != CMD_WRITE_MULTIPLE_FUA_EXT )
           )
            multiCnt = 1;
    }

    reg_cmd_info.ns  = numSect;
    reg_cmd_info.mc  = multiCnt;

    return exec_pio_data_out_cmd( dev, seg, off, numSect, multiCnt );
}

//*************************************************************
//
// reg_pio_data_out_lba28() - Easy way to execute a PIO Data In command
//                            using an LBA sector address.
//
//*************************************************************
#endif

int reg_pio_data_out_lba28( ataio_t *ata, int dev, int cmd,
                            unsigned int fr, unsigned int sc,
                            unsigned long lba,
                            void *addr,
                            long numSect, int multiCnt )

{
//    struct REG_CMD_INFO reg_cmd_info;

    // Reset error return data.

    sub_zero_return_data(ata);
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_APDO;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.dh1 = CB_DH_LBA | (dev ? CB_DH_DEV1 : CB_DH_DEV0 );
    reg_cmd_info.dc1 = ata->int_use_intr_flag ? 0 : CB_DC_NIEN;
    reg_cmd_info.lbaSize = LBA28;
    reg_cmd_info.lbaHigh1 = 0;
    reg_cmd_info.lbaLow1 = lba;

    // adjust multiple count
    if ( multiCnt & 0x0800 )
    {
        // assume caller knows what they are doing
        multiCnt &= 0x00ff;
    }
    else
    {
        // only multiple commands use multiCnt
        if (    ( cmd != CMD_CFA_WRITE_MULTIPLE_WO_ERASE )
                && ( cmd != CMD_WRITE_MULTIPLE )
                && ( cmd != CMD_WRITE_MULTIPLE_EXT )
                && ( cmd != CMD_WRITE_MULTIPLE_FUA_EXT )
           )
            multiCnt = 1;
    }

    reg_cmd_info.ns  = numSect;
    reg_cmd_info.mc  = multiCnt;

    return exec_pio_data_out_cmd( ata, &reg_cmd_info, dev, addr, numSect, multiCnt );
}


//*************************************************************
//
// reg_pio_data_out_lba48() - Easy way to execute a PIO Data In command
//                            using an LBA sector address.
//
//*************************************************************

int reg_pio_data_out_lba48( ataio_t *ata, int dev, int cmd,
                            unsigned int fr, unsigned int sc,
                            unsigned long lbahi, unsigned long lbalo,
                            //unsigned int seg, unsigned int off,
                            void *addr,
                            long numSect, int multiCnt )

{
//    struct REG_CMD_INFO reg_cmd_info;

    // Reset error return data.

    sub_zero_return_data(ata);
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_APDO;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.dh1 = CB_DH_LBA | (dev ? CB_DH_DEV1 : CB_DH_DEV0 );
    reg_cmd_info.dc1 = ata->int_use_intr_flag ? 0 : CB_DC_NIEN;
    reg_cmd_info.lbaSize = LBA48;
    reg_cmd_info.lbaHigh1 = lbahi;
    reg_cmd_info.lbaLow1 = lbalo;

    // adjust multiple count
    if ( multiCnt & 0x0800 )
    {
        // assume caller knows what they are doing
        multiCnt &= 0x00ff;
    }
    else
    {
        // only multiple commands use multiCnt
        if (    ( cmd != CMD_CFA_WRITE_MULTIPLE_WO_ERASE )
                && ( cmd != CMD_WRITE_MULTIPLE )
                && ( cmd != CMD_WRITE_MULTIPLE_EXT )
                && ( cmd != CMD_WRITE_MULTIPLE_FUA_EXT )
           )
            multiCnt = 1;
    }

    reg_cmd_info.ns  = numSect;
    reg_cmd_info.mc  = multiCnt;

    return exec_pio_data_out_cmd( ata, &reg_cmd_info, dev, addr, numSect, multiCnt );
}

//*************************************************************
//
// reg_packet() - Execute an ATAPI Packet (A0H) command.
//
// See ATA-4 Section 9.10, Figure 14.
//
//*************************************************************

int reg_packet( ataio_t *ata, int dev,
                unsigned int cpbc,
                //unsigned int cpseg, unsigned int cpoff,
                void *cpAddr,
                int dir,
                long dpbc,
                //unsigned int dpseg, unsigned int dpoff,
                void *dpAddr,
                unsigned long lba )

{
    unsigned char status;
    unsigned char reason;
    unsigned char lowCyl;
    unsigned char highCyl;
    unsigned int byteCnt;
    long wordCnt;
    int ndx;
    //unsigned long dpaddr;
    void *savedpaddr;
    unsigned char * cfp;
    int slowXferCntr = 0;
    int prevFailBit7 = 0;

//    struct REG_CMD_INFO reg_cmd_info;

    // mark start of PI cmd in low level trace

    trc_llt( 0, 0, TRC_LLT_S_PI );

    // reset Bus Master Error bit

    sub_writeBusMstrStatus( ata, BM_SR_MASK_ERR );

    // Make sure the command packet size is either 12 or 16
    // and save the command packet size and data.

    cpbc = cpbc < 12 ? 12 : cpbc;
    cpbc = cpbc > 12 ? 16 : cpbc;

    // Setup current command information.

    sub_zero_return_data(ata);
    reg_cmd_info.flg = TRC_FLAG_ATAPI;
    reg_cmd_info.ct  = dir ? TRC_TYPE_PPDO : TRC_TYPE_PPDI;
    reg_cmd_info.cmd = CMD_PACKET;
    reg_cmd_info.fr1 = ata->reg_atapi_reg_fr;
    reg_cmd_info.sc1 = ata->reg_atapi_reg_sc;
    reg_cmd_info.sn1 = ata->reg_atapi_reg_sn;
    reg_cmd_info.cl1 = (unsigned char) ( dpbc & 0x00ff );
    reg_cmd_info.ch1 = ( unsigned char) ( ( dpbc & 0xff00 ) >> 8 );
    reg_cmd_info.dh1 = ( dev ? CB_DH_DEV1 : CB_DH_DEV0 )
        | ( ata->reg_atapi_reg_dh & 0x0f );
    reg_cmd_info.dc1 = ata->int_use_intr_flag ? 0 : CB_DC_NIEN;
    reg_cmd_info.lbaSize = LBA32;
    reg_cmd_info.lbaLow1 = lba;
    reg_cmd_info.lbaHigh1 = 0L;
    ata->reg_atapi_cp_size = cpbc;
    cfp = cpAddr;
    for ( ndx = 0; ndx < cpbc; ndx ++ )
    {
        ata->reg_atapi_cp_data[ndx] = * cfp;
        cfp ++ ;
    }

    // Zero the alternate ATAPI register data.

    ata->reg_atapi_reg_fr = 0;
    ata->reg_atapi_reg_sc = 0;
    ata->reg_atapi_reg_sn = 0;
    ata->reg_atapi_reg_dh = 0;

    // Set command time out.

    tmr_set_timeout();

    // Select the drive - call the sub_select function.
    // Quit now if this fails.

    if ( sub_select( ata, dev ) )
    {
        sub_trace_command(ata);
        trc_llt( 0, 0, TRC_LLT_E_PI );
        ata->reg_drq_block_call_back = (void *) 0;
        return 1;
    }

    // Set up all the registers except the command register.

    sub_setup_command(ata);

    // For interrupt mode, install interrupt handler.

    //int_save_int_vect();
    ata->int_intr_flag = 0;

    // Start the command by setting the Command register.  The drive
    // should immediately set BUSY status.

    pio_outbyte( CB_CMD, CMD_PACKET );

    // Waste some time by reading the alternate status a few times.
    // This gives the drive time to set BUSY in the status register on
    // really fast systems.  If we don't do this, a slow drive on a fast
    // system may not set BUSY fast enough and we would think it had
    // completed the command when it really had not even started the
    // command yet.

    ATA_DELAY();
    ATAPI_DELAY( ata, dev );

    // Command packet transfer...
    // Check for protocol failures,
    // the device should have BSY=1 or
    // if BSY=0 then either DRQ=1 or CHK=1.

    status = pio_inbyte( CB_ASTAT );
    if ( status & CB_STAT_BSY )
    {
        // BSY=1 is OK
    }
    else
    {
        if ( status & ( CB_STAT_DRQ | CB_STAT_ERR ) )
        {
            // BSY=0 and DRQ=1 is OK
            // BSY=0 and ERR=1 is OK
        }
        else
        {
            reg_cmd_info.failbits |= FAILBIT0;  // not OK
        }
    }

    // Command packet transfer...
    // Poll Alternate Status for BSY=0.

    trc_llt( 0, 0, TRC_LLT_PNBSY );
    while ( 1 )
    {
        status = pio_inbyte( CB_ASTAT );       // poll for not busy
        if ( ( status & CB_STAT_BSY ) == 0 )
            break;
        if ( tmr_chk_timeout() )               // time out yet ?
        {
            trc_llt( 0, 0, TRC_LLT_TOUT );      // yes
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 51;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            dir = -1;   // command done
            break;
        }
    }

    // Command packet transfer...
    // Check for protocol failures... no interrupt here please!
    // Clear any interrupt the command packet transfer may have caused.

    if ( ata->int_intr_flag )       // extra interrupt(s) ?
        reg_cmd_info.failbits |= FAILBIT1;
    ata->int_intr_flag = 0;

    // Command packet transfer...
    // If no error, transfer the command packet.

    if ( reg_cmd_info.ec == 0 )
    {

        // Command packet transfer...
        // Read the primary status register and the other ATAPI registers.

        status = pio_inbyte( CB_STAT );
        reason = pio_inbyte( CB_SC );
        lowCyl = pio_inbyte( CB_CL );
        highCyl = pio_inbyte( CB_CH );

        // Command packet transfer...
        // check status: must have BSY=0, DRQ=1 now

        if (    ( status & ( CB_STAT_BSY | CB_STAT_DRQ | CB_STAT_ERR ) )
                != CB_STAT_DRQ
           )
        {
            reg_cmd_info.ec = 52;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            dir = -1;   // command done
        }
        else
        {
            // Command packet transfer...
            // Check for protocol failures...
            // check: C/nD=1, IO=0.

            if ( ( reason &  ( CB_SC_P_TAG | CB_SC_P_REL | CB_SC_P_IO ) )
                 || ( ! ( reason &  CB_SC_P_CD ) )
               )
                reg_cmd_info.failbits |= FAILBIT2;
            if (    ( lowCyl != reg_cmd_info.cl1 )
                    || ( highCyl != reg_cmd_info.ch1 ) )
                reg_cmd_info.failbits |= FAILBIT3;

            // Command packet transfer...
            // trace cdb byte 0,
            // xfer the command packet (the cdb)

            trc_llt( 0, * (unsigned char *) cpAddr, TRC_LLT_P_CMD );
            pio_drq_block_out( ata, CB_DATA, cpAddr, cpbc >> 1 );

            ATA_DELAY();   // delay so device can get the status updated
        }
    }

    // Data transfer loop...
    // If there is no error, enter the data transfer loop.
    // First adjust the I/O buffer address so we are able to
    // transfer large amounts of data (more than 64K).

    //dpaddr = dpseg;
    //dpaddr = dpaddr << 4;
    //dpaddr = dpaddr + dpoff;
    savedpaddr = dpAddr;

    while ( reg_cmd_info.ec == 0 )
    {
        // Data transfer loop...
        // Wait for interrupt -or- wait for not BUSY -or- wait for time out.

        ATAPI_DELAY( ata, dev );
        reg_wait_poll( ata, &reg_cmd_info, 53, 54 );

        // Data transfer loop...
        // If there was a time out error, exit the data transfer loop.

        if ( reg_cmd_info.ec )
        {
            dir = -1;   // command done
            break;
        }

        // Data transfer loop...
        // If using interrupts get the status read by the interrupt
        // handler, otherwise read the status register.

        if ( ata->int_use_intr_flag )
            status = ata->int_ata_status;
        else
            status = pio_inbyte( CB_STAT );
        reason = pio_inbyte( CB_SC );
        lowCyl = pio_inbyte( CB_CL );
        highCyl = pio_inbyte( CB_CH );

        // Data transfer loop...
        // Exit the read data loop if the device indicates this
        // is the end of the command.

        if ( ( status & ( CB_STAT_BSY | CB_STAT_DRQ ) ) == 0 )
        {
            dir = -1;   // command done
            break;
        }

        // Data transfer loop...
        // The device must want to transfer data...
        // check status: must have BSY=0, DRQ=1 now.

        if ( ( status & ( CB_STAT_BSY | CB_STAT_DRQ ) ) != CB_STAT_DRQ )
        {
            reg_cmd_info.ec = 55;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            dir = -1;   // command done
            break;
        }
        else
        {
            // Data transfer loop...
            // Check for protocol failures...
            // check: C/nD=0, IO=1 (read) or IO=0 (write).

            if ( ( reason &  ( CB_SC_P_TAG | CB_SC_P_REL ) )
                 || ( reason &  CB_SC_P_CD )
               )
                reg_cmd_info.failbits |= FAILBIT4;
            if ( ( reason & CB_SC_P_IO ) && dir )
                reg_cmd_info.failbits |= FAILBIT5;
        }

        // do the slow data transfer thing

        if ( ata->reg_slow_xfer_flag )
        {
            slowXferCntr ++ ;
            if ( slowXferCntr <= ata->reg_slow_xfer_flag )
            {
                tmr_delay_xfer();
                ata->reg_slow_xfer_flag = 0;
            }
        }

        // Data transfer loop...
        // get the byte count, check for zero...

        byteCnt = ( highCyl << 8 ) | lowCyl;
        if ( byteCnt < 1 )
        {
            reg_cmd_info.ec = 59;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            dir = -1;   // command done
            break;
        }

        // Data transfer loop...
        // and check protocol failures...

        if ( byteCnt > dpbc )
            reg_cmd_info.failbits |= FAILBIT6;
        reg_cmd_info.failbits |= prevFailBit7;
        prevFailBit7 = 0;
        if ( byteCnt & 0x0001 )
            prevFailBit7 = FAILBIT7;

        // Data transfer loop...
        // Quit if buffer overrun.
        // If DRQ call back in use adjust buffer address.

        if ( ata->reg_drq_block_call_back )
        {
            if ( byteCnt > ata->reg_buffer_size )
            {
                reg_cmd_info.ec = 61;
                trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                dir = -1;   // command done
                break;
            }
            reg_cmd_info.drqPacketSize = byteCnt;
            dpAddr = savedpaddr;
        }
        else
        {
            if ( ( reg_cmd_info.totalBytesXfer + byteCnt ) > ata->reg_buffer_size )
            {
                reg_cmd_info.ec = 61;
                trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                dir = -1;   // command done
                break;
            }
        }

        // increment number of DRQ packets

        reg_cmd_info.drqPackets ++ ;

        // Data transfer loop...
        // transfer the data and update the i/o buffer address
        // and the number of bytes transfered.

        wordCnt = ( byteCnt >> 1 ) + ( byteCnt & 0x0001 );
        reg_cmd_info.totalBytesXfer += ( wordCnt << 1 );
        //dpseg = (unsigned int) ( dpaddr >> 4 );
        //dpoff = (unsigned int) ( dpaddr & 0x0000000fL );
        if ( dir )
        {
            if ( ata->reg_drq_block_call_back )
                (* ata->reg_drq_block_call_back) ( & reg_cmd_info );
            pio_drq_block_out( ata, CB_DATA, dpAddr, wordCnt );
        }
        else
        {
            pio_drq_block_in( ata, CB_DATA, dpAddr, wordCnt );
            if ( ata->reg_drq_block_call_back )
                (* ata->reg_drq_block_call_back) ( & reg_cmd_info );
        }
        dpAddr += byteCnt;

        ATA_DELAY();   // delay so device can get the status updated
    }

    // End of command...
    // Wait for interrupt or poll for BSY=0,
    // but don't do this if there was any error or if this
    // was a commmand that did not transfer data.

    if ( ( reg_cmd_info.ec == 0 ) && ( dir >= 0 ) )
    {
        ATAPI_DELAY( ata, dev );
        reg_wait_poll( ata, &reg_cmd_info, 56, 57 );
    }

    // Final status check, only if no previous error.

    if ( reg_cmd_info.ec == 0 )
    {
        // Final status check...
        // If using interrupts get the status read by the interrupt
        // handler, otherwise read the status register.

        if ( ata->int_use_intr_flag )
            status = ata->int_ata_status;
        else
            status = pio_inbyte( CB_STAT );
        reason = pio_inbyte( CB_SC );
        lowCyl = pio_inbyte( CB_CL );
        highCyl = pio_inbyte( CB_CH );

        // Final status check...
        // check for any error.

        if ( status & ( CB_STAT_BSY | CB_STAT_DRQ | CB_STAT_ERR ) )
        {
            reg_cmd_info.ec = 58;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        }
    }

    // Final status check...
    // Check for protocol failures...
    // check: C/nD=1, IO=1.

    if ( ( reason & ( CB_SC_P_TAG | CB_SC_P_REL ) )
         || ( ! ( reason & CB_SC_P_IO ) )
         || ( ! ( reason & CB_SC_P_CD ) )
       )
        reg_cmd_info.failbits |= FAILBIT8;

    // Done...
    // Read the output registers and trace the command.

    if ( ! reg_cmd_info.totalBytesXfer )
        reg_cmd_info.ct = TRC_TYPE_PND;
    sub_trace_command(ata);

    // Final status check
    // BMIDE Error=1?

    if ( sub_readBusMstrStatus(ata) & BM_SR_MASK_ERR )
    {
        reg_cmd_info.ec = 78;                  // yes
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
    }

    // For interrupt mode, remove interrupt handler.

    //int_restore_int_vect();

    // mark end of PI cmd in low level trace

    trc_llt( 0, 0, TRC_LLT_E_PI );

    // reset reg_drq_block_call_back to NULL (0)

    ata->reg_drq_block_call_back = (void *) 0;

    // All done.  The return values of this function are described in
    // ATAIO.H.

    if ( reg_cmd_info.ec )
        return 1;
    return 0;
}

// end ataioreg.c
#endif
