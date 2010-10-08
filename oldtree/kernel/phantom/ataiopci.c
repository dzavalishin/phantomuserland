/**
 *
 * dz - removed LARGE mode at all for simplicity.
 *
**/

#include <i386/pio.h>
#include <phantom_libc.h>
#include <phantom_types.h>

#include <malloc.h>



//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOPCI.C
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
// This C source contains the functions for executing ATA PCI bus
// mastering READ/WRITE DMA commands for ATA and ATAPI.
//********************************************************************

#include "ataio.h"

#define DEBUG_PCI 0x00  // not zero for debug
                        // 0x01 trace the interrupt counter and flag
                        // 0x02 debug LARGE PRD

#if DEBUG_PCI & 0x02
   extern unsigned char LFB[128];
   extern void prt( void );
   extern void pstr( unsigned char * s );
#endif

//***********************************************************
//
// Some notes about PCI bus mastering DMA...
//
// ATA PCI Bus Master DMA was first described by the SFF-8038
// document.  That document is now very obsolete and generally
// difficult to obtain.  The ANSI INCITS T13 document "ATA Host
// Adapter Standards" (T13 document 1510D) has replaced
// SFF-8038.  This code supports the type of DMA described by
// sections 1 to 5 of the T13 1510D document.
//
// Note that the T13 1510D document also describes (in section 6) a
// complex DMA engine called ADMA.  While ADMA is a good idea it
// will probably never be popular or widely implemented.  This code
// does not support ADMA.
//
// The base address of the Bus Master Control Registers (BMIDE) is
// found in the PCI Configuration space for the ATA controller (at
// offset 0x30 in the config space data).  This is normally an I/O
// address.
//
// The BMIDE data is 16 bytes of data starting at the BMIDE base
// address.  The first 8 bytes is for the primary ATA channel and
// the second 8 bytes is for the secondary ATA channel.  The 8 bytes
// contain a "command" byte and a "status" byte and a 4 byte
// (32-bit) physical memory address pointing to the Physical Region
// Descriptor (PRD) list.  Each PRD entry describes an area of
// memory or data buffer for the DMA transfer.  A region described
// by a PRD may not cross a 64K byte boundary in physical memory.
// Also, the PRD list must not cross a 64K byte boundary.
//
// SIMPLE/COMPLEX PRD lists...
// This code can build a PRD list for data transfers up to 256K
// bytes. These types of PRD lists are built in a data area
// that is statically allocated below. These PRD lists use
// transfer the data to/from the caller's I/O buffer.
//
// LARGE PRD lists...
// This code also supports READ/WRITE DMA EXT command transfers
// up to 65536 sectors. This is done by creating a large
// PRD list in the caller's I/O buffer. This PRD list
// reuses a 64K part of the caller's I/O buffer for the data transfer.
// In this manner transfers up toe 65536 sectors (or about 33Mbytes).
// See the function dma_pci_set_max_xfer().
//
//***********************************************************

//***********************************************************
//
// pci bus master registers and PRD list buffer,
// see dma_pci_config().
//
//***********************************************************

// public data...

int dma_pci_prd_type = PRD_TYPE_SIMPLE;                  // type of PRD list to use

//long dma_pci_largeMaxB;                // max LARGE dma xfer size in bytes
//long dma_pci_largeMaxS;                // max LARGE dma xfer size in sectors

void * dma_pci_prd_va;
physaddr_t dma_pci_prd_pa;


int dma_pci_num_prd;                   // current number of PRD entries

// private data...

// data used by SIMPLE/COMPLEX PRD lists

#define MAX_TRANSFER_SIZE  262144L     // max transfer size (in bytes,
                                       // should be multiple of 65536)

#define MAX_SEG ((MAX_TRANSFER_SIZE/65536L)+2L) // number physical segments
#define MAX_PRD (MAX_SEG*4L)                    // number of PRDs required

#define PRD_BUF_SIZE (48+(2*MAX_PRD*8))         // size of PRD list buffer

// BMIDE data

static unsigned char statReg;          // save BM status reg bits
static unsigned char rwControl;        // read/write control bit setting

//***********************************************************
//
// set_up_xfer() -- set up the PRD entry list
//
// NOTE:
// dma_pci_prd_type == PCI_TYPE_LARGE uses part of the caller's
// I/O buffer to hold a large PRD list and another part of the
// caller's I/O buffer to send/receive the actual data. Each
// PRD entry uses the same memory address.
//
//***********************************************************

//static int set_up_xfer( int dir, long bc, unsigned int seg, unsigned int off );

static int set_up_xfer( int dir, long bc, physaddr_t phyAddr )

