#if 0
//#define __inline /**/
//#define inline /**/


//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOPIO.C
//
// by Hale Landis (hlandis@ata-atapi.com)
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
// This module contains inline assembler code so you'll
// also need Borland's Assembler.
//
// This C source contains the low level I/O port IN/OUT functions.
//********************************************************************


#include "ataio.h"

#include <ia32/pio.h>

//*************************************************************
//
// Host adapter base addresses.
//
//*************************************************************

//unsigned int pio_base_addr1 = 0x1f0;
//unsigned int pio_base_addr2 = 0x3f0;

//unsigned char pio_memory_access = 0; // do we access IDE through memory
//unsigned int pio_memory_seg = 0;
int pio_memory_dt_opt = PIO_MEMORY_DT_OPT0;

//unsigned int pio_bmide_base_addr = 0;

//void * pio_reg_addrs[10];

//unsigned char pio_last_write[10];
//unsigned char pio_last_read[10];

#if HAVE_WIDE_XFERS
//int pio_xfer_width = 16;
#else
//int pio_xfer_width = 8;
#endif

//*************************************************************
//
// Set the host adapter i/o base addresses.
//
//*************************************************************

void pio_set_iobase_addr( ataio_t *ata, unsigned int base1,
                          unsigned int base2,
                          unsigned int base3 )
{

    int pio_base_addr1 = base1;
    int pio_base_addr2 = base2;

    ata->pio_base_addr1 = base1;
    ata->pio_base_addr2 = base2;
    ata->pio_bmide_base_addr = base3;
    //ata->pio_memory_access = 0;
    ata->pio_reg_addrs[ CB_DATA ] = (void *)pio_base_addr1 + 0;  // 0
    ata->pio_reg_addrs[ CB_FR   ] = (void *)pio_base_addr1 + 1;  // 1
    ata->pio_reg_addrs[ CB_SC   ] = (void *)pio_base_addr1 + 2;  // 2
    ata->pio_reg_addrs[ CB_SN   ] = (void *)pio_base_addr1 + 3;  // 3
    ata->pio_reg_addrs[ CB_CL   ] = (void *)pio_base_addr1 + 4;  // 4
    ata->pio_reg_addrs[ CB_CH   ] = (void *)pio_base_addr1 + 5;  // 5
    ata->pio_reg_addrs[ CB_DH   ] = (void *)pio_base_addr1 + 6;  // 6
    ata->pio_reg_addrs[ CB_CMD  ] = (void *)pio_base_addr1 + 7;  // 7
    ata->pio_reg_addrs[ CB_DC   ] = (void *)pio_base_addr2 + 6;  // 8
    ata->pio_reg_addrs[ CB_DA   ] = (void *)pio_base_addr2 + 7;  // 9
}

//*************************************************************
//
// Set the host adapter memory base addresses.
//
//*************************************************************
#if 0
void pio_set_memory_addr( void * base )

{

    pio_base_addr1 = (void *)base + 0;
    pio_base_addr2 = (void *)base + 8;
    pio_bmide_base_addr = 0;
    //pio_memory_seg = seg;
    pio_memory_access = 1;
    pio_memory_dt_opt = PIO_MEMORY_DT_OPT0;
    pio_reg_addrs[ CB_DATA ] = (void *)pio_base_addr1 + 0;  // 0
    pio_reg_addrs[ CB_FR   ] = (void *)pio_base_addr1 + 1;  // 1
    pio_reg_addrs[ CB_SC   ] = (void *)pio_base_addr1 + 2;  // 2
    pio_reg_addrs[ CB_SN   ] = (void *)pio_base_addr1 + 3;  // 3
    pio_reg_addrs[ CB_CL   ] = (void *)pio_base_addr1 + 4;  // 4
    pio_reg_addrs[ CB_CH   ] = (void *)pio_base_addr1 + 5;  // 5
    pio_reg_addrs[ CB_DH   ] = (void *)pio_base_addr1 + 6;  // 6
    pio_reg_addrs[ CB_CMD  ] = (void *)pio_base_addr1 + 7;  // 7
    pio_reg_addrs[ CB_DC   ] = (void *)pio_base_addr2 + 6;  // 8
    pio_reg_addrs[ CB_DA   ] = (void *)pio_base_addr2 + 7;  // 9
}
#endif
//*************************************************************
//
// These functions do basic IN/OUT of byte and word values:
//
//    pio_inbyte()
//    pio_outbyte()
//    pio_inword()
//    pio_outword()
//
//*************************************************************