{
    int numPrd;                      // number of PRD required
    int maxPrd;                      // max number of PRD allowed
    unsigned long temp;
    //unsigned long phyAddr;           // physical memory address
    unsigned long savePhyAddr;       // physical memory address
    unsigned long bigCnt;            // complex big count
    unsigned long smallCnt;          // complex small count

    u_int32_t * prdPtr;      // pointer to PRD entry list

    // disable/stop the dma channel, clear interrupt and error bits
    sub_writeBusMstrCmd( BM_CR_MASK_STOP );
    sub_writeBusMstrStatus( statReg | BM_SR_MASK_INT | BM_SR_MASK_ERR );

#if 0
    // setup to build the PRD list...
    if ( dma_pci_prd_type == PRD_TYPE_LARGE )
    {
        // ...set up for LARGE PRD list
        // ...max PRDs allowed
        maxPrd = 512;
        // ...set big and small counts to max
        bigCnt = smallCnt = 65536L;
        // ...set the LARGE PRD buffer address
        dma_pci_prd_ptr = prdPtr = dma_pci_largePrdBufPtr;
        // ...convert I/O buffer address to physical memory address
        //phyAddr = (unsigned long) FP_SEG( dma_pci_largeIoBufPtr );
        //phyAddr = phyAddr << 4;
        //phyAddr = phyAddr + (unsigned long) FP_OFF( dma_pci_largeIoBufPtr );
        //phyAddr = phyAddr & 0xfffffffeL;
    }
    else
#else
        if ( dma_pci_prd_type == PRD_TYPE_LARGE )
        {
            return 1;
        }
#endif

    {
        // ...set up for SIMPLE/COMPLEX PRD list
        // ...max PRDs allowed
        maxPrd = (int) MAX_PRD;
        // ...set big and small counts to max
        bigCnt = smallCnt = 65536L;
        // ...adjust PRD buffer address and adjust big and small counts
        prdPtr = dma_pci_prd_va; //prdBufPtr;
        /*
         if ( dma_pci_prd_type == PRD_TYPE_COMPLEX )
         {
         temp = tmr_read_bios_timer();
         prdPtr = MK_FP( FP_SEG( prdBufPtr ),
         (unsigned int) ( temp & 0x0000000cL ) );
         smallCnt = ( temp & 0x000000feL ) + 2L;
         bigCnt = bigCnt - smallCnt - ( temp & 0x0000000eL );
         }
         */
        // ...set the SIMPLE/COMPLEX PRD buffer address
        //dma_pci_prd_ptr = prdPtr;
        // ... convert I/O buffer address to an physical memory address
        //phyAddr = (unsigned long) seg;
        //phyAddr = phyAddr << 4;
        //phyAddr = phyAddr + (unsigned long) off;
        //phyAddr = phyAddr & 0xfffffffeL;
    }
    savePhyAddr = phyAddr;

#if DEBUG_PCI & 0x02
    printf( "z=>[prd] set_up_xfer()...\n" );
    printf( "z=>[prd] dir %d bc %lx seg %04x off %04x",
            dir, bc, seg, off );

    printf( "z=>[prd] maxPrd %d prdPtr %Fp bigCnt %lx smallCnt %lx",
            maxPrd, prdPtr, bigCnt, smallCnt );

    printf( "z=>[prd] phyAddr %08lx", phyAddr );

#endif

    // build the PRD list...
    // ...PRD entry format:
    // +0 to +3 = memory address
    // +4 to +5 = 0x0000 (not EOT) or 0x8000 (EOT)
    // +6 to +7 = byte count
    // ...zero number of PRDs
    numPrd = 0;
    // ...loop to build each PRD
    while ( bc > 0 )
    {
        if ( numPrd >= maxPrd )
            return 1;
#if 0
        // set this PRD's address
        if ( dma_pci_prd_type == PRD_TYPE_LARGE )
            phyAddr = savePhyAddr;
#endif

        prdPtr[0] = phyAddr;
        // set count for this PRD
        if ( ( numPrd & 0x0003 ) == 0x0002 )
            temp = bigCnt;       // use big count (1 of 4 PRDs)
        else                    // else
            temp = smallCnt;     // use small count (3 of 4 PRDs)
        if ( temp > (unsigned long)bc )        // count too large?
            temp = bc;           //    yes - use actual count
        // check if count will fit
        phyAddr += temp;
        if ( ( phyAddr & 0xffff0000L ) != ( prdPtr[0] & 0xffff0000L ) )
        {
            phyAddr = phyAddr & 0xffff0000L;
            temp = phyAddr - prdPtr[0];
        }
        // set this PRD's count
        prdPtr[1] = temp & 0x0000ffffL;
        // update byte count
        bc = bc - temp;
        // set the end bit in the prd list
        if ( bc < 1 )
            prdPtr[1] = prdPtr[1] | 0x80000000L;
        prdPtr ++ ;
        prdPtr ++ ;
        numPrd ++ ;
    }

    // return the current PRD list size and
    // set the prd list address in the BMIDE:
    // convert PRD buffer seg:off to a physical address
    // and write into BMIDE PRD address registers.

    //dma_pci_num_prd = numPrd;
    outw( pio_bmide_base_addr + BM_PRD_ADDR_LOW,
          (unsigned int) ( dma_pci_prd_pa & 0x0000ffffL ) );
    outw( pio_bmide_base_addr + BM_PRD_ADDR_HIGH,
          (unsigned int) (( dma_pci_prd_pa >> 16 ) & 0x0000ffffL ) );

#if 0 || DEBUG_PCI & 0x02
    {
        int ndx;
#warning long here meant 32 bit?
        //unsigned long * lfp;
        u_int32_t * lfp;

        printf( "z=>[prd] ----- Bus Master PRD List -----\n" );
        printf( "z=>[prd] PRD PhyAddr %08lX\n", dma_pci_prd_pa );
        //prt();
        lfp = dma_pci_prd_va;
        ndx = 0;
        while ( ndx < dma_pci_num_prd )
        {
            printf( "z=>[prd] PRD %2d - PhyAddr %08lX Cnt %08lX\n",
                    ndx, * lfp, * ( lfp + 1 ) );
            //prt();
            lfp ++ ;
            if (  ( * lfp ) & 0x80000000L )
                break;
            lfp ++ ;
            ndx ++ ;
        }
    }
#endif

    // set the read/write control:
    // PCI reads for ATA Write DMA commands,
    // PCI writes for ATA Read DMA commands.

    if ( dir )
        rwControl = BM_CR_MASK_READ;     // ATA Write DMA
    else
        rwControl = BM_CR_MASK_WRITE;    // ATA Read DMA
    sub_writeBusMstrCmd( rwControl );
    return 0;
}