/*
unsigned char pio_inbyte( unsigned int addr )

{
    unsigned int regAddr;
    unsigned char uc;

    regAddr = (int)pio_reg_addrs[ addr ];
#if 0
    if ( pio_memory_seg )
    {
        unsigned char * ucp;
        ucp = (unsigned char *) MK_FP( pio_memory_seg, regAddr );
        uc = * ucp;
    }
    else
#endif
    {
        uc = (unsigned char) inb( regAddr );
    }
    pio_last_read[ addr ] = uc;
    trc_llt( addr, uc, TRC_LLT_INB );
    return uc;
    }
*/

//*************************************************************
/*
void pio_outbyte( unsigned int addr, unsigned char data )

{

#if 0
    if ( pio_memory_access )
    {
        //unsigned int regAddr;
        unsigned char * ucp;
        ucp = pio_reg_addrs[ addr ];
        * ucp = data;
    }
    else
#endif
    {
        outb( (int)pio_reg_addrs[ addr ], data );
    }
    pio_last_write[ addr ] = data;
    trc_llt( addr, data, TRC_LLT_OUTB );
}
*/
//*************************************************************

/*
unsigned int pio_inword( unsigned int addr )
{
    unsigned int regAddr;
    unsigned int ui;

    regAddr = (int)pio_reg_addrs[ addr ];
#if 0
    if ( pio_memory_access )
    {
        unsigned int * uip;
        uip = regAddr;
        ui = * uip;
    }
    else
#endif
    {
        ui = inw( regAddr );
    }
    trc_llt( addr, 0, TRC_LLT_INW );
    return ui;
}
*/
//*************************************************************
/*
void pio_outword( unsigned int addr, unsigned int data )
{
#if 0
    unsigned int regAddr;
    unsigned int * uip;

    regAddr = (int)pio_reg_addrs[ addr ];

    if( pio_memory_access )
    {
        uip = regAddr;
        *uip = data;
    }
    else
#endif
    {
        outw( (int)pio_reg_addrs[ addr ], data );
    }
    trc_llt( addr, 0, TRC_LLT_OUTW );
}
*/
//*************************************************************
//
// These functions are normally used to transfer DRQ blocks:
//
// pio_drq_block_in()
// pio_drq_block_out()
//
//*************************************************************

// Note: pio_drq_block_in() is the primary way perform PIO
// Data In transfers. It will handle 8-bit, 16-bit and 32-bit
// I/O based data transfers and 8-bit and 16-bit PCMCIA Memory
// mode transfers.

void pio_drq_block_in( ataio_t *ata, unsigned int addrDataReg,
                       void *bufAddr,
                       long wordCnt )