//***********************************************************
//
// dma_pci_config() - configure/setup for Read/Write DMA
//
// The caller must call this function before attempting
// to use any ATA or ATAPI commands in PCI DMA mode.
//
//***********************************************************

int dma_pci_config( unsigned int regAddr )
{
    // check reg address

    if ( regAddr & 0x0007 )       // error if regs addr
        return 1;                  // are not xxx0h or xxx8h

    // save the base i/o address of the bus master (BMIDE) regs

    pio_bmide_base_addr = regAddr;

    // disable if reg address is zero

    if ( ! regAddr )              // if zero,
        return 0;                  // PCI DMA is disabled.

#if 1
    // TODO replace 4096 with native page size definition
#if 0
    dma_pci_prd_va = smemalign( 4096, PRD_BUF_SIZE );
    dma_pci_prd_pa = kvtophys(dma_pci_prd_va);
#else

    /*
    {
        int npages = ((PRD_BUF_SIZE-1)/PAGE_SIZE) + 1;

        if( hal_alloc_vaddress(&dma_pci_prd_va, npages) )
            panic("out of vaddr");

        if( hal_alloc_phys_pages( &dma_pci_prd_pa, npages) )
        {
            //hal_free_vaddress( dma_pci_prd_va, npages );
            panic("out of physmem");
        }

        hal_pages_control( dma_pci_prd_pa, dma_pci_prd_va, npages, page_map, page_rw );
    }
    */
    hal_pv_alloc( &dma_pci_prd_pa, &dma_pci_prd_va, PRD_BUF_SIZE );

#endif

#else
    unsigned u_int16_t off;
    unsigned u_int16_t seg;
    //unsigned long lw;
    unsigned u_int32_t lw;

    // Set up the PRD entry list buffer address - the PRD entry list
    // may not span a 64KB boundary in physical memory. Space is
    // allocated (above) for this buffer such that it will be
    // aligned on a seqment boundary (off of seg:off will be 0)
    // and such that the PRD list will not span a 64KB boundary...
    // ...convert seg:off to physical address.
    seg = FP_SEG( (unsigned char *) prdBuf );
    off = FP_OFF( (unsigned char *) prdBuf );
    lw = (u_int32_t) seg;
    lw = lw << 4;
    lw = lw + (u_int32_t) off;
    // ...move up to a segment boundary.
    lw = lw + 15;
    lw = lw & 0xfffffff0L;
    // ...check for 64KB boundary in the first part of the PRD buffer,
    // ...if so just move the buffer to that boundary.
    if ( ( lw & 0xffff0000L )
         !=
         ( ( lw + ( MAX_PRD * 8L ) - 1L ) & 0xffff0000L )
       )
        lw = ( lw + ( MAX_PRD * 8L ) ) & 0xffff0000L;
    // ...convert back to seg:off, note that off is now 0
    seg = (u_int16_t) ( lw >> 4 );
    dma_pci_prd_ptr = prdBufPtr = MK_FP( seg, 0 );
#endif

    // ... current size of the SIMPLE/COMPLEX PRD buffer
    //dma_pci_num_prd = 0;

    // read the BM status reg and save the upper 3 bits.
    statReg = sub_readBusMstrStatus() & 0x60;

    // initialize the large PRD list info
    //dma_pci_largePrdBufPtr = (void *) 0;
    //dma_pci_largeIoBufPtr = (void *) 0;
    //dma_pci_largeMaxB = 0;
    //dma_pci_largeMaxS = 0;

    return 0;
}
#if 0
//***********************************************************
//
// determine max dma transfer for PRD type LARGE
//
// dma_pci_prd_type == PCI_TYPE_LARGE uses part of the caller's
// I/O buffer to hold a large PRD list and another part of the
// caller's I/O buffer to send/receive the actual data. Each
// PRD entry uses the same memory address.
//
//***********************************************************

void dma_pci_set_max_xfer( unsigned int seg, unsigned int off,
                           long bufSize )
{
    u_int32_t bufStart;       // buffer starting physical memory address
    u_int32_t bufEnd;         // buffer ending physical memory address (+1)
    u_int32_t pmaStart;       // start of 64K area within buffer
    u_int32_t pmaEnd;         // end of 64K area within buffer

    // save buffer size
    reg_buffer_size = bufSize;

    // large DMA xfers not supported
    dma_pci_largePrdBufPtr = (void *) 0;
    dma_pci_largeIoBufPtr = (void *) 0;
    //dma_pci_largeMaxB = 0L;
    //dma_pci_largeMaxS = 0L;

    // convert I/O buffer address from seg:off to an absolute memory address
    // note: the physical address must be a word boundary (an even number).
    bufStart = (unsigned long) seg;
    bufStart = bufStart << 4;
    bufStart = bufStart + (unsigned long) off;
    bufStart = bufStart & 0xfffffffeL;
    // move up to dword boundary
    bufStart = ( bufStart + 15L ) & 0xfffffff0;
    // compute end
    bufEnd = bufStart + bufSize;
    // compute start/end of 64K area (pma)
    // that will be the I/O buffer
    pmaStart = ( bufStart + 0x00010000L ) & 0xffff0000;
    pmaEnd = pmaStart + 0x00010000L;

    // if pma is in buffer then...
    if ( ( pmaStart < bufEnd ) && ( pmaEnd <= bufEnd ) )
    {
        // ...find location for PRD list
        if ( ( pmaStart - bufStart ) >= 4096L )
        {
            // PRD buffer first (I/O buffer second)
            dma_pci_largePrdBufPtr = MK_FP( (unsigned int) ( bufStart >> 4 ), 0 );
        }
        else
            if ( ( bufEnd - pmaEnd ) >= 4096L )
            {
                // PRD buffer second (I/O buffer first)
                dma_pci_largePrdBufPtr = MK_FP( (unsigned int) ( pmaEnd >> 4 ), 0 );
            }
        if ( dma_pci_largePrdBufPtr )
        {
            dma_pci_largeIoBufPtr = MK_FP( (unsigned int) ( pmaStart >> 4 ), 0 );
            //dma_pci_largeMaxB = 65536L * 512L;
            //dma_pci_largeMaxS = 65536L;
        }
    }

#if DEBUG_PCI & 0x02
    pstr( "z=>[dma] dma_pci_set_max_xfer()..." );
    sprintf( LFB, "z=>[dma] seg %04x off %04x bufSize %lx", seg, off, bufSize );
    prt();
    sprintf( LFB, "z=>[dma] bufStart %lx bufEnd %lx", bufStart, bufEnd );
    prt();
    sprintf( LFB, "z=>[dma] pmaStart %lx pmaEnd %lx", pmaStart, pmaEnd );
    prt();
    sprintf( LFB, "z=>[dma] largePrdBufPtr %Fp largeIoBufPtr %Fp",
             dma_pci_largePrdBufPtr, dma_pci_largeIoBufPtr );
    prt();
    //sprintf( LFB, "z=>[dma] dma_pci_largeMaxB %ld dma_pci_largeMaxS %ld",                             dma_pci_largeMaxB, dma_pci_largeMaxS );
    prt();
#endif

}
#endif
//***********************************************************
//
// exec_pci_ata_cmd() - PCI Bus Master for ATA R/W DMA commands
//
//***********************************************************

static int exec_pci_ata_cmd( int dev, physaddr_t physAddr,
                             //                             unsigned int seg, unsigned int off,
                             long numSect );

static int exec_pci_ata_cmd( int dev, physaddr_t physAddr,
                             //                             unsigned int seg, unsigned int off,
                             long numSect )