{
    //unsigned long bufAddr;

    // NOTE: wordCnt is the size of a DRQ data block/packet
    // in words. The maximum value of wordCnt is normally:
    // a) For ATA, 16384 words or 32768 bytes (64 sectors,
    //    only with READ/WRITE MULTIPLE commands),
    // b) For ATAPI, 32768 words or 65536 bytes
    //    (actually 65535 bytes plus a pad byte).

    // normalize bufSeg:bufOff

    //bufAddr = bufSeg;
    //bufAddr = bufAddr << 4;
    //bufAddr = bufAddr + bufOff;

#if 0
    if ( pio_memory_access )
    {
        long bCnt;
        int memDtOpt;
        unsigned int randVal;
        unsigned int dataRegAddr;
        unsigned int * uip1;
        unsigned int * uip2;
        unsigned char * ucp1;
        unsigned char * ucp2;

        // PCMCIA Memory mode data transfer.

        // set Data reg address per pio_memory_dt_opt
        dataRegAddr = 0x0000;
        memDtOpt = pio_memory_dt_opt;
        if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
        {
            randVal = * (unsigned int *) MK_FP( 0x40, 0x6c );
            memDtOpt = randVal % 3;
        }
        if ( memDtOpt == PIO_MEMORY_DT_OPT8 )
            dataRegAddr = 0x0008;
        if ( memDtOpt == PIO_MEMORY_DT_OPTB )
        {
            dataRegAddr = 0x0400;
            if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
                dataRegAddr = dataRegAddr | ( randVal & 0x03fe );
        }

        if ( pio_xfer_width == 8 )
        {
            // PCMCIA Memory mode 8-bit
            bCnt = wordCnt * 2L;
            ucp1 = (unsigned char *) dataRegAddr;
            for ( ; bCnt > 0; bCnt -- )
            {
                bufSeg = (unsigned int) ( bufAddr >> 4 );
                bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
                ucp2 = (unsigned char *) MK_FP( bufSeg, bufOff );
                * ucp2 = * ucp1;
                bufAddr += 1;
                if ( memDtOpt == PIO_MEMORY_DT_OPTB )
                {
                    dataRegAddr += 1;
                    dataRegAddr = ( dataRegAddr & 0x03ff ) | 0x0400;
                    ucp1 = (unsigned char *) MK_FP( pio_memory_seg, dataRegAddr );
                }
            }
            trc_llt( addrDataReg, 0, TRC_LLT_INSB );
        }
        else
        {
            // PCMCIA Memory mode 16-bit
            uip1 = (unsigned int *) MK_FP( pio_memory_seg, dataRegAddr );
            for ( ; wordCnt > 0; wordCnt -- )
            {
                bufSeg = (unsigned int) ( bufAddr >> 4 );
                bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
                uip2 = (unsigned int *) MK_FP( bufSeg, bufOff );
                * uip2 = * uip1;
                bufAddr += 2;
                if ( memDtOpt == PIO_MEMORY_DT_OPTB )
                {
                    dataRegAddr += 2;
                    dataRegAddr = ( dataRegAddr & 0x03fe ) | 0x0400;
                    uip1 = (unsigned int *) MK_FP( pio_memory_seg, dataRegAddr );
                }
            }
            trc_llt( addrDataReg, 0, TRC_LLT_INSW );
        }
    }
    else
#endif
    {
        int pxw = 16;
        long wc;

        // adjust pio_xfer_width - don't use DWORD if wordCnt is odd.

#if HAVE_WIDE_XFERS
        pxw = pio_xfer_width;
        if ( ( pxw == 32 ) && ( wordCnt & 0x00000001L ) )
            pxw = 16;
#else
        if(pxw > 16) pxw = 16;
#endif
        // Data transfer using INS instruction.
        // Break the transfer into chunks of 32768 or fewer bytes.

        while ( wordCnt > 0 )
        {
            //bufSeg = (unsigned int) ( bufAddr >> 4 );
            //bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
            if ( wordCnt > 16384L )
                wc = 16384;
            else
                wc = wordCnt;
            if ( pxw == 8 )
            {
                // do REP INS
                pio_rep_inbyte( addrDataReg, bufAddr, wc * 2L );
            }
            else
#if HAVE_WIDE_XFERS
                if ( pxw == 32 )
                {
                    // do REP INSD
                    pio_rep_indword( addrDataReg, bufAddr, wc / 2L );
                }
                else
#endif
                {
                    // do REP INSW
                    pio_rep_inword( addrDataReg, bufAddr, wc );
                }
            bufAddr += wc * 2;
            wordCnt = wordCnt - wc;
        }
    }

    return;
}

//*************************************************************

// Note: pio_drq_block_out() is the primary way perform PIO
// Data Out transfers. It will handle 8-bit, 16-bit and 32-bit
// I/O based data transfers and 8-bit and 16-bit PCMCIA Memory
// mode transfers.

void pio_drq_block_out( ataio_t *ata, unsigned int addrDataReg,
                        //unsigned int bufSeg, unsigned int bufOff,
                        void *bufAddr,
                        long wordCnt )