{
    unsigned int cntr;
    unsigned char status;
    long lw;

    // mark start of a R/W DMA command in low level trace

    trc_llt( 0, 0, TRC_LLT_S_RWD );

    // Quit now if no dma channel set up
    // or interrupts are not enabled.

    if ( ( ! pio_bmide_base_addr ) || ( ! int_use_intr_flag ) )
    {
        reg_cmd_info.ec = 70;
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        sub_trace_command();
        trc_llt( 0, 0, TRC_LLT_E_RWD );
        return 1;
    }

    // Quit now if 1) I/O buffer overrun possible.
    // or 2) DMA can't handle the transfer size.

    lw = numSect * 512L;
    if ( (    ( dma_pci_prd_type != PRD_TYPE_LARGE )
              && ( ( lw > MAX_TRANSFER_SIZE ) || ( lw > reg_buffer_size ) ) )
         ||
         (    ( dma_pci_prd_type == PRD_TYPE_LARGE )
              //&& ( lw > dma_pci_largeMaxB )
         )
       )
    {
        reg_cmd_info.ec = 61;
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        sub_trace_command();
        trc_llt( 0, 0, TRC_LLT_E_PID );
        return 1;
    }

    // Set up the dma transfer

    if ( set_up_xfer(    ( reg_cmd_info.cmd == CMD_WRITE_DMA )
                         || ( reg_cmd_info.cmd == CMD_WRITE_DMA_EXT )
                         || ( reg_cmd_info.cmd == CMD_WRITE_DMA_FUA_EXT ),
                         numSect * 512L, physAddr ) )
    {
        reg_cmd_info.ec = 61;
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        sub_trace_command();
        trc_llt( 0, 0, TRC_LLT_E_PID );
        return 1;
    }

    // Set command time out.

    tmr_set_timeout();

    // Select the drive - call the sub_select function.
    // Quit now if this fails.

    if ( sub_select( dev ) )
    {
        sub_trace_command();
        trc_llt( 0, 0, TRC_LLT_E_RWD );
        return 1;
    }

    // Set up all the registers except the command register.

    sub_setup_command();

    // For interrupt mode, install interrupt handler.

    int_save_int_vect();

    // Start the command by setting the Command register.  The drive
    // should immediately set BUSY status.

    pio_outbyte( CB_CMD, reg_cmd_info.cmd );

    // The drive should start executing the command including any
    // data transfer.

    // Data transfer...
    // read the BMIDE regs
    // enable/start the dma channel.
    // read the BMIDE regs again

    sub_readBusMstrCmd();
    sub_readBusMstrStatus();
    sub_writeBusMstrCmd( rwControl | BM_CR_MASK_START );
    sub_readBusMstrCmd();
    sub_readBusMstrStatus();

    // Data transfer...
    // the device and dma channel transfer the data here while we start
    // checking for command completion...
    // wait for the PCI BM Interrupt=1 (see ATAIOINT.C)...

    trc_llt( 0, 0, TRC_LLT_WINT );
    cntr = 0;
    while ( 1 )
    {
        cntr ++ ;
        if ( ! ( cntr & 0x1fff ) )
        {
            sub_readBusMstrStatus();         // read BM status (for trace)
            if ( ! ( reg_incompat_flags & REG_INCOMPAT_DMA_POLL ) )
                pio_inbyte( CB_ASTAT );       // poll Alt Status
        }
        if ( int_intr_flag )                // interrupt ?
        {
            trc_llt( 0, 0, TRC_LLT_INTRQ );  // yes
            trc_llt( 0, int_bm_status, TRC_LLT_R_BM_SR );
            trc_llt( CB_STAT, int_ata_status, TRC_LLT_INB );
            trc_llt( 0, 0x04, TRC_LLT_W_BM_SR );
            break;
        }
        if ( tmr_chk_timeout() )            // time out ?
        {
            trc_llt( 0, 0, TRC_LLT_TOUT );   // yes
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 73;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;
        }
    }

    if ( reg_incompat_flags & REG_INCOMPAT_DMA_DELAY )
    {
        tmr_delay_1ms( 1L );    // delay for buggy controllers
    }

    // End of command...
    // disable/stop the dma channel

    status = int_bm_status;                // read BM status
    status &= ~ BM_SR_MASK_ACT;            // ignore Active bit
    sub_writeBusMstrCmd( BM_CR_MASK_STOP );    // shutdown DMA
    sub_readBusMstrCmd();                      // read BM cmd (just for trace)
    status |= sub_readBusMstrStatus();         // read BM status again

    if ( reg_incompat_flags & REG_INCOMPAT_DMA_DELAY )
    {
        tmr_delay_1ms( 1L );    // delay for buggy controlers
    }

    if ( reg_cmd_info.ec == 0 )
    {
        if ( status & BM_SR_MASK_ERR )            // bus master error?
        {
            reg_cmd_info.ec = 78;                  // yes
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        }
    }
    if ( reg_cmd_info.ec == 0 )
    {
        if ( status & BM_SR_MASK_ACT )            // end of PRD list?
        {
            reg_cmd_info.ec = 71;                  // no
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        }
    }

#if DEBUG_PCI & 0x01
    trc_llt( 0, int_intr_cntr, TRC_LLT_DEBUG );  // for debugging
#endif

    // End of command...
    // If no error use the Status register value that was read
    // by the interrupt handler. If there was an error
    // read the Status register because it may not have been
    // read by the interrupt handler.

    if ( reg_cmd_info.ec )
        status = pio_inbyte( CB_STAT );
    else
        status = int_ata_status;

    // Final status check...
    // if no error, check final status...
    // Error if BUSY, DEVICE FAULT, DRQ or ERROR status now.

    if ( reg_cmd_info.ec == 0 )
    {
        if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_DRQ | CB_STAT_ERR ) )
        {
            reg_cmd_info.ec = 74;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        }
    }

    // Final status check...
    // if any error, update total bytes transferred.

    if ( reg_cmd_info.ec == 0 )
        reg_cmd_info.totalBytesXfer = numSect * 512L;
    else
        reg_cmd_info.totalBytesXfer = 0L;

    // Done...
    // Read the output registers and trace the command.

    sub_trace_command();

    // Done...
    // For interrupt mode, remove interrupt handler.

    int_restore_int_vect();

    // Done...
    // mark end of a R/W DMA command in low level trace

    trc_llt( 0, 0, TRC_LLT_E_RWD );

    // All done.  The return values of this function are described in
    // ATAIO.H.

    if ( reg_cmd_info.ec )
        return 1;
    return 0;
}

#if 0

//***********************************************************
//
// dma_pci_chs() - PCI Bus Master for ATA R/W DMA commands
//
//***********************************************************

int dma_pci_chs( int dev, int cmd,
                 unsigned int fr, unsigned int sc,
                 unsigned int cyl, unsigned int head, unsigned int sect,
                 unsigned int seg, unsigned int off,
                 long numSect )
{
    // Setup current command information.

    sub_zero_return_data();
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_ADMAI;
    if (    ( cmd == CMD_WRITE_DMA )
            || ( cmd == CMD_WRITE_DMA_EXT )
            || ( cmd == CMD_WRITE_DMA_FUA_EXT )
       )
        reg_cmd_info.ct  = TRC_TYPE_ADMAO;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.sn1 = sect;
    reg_cmd_info.cl1 = cyl & 0x00ff;
    reg_cmd_info.ch1 = ( cyl & 0xff00 ) >> 8;
    reg_cmd_info.dh1 = ( dev ? CB_DH_DEV1 : CB_DH_DEV0 ) | ( head & 0x0f );
    reg_cmd_info.dc1 = 0x00;      // nIEN=0 required on PCI !
    reg_cmd_info.ns  = numSect;
    reg_cmd_info.lbaSize = LBACHS;

    // Execute the command.

    return exec_pci_ata_cmd( dev, seg, off, numSect );
}
#endif

//***********************************************************
//
// dma_pci_lba28() - DMA in PCI Multiword for ATA R/W DMA
//
//***********************************************************

int dma_pci_lba28( int dev, int cmd,
                   unsigned int fr, unsigned int sc,
                   unsigned long lba,
                   //unsigned int seg, unsigned int off,
                   physaddr_t addr, // NB! physaddr?
                   long numSect )

{

    // Setup current command information.

    sub_zero_return_data();
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_ADMAI;
    if (    ( cmd == CMD_WRITE_DMA )
            || ( cmd == CMD_WRITE_DMA_EXT )
            || ( cmd == CMD_WRITE_DMA_FUA_EXT )
       )
        reg_cmd_info.ct  = TRC_TYPE_ADMAO;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.dh1 = CB_DH_LBA | (dev ? CB_DH_DEV1 : CB_DH_DEV0 );
    reg_cmd_info.dc1 = 0x00;      // nIEN=0 required on PCI !
    reg_cmd_info.ns  = numSect;
    reg_cmd_info.lbaSize = LBA28;
    reg_cmd_info.lbaHigh1 = 0L;
    reg_cmd_info.lbaLow1 = lba;

    // Execute the command.

    return exec_pci_ata_cmd( dev, (int)addr, numSect );
}


//***********************************************************
//
// dma_pci_lba48() - DMA in PCI Multiword for ATA R/W DMA
//
//***********************************************************

int dma_pci_lba48( int dev, int cmd,
                   unsigned int fr, unsigned int sc,
                   unsigned long lbahi, unsigned long lbalo,
                   //unsigned int seg, unsigned int off,
                   physaddr_t physAddr,
                   long numSect )

{

    // Setup current command information.

    sub_zero_return_data();
    reg_cmd_info.flg = TRC_FLAG_ATA;
    reg_cmd_info.ct  = TRC_TYPE_ADMAI;
    if (    ( cmd == CMD_WRITE_DMA )
            || ( cmd == CMD_WRITE_DMA_EXT )
            || ( cmd == CMD_WRITE_DMA_FUA_EXT )
       )
        reg_cmd_info.ct  = TRC_TYPE_ADMAO;
    reg_cmd_info.cmd = cmd;
    reg_cmd_info.fr1 = fr;
    reg_cmd_info.sc1 = sc;
    reg_cmd_info.dh1 = CB_DH_LBA | (dev ? CB_DH_DEV1 : CB_DH_DEV0 );
    reg_cmd_info.dc1 = 0x00;      // nIEN=0 required on PCI !
    reg_cmd_info.ns  = numSect;
    reg_cmd_info.lbaSize = LBA48;
    reg_cmd_info.lbaHigh1 = lbahi;
    reg_cmd_info.lbaLow1 = lbalo;

    // Execute the command.

    return exec_pci_ata_cmd( dev, physAddr, numSect );
}

//***********************************************************
//
// dma_pci_packet() - PCI Bus Master for ATAPI Packet command
//
//***********************************************************

int dma_pci_packet( int dev,
                    unsigned int cpbc,
                    //unsigned int cpseg, unsigned int cpoff,
                    unsigned char * cpAddr,
                    int dir,
                    long dpbc,
                    physaddr_t dp_physAddr,
                    //unsigned int dpseg, unsigned int dpoff,
                    unsigned long lba )