{
    //unsigned long bufAddr;

    // NOTE: wordCnt is the size of a DRQ data block/packet
    // in words. The maximum value of wordCnt is normally:
    // a) For ATA, 16384 words or 32768 bytes (64 sectors,
    //    only with READ/WRITE MULTIPLE commands),
    // b) For ATAPI, 32768 words or 65536 bytes
    //    (actually 65535 bytes plus a pad byte).

    // normalize bufSeg:bufOff

    //bufAddr = bufSeg;
    //bufAddr = bufAddr << 4;
    //bufAddr = bufAddr + bufOff;

#if 0
    if ( pio_memory_seg )
    {
        long bCnt;
        int memDtOpt;
        unsigned int randVal;
        unsigned int dataRegAddr;
        unsigned int * uip1;
        unsigned int * uip2;
        unsigned char * ucp1;
        unsigned char * ucp2;

        // PCMCIA Memory mode data transfer.

        // set Data reg address per pio_memory_dt_opt
        dataRegAddr = 0x0000;
        memDtOpt = pio_memory_dt_opt;
        if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
        {
            randVal = * (unsigned int *) MK_FP( 0x40, 0x6c );
            memDtOpt = randVal % 3;
        }
        if ( memDtOpt == PIO_MEMORY_DT_OPT8 )
            dataRegAddr = 0x0008;
        if ( memDtOpt == PIO_MEMORY_DT_OPTB )
        {
            dataRegAddr = 0x0400;
            if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
                dataRegAddr = dataRegAddr | ( randVal & 0x03fe );
        }

        if ( pio_xfer_width == 8 )
        {
            // PCMCIA Memory mode 8-bit
            bCnt = wordCnt * 2L;
            ucp2 = (unsigned char *) MK_FP( pio_memory_seg, dataRegAddr );
            for ( ; bCnt > 0; bCnt -- )
            {
                bufSeg = (unsigned int) ( bufAddr >> 4 );
                bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
                ucp1 = (unsigned char *) MK_FP( bufSeg, bufOff );
                * ucp2 = * ucp1;
                bufAddr += 1;
                if ( memDtOpt == PIO_MEMORY_DT_OPTB )
                {
                    dataRegAddr += 1;
                    dataRegAddr = ( dataRegAddr & 0x03ff ) | 0x0400;
                    ucp2 = (unsigned char *) MK_FP( pio_memory_seg, dataRegAddr );
                }
            }
            trc_llt( addrDataReg, 0, TRC_LLT_OUTSB );
        }
        else
        {
            // PCMCIA Memory mode 16-bit
            uip2 = (unsigned int *) MK_FP( pio_memory_seg, dataRegAddr );
            for ( ; wordCnt > 0; wordCnt -- )
            {
                bufSeg = (unsigned int) ( bufAddr >> 4 );
                bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
                uip1 = (unsigned int *) MK_FP( bufSeg, bufOff );
                * uip2 = * uip1;
                bufAddr += 2;
                if ( memDtOpt == PIO_MEMORY_DT_OPTB )
                {
                    dataRegAddr = dataRegAddr + 2;
                    dataRegAddr = ( dataRegAddr & 0x03fe ) | 0x0400;
                    uip2 = (unsigned int *) MK_FP( pio_memory_seg, dataRegAddr );
                }
            }
            trc_llt( addrDataReg, 0, TRC_LLT_OUTSW );
        }
    }
    else
#endif
    {
        int pxw = 16;
        long wc;

        // adjust pio_xfer_width - don't use DWORD if wordCnt is odd.

#if HAVE_WIDE_XFERS
        pxw = pio_xfer_width;
        if ( ( pxw == 32 ) && ( wordCnt & 0x00000001L ) )
            pxw = 16;
#else
        if(pxw > 16) pxw = 16;
#endif
        // Data transfer using OUTS instruction.
        // Break the transfer into chunks of 32768 or fewer bytes.

        while ( wordCnt > 0 )
        {
            //bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
            //bufSeg = (unsigned int) ( bufAddr >> 4 );
            if ( wordCnt > 16384L )
                wc = 16384;
            else
                wc = wordCnt;
            if ( pxw == 8 )
            {
                // do REP OUTS
                pio_rep_outbyte( addrDataReg, bufAddr, wc * 2L );
            }
            else
#if HAVE_WIDE_XFERS
                if ( pxw == 32 )
                {
                    // do REP OUTSD
                    pio_rep_outdword( addrDataReg, bufAddr, wc / 2L );
                }
                else
#endif
                {
                    // do REP OUTSW
                    pio_rep_outword( addrDataReg, bufAddr, wc );
                }
            bufAddr += wc * 2;
            wordCnt = wordCnt - wc;
        }
    }

    return;
}