{
    unsigned char status;
    unsigned char reason;
    unsigned char lowCyl;
    unsigned char highCyl;
    unsigned int cntr;
    unsigned int ndx;
    unsigned char  * cfp;

    // mark start of isa dma PI cmd in low level trace

    trc_llt( 0, 0, TRC_LLT_S_PID );

    // Make sure the command packet size is either 12 or 16
    // and save the command packet size and data.

    cpbc = cpbc < 12 ? 12 : cpbc;
    cpbc = cpbc > 12 ? 16 : cpbc;

    // Setup current command information.

    sub_zero_return_data();
    reg_cmd_info.flg = TRC_FLAG_ATAPI;
    reg_cmd_info.ct  = dir ? TRC_TYPE_PDMAO : TRC_TYPE_PDMAI;
    reg_cmd_info.cmd = CMD_PACKET;
    reg_cmd_info.fr1 = reg_atapi_reg_fr | 0x01;  // packet DMA mode !
    reg_cmd_info.sc1 = reg_atapi_reg_sc;
    reg_cmd_info.sn1 = reg_atapi_reg_sn;
    reg_cmd_info.cl1 = 0;         // no Byte Count Limit in DMA !
    reg_cmd_info.ch1 = 0;         // no Byte Count Limit in DMA !
    reg_cmd_info.dh1 = dev ? CB_DH_DEV1 : CB_DH_DEV0;
    reg_cmd_info.dc1 = 0x00;      // nIEN=0 required on PCI !
    reg_cmd_info.lbaSize = LBA32;
    reg_cmd_info.lbaLow1 = lba;
    reg_cmd_info.lbaHigh1 = 0L;
    reg_atapi_cp_size = cpbc;
    cfp = cpAddr;
    for ( ndx = 0; ndx < cpbc; ndx ++ )
    {
        reg_atapi_cp_data[ndx] = * cfp;
        cfp ++ ;
    }

    // Zero the alternate ATAPI register data.

    reg_atapi_reg_fr = 0;
    reg_atapi_reg_sc = 0;
    reg_atapi_reg_sn = 0;
    reg_atapi_reg_dh = 0;

    // Quit now if no dma channel set up

    if ( ! pio_bmide_base_addr )
    {
        reg_cmd_info.ec = 70;
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        sub_trace_command();
        trc_llt( 0, 0, TRC_LLT_E_PID );
        return 1;
    }

    // the data packet byte count must be even
    // and must not be zero

    if ( dpbc & 1L )
        dpbc ++ ;
    if ( dpbc < 2L )
        dpbc = 2L;

    // Quit now if 1) I/O buffer overrun possible
    // or 2) DMA can't handle the transfer size.

    if ( (    ( dma_pci_prd_type != PRD_TYPE_LARGE )
              && ( ( dpbc > MAX_TRANSFER_SIZE ) || ( dpbc > reg_buffer_size ) ) )
         ||
         (    ( dma_pci_prd_type == PRD_TYPE_LARGE )
              //&& ( dpbc > dma_pci_largeMaxB )
         )
       )
    {
        reg_cmd_info.ec = 61;
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        sub_trace_command();
        trc_llt( 0, 0, TRC_LLT_E_PID );
        return 1;
    }

    // Set up the dma transfer

    if ( set_up_xfer( dir, dpbc, dp_physAddr ) )
    {
        reg_cmd_info.ec = 61;
        trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        sub_trace_command();
        trc_llt( 0, 0, TRC_LLT_E_PID );
        return 1;
    }

    // Set command time out.

    tmr_set_timeout();

    // Select the drive - call the reg_select function.
    // Quit now if this fails.

    if ( sub_select( dev ) )
    {
        sub_trace_command();
        trc_llt( 0, 0, TRC_LLT_E_PID );
        return 1;
    }

    // Set up all the registers except the command register.

    sub_setup_command();

    // For interrupt mode, install interrupt handler.

    int_save_int_vect();

    // Start the command by setting the Command register.  The drive
    // should immediately set BUSY status.

    pio_outbyte( CB_CMD, CMD_PACKET );

    // Waste some time by reading the alternate status a few times.
    // This gives the drive time to set BUSY in the status register on
    // really fast systems.  If we don't do this, a slow drive on a fast
    // system may not set BUSY fast enough and we would think it had
    // completed the command when it really had not started the
    // command yet.

    ATA_DELAY();

    // Command packet transfer...
    // Check for protocol failures,
    // the device should have BSY=1 or
    // if BSY=0 then either DRQ=1 or CHK=1.

    ATAPI_DELAY( dev );
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
            reg_cmd_info.ec = 75;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;
        }
    }

    // Command packet transfer...
    // Check for protocol failures... no interrupt here please!
    // Clear any interrupt the command packet transfer may have caused.

    if ( int_intr_flag )    // extra interrupt(s) ?
        reg_cmd_info.failbits |= FAILBIT1;
    int_intr_flag = 0;

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
            reg_cmd_info.ec = 76;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
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

            trc_llt( 0, *cpAddr, TRC_LLT_P_CMD );
            // TODO is it a PHYSICAL ADDRESS?
            pio_drq_block_out( CB_DATA, cpAddr, cpbc >> 1 );
        }
    }

    // Data transfer...
    // The drive should start executing the command
    // including any data transfer.
    // If no error, set up and start the DMA,
    // and wait for the DMA to complete.

    if ( reg_cmd_info.ec == 0 )
    {

        // Data transfer...
        // read the BMIDE regs
        // enable/start the dma channel.
        // read the BMIDE regs again

        sub_readBusMstrCmd();
        sub_readBusMstrStatus();
        sub_writeBusMstrCmd( rwControl | BM_CR_MASK_START );
        sub_readBusMstrCmd();
        sub_readBusMstrStatus();

        // Data transfer...
        // the device and dma channel transfer the data here while we start
        // checking for command completion...
        // wait for the PCI BM Active=0 and Interrupt=1 or PCI BM Error=1...

        trc_llt( 0, 0, TRC_LLT_WINT );
        cntr = 0;
        while ( 1 )
        {
            cntr ++ ;
            if ( ! ( cntr & 0x1fff) )
            {
                sub_readBusMstrStatus();         // read BM status (for trace)
                if ( ! ( reg_incompat_flags & REG_INCOMPAT_DMA_POLL ) )
                    pio_inbyte( CB_ASTAT );       // poll Alt Status
            }
            if ( int_intr_flag )                // interrupt ?
            {
                trc_llt( 0, 0, TRC_LLT_INTRQ );  // yes
                trc_llt( 0, int_bm_status, TRC_LLT_R_BM_SR );
                trc_llt( CB_STAT, int_ata_status, TRC_LLT_INB );
                trc_llt( 0, 0x04, TRC_LLT_W_BM_SR );
                break;
            }
            if ( tmr_chk_timeout() )            // time out ?
            {
                trc_llt( 0, 0, TRC_LLT_TOUT );   // yes
                reg_cmd_info.to = 1;
                reg_cmd_info.ec = 73;
                trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
                break;
            }
        }

        if ( reg_incompat_flags & REG_INCOMPAT_DMA_DELAY )
        {
            tmr_delay_1ms( 1L);     // delay for buggy controllers
        }

        // End of command...
        // disable/stop the dma channel

        status = int_bm_status;                // read BM status
        status &= ~ BM_SR_MASK_ACT;            // ignore Active bit
        sub_writeBusMstrCmd( BM_CR_MASK_STOP );    // shutdown DMA
        sub_readBusMstrCmd();                      // read BM cmd (just for trace)
        status |= sub_readBusMstrStatus();         // read BM statu again

        if ( reg_incompat_flags & REG_INCOMPAT_DMA_DELAY )
        {
            tmr_delay_1ms( 1L );    // delay for buggy controllers
        }
    }

    if ( reg_cmd_info.ec == 0 )
    {
        if ( status & ( BM_SR_MASK_ERR ) )        // bus master error?
        {
            reg_cmd_info.ec = 78;                  // yes
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        }
        if ( ( status & BM_SR_MASK_ACT ) )        // end of PRD list?
        {
            reg_cmd_info.ec = 71;                  // no
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        }
    }

#if DEBUG_PCI & 0x01
    trc_llt( 0, int_intr_cntr, TRC_LLT_DEBUG );  // for debugging
#endif

    // End of command...
    // If no error use the Status register value that was read
    // by the interrupt handler. If there was an error
    // read the Status register because it may not have been
    // read by the interrupt handler.

    if ( reg_cmd_info.ec )
        status = pio_inbyte( CB_STAT );
    else
        status = int_ata_status;

    // Final status check...
    // if no error, check final status...
    // Error if BUSY, DRQ or ERROR status now.

    if ( reg_cmd_info.ec == 0 )
    {
        if ( status & ( CB_STAT_BSY | CB_STAT_DRQ | CB_STAT_ERR ) )
        {
            reg_cmd_info.ec = 74;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
        }
    }

    // Final status check...
    // Check for protocol failures...
    // check: C/nD=1, IO=1.

    reason = pio_inbyte( CB_SC );
    if ( ( reason & ( CB_SC_P_TAG | CB_SC_P_REL ) )
         || ( ! ( reason & CB_SC_P_IO ) )
         || ( ! ( reason & CB_SC_P_CD ) )
       )
        reg_cmd_info.failbits |= FAILBIT8;

    // Final status check...
    // if any error, update total bytes transferred.

    if ( reg_cmd_info.ec == 0 )
        reg_cmd_info.totalBytesXfer = dpbc;
    else
        reg_cmd_info.totalBytesXfer = 0L;

    // Done...
    // Read the output registers and trace the command.

    sub_trace_command();

    // Done...
    // For interrupt mode, remove interrupt handler.

    int_restore_int_vect();

    // Done...
    // mark end of isa dma PI cmd in low level trace

    trc_llt( 0, 0, TRC_LLT_E_PID );

    // All done.  The return values of this function are described in
    // ATAIO.H.

    if ( reg_cmd_info.ec )
        return 1;
    return 0;
}

// end ataiopci.c