//*************************************************************
//
// These functions do REP INS/OUTS data transfers
// (PIO data transfers in I/O mode):
//
// pio_rep_inbyte()
// pio_rep_outbyte()
// pio_rep_inword()
// pio_rep_outword()
// pio_rep_indword()
// pio_rep_outdword()
//
// These functions can be called directly but usually they
// are called by the pio_drq_block_in() and pio_drq_block_out()
// functions to perform I/O mode transfers. See the
// pio_xfer_width variable!
//
//*************************************************************
#if 0
void pio_rep_inbyte( unsigned int addrDataReg,
                     void *addr,
                     long byteCnt )
{
    insb((int)pio_reg_addrs[ addrDataReg ], addr, byteCnt );
    trc_llt( addrDataReg, 0, TRC_LLT_INSB );
}

//*************************************************************

void pio_rep_outbyte( unsigned int addrDataReg,
                      void *addr,
                      long byteCnt )

{
    outsb((int)pio_reg_addrs[ addrDataReg ], addr, byteCnt);
    trc_llt( addrDataReg, 0, TRC_LLT_OUTSB );
}


//*************************************************************

void pio_rep_inword( unsigned int addrDataReg,
                     void *addr,
                     long wordCnt )

{
    insw((int)pio_reg_addrs[ addrDataReg ], addr, (int)wordCnt );
    trc_llt( addrDataReg, 0, TRC_LLT_INSW );
}


//*************************************************************

void pio_rep_outword( unsigned int addrDataReg,
                      void *addr,
                      long wordCnt )

{
    outsw((int)pio_reg_addrs[ addrDataReg ], addr, (int)wordCnt );
    trc_llt( addrDataReg, 0, TRC_LLT_OUTSW );
}

#endif
#if HAVE_WIDE_XFERS

//*************************************************************

void pio_rep_indword( unsigned int addrDataReg,
                      unsigned int bufSeg, unsigned int bufOff,
                      long dwordCnt )

{
    unsigned int dataRegAddr = pio_reg_addrs[ addrDataReg ];
    unsigned int dwCnt = (unsigned int) dwordCnt;

    // Warning: Avoid calling this function with
    // dwordCnt > 8192 (transfers 32768 bytes).
    // bufSeg and bufOff should be normalized such
    // that bufOff is a value between 0 and 15 (0xf).

    asm   .386

        asm   push  ax
        asm   push  cx
        asm   push  dx
        asm   push  di
        asm   push  es

        asm   mov   ax,bufSeg
    asm   mov   es,ax
    asm   mov   di,bufOff

    asm   mov   cx,dwCnt
    asm   mov   dx,dataRegAddr

    asm   cld

        asm   rep   insd

        asm   pop   es
        asm   pop   di
        asm   pop   dx
        asm   pop   cx
        asm   pop   ax

        trc_llt( addrDataReg, 0, TRC_LLT_INSD );
}

//*************************************************************

void pio_rep_outdword( unsigned int addrDataReg,
                       unsigned int bufSeg, unsigned int bufOff,
                       long dwordCnt )

{
    unsigned int dataRegAddr = pio_reg_addrs[ addrDataReg ];
    unsigned int dwCnt = (unsigned int) dwordCnt;

    // Warning: Avoid calling this function with
    // dwordCnt > 8192 (transfers 32768 bytes).
    // bufSeg and bufOff should be normalized such
    // that bufOff is a value between 0 and 15 (0xf).

    asm   .386

        asm   push  ax
        asm   push  cx
        asm   push  dx
        asm   push  si
        asm   push  ds

        asm   mov   ax,bufSeg
    asm   mov   ds,ax
    asm   mov   si,bufOff

    asm   mov   cx,dwCnt
    asm   mov   dx,dataRegAddr

    asm   cld

        asm   rep   outsd

        asm   pop   ds
        asm   pop   si
        asm   pop   dx
        asm   pop   cx
        asm   pop   ax

        trc_llt( addrDataReg, 0, TRC_LLT_OUTSD );
}
#endif

// end ataiopio.c
#endif
