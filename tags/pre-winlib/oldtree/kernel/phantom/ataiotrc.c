#ifdef ARCH_ia32


//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOTRC.C
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
// This C source contains the low level I/O trace functions.
//********************************************************************

#include <phantom_libc.h>

#include "ataio.h"

//**************************************************************

// trace dump buffer returned by trc_err_dump2()
// trc_cht_dump2() and trc_llt_dump2()

#define TDB_SIZE 200
static char trcDmpBuf[TDB_SIZE+1];

// buffer used to assemble print lines

static char prtBuf[64];

//**************************************************************

// function to return command type (protocol)

static const char * typeNames[] =
   {
     //123456789012 (max 12 chars)
      "NONE",              // TRC_TYPE_NONE
      "ATA DmaIn",         // TRC_TYPE_ADMAI
      "ATA DmaOut",        // TRC_TYPE_ADMAO
      "ATA ND",            // TRC_TYPE_AND
      "ATA PDI",           // TRC_TYPE_APDI
      "ATA PDO",           // TRC_TYPE_APDO
      "ATA SR",            // TRC_TYPE_ASR
      "ATAPI DmaIn",       // TRC_TYPE_PDMAI
      "ATAPI DmaOut",      // TRC_TYPE_PDMAO
      "ATAPI ND",          // TRC_TYPE_PND
      "ATAPI PDI",         // TRC_TYPE_PPDI
      "ATAPI PDO",         // TRC_TYPE_PPDO
      "????"
   } ;

const char * trc_get_type_name( unsigned char ct )
{
   return typeNames[ ct ];
}

//**************************************************************

// table used to initialize cmd code ndx table
// this is used for fast lookup of cmd names
// and can be used to implement other arrays
// of information per cmd code.

// number of commands in lookup tables
// #define TRC_NUM_CMDS 52    // see ATAIO.H

static unsigned char cmdCodeLst[TRC_NUM_CMDS] =
   {
      0x00,    // 00 - entry for unknown cmd codes
      0x03,    // 01
      0x08,    // 02
      0x10,    // 03
      0x20,    // 04
      0x24,    // 05
      0x25,    // 06
      0x29,    // 08
      0x30,    // 09
      0x34,    // 10
      0x35,    // 11
      0x38,    // 12
      0x39,    // 13
      0x3C,    // 14
      0x3D,    // 15
      0x40,    // 16
      0x42,    // 17
      0x50,    // 18
      0x70,    // 19
      0x87,    // 20
      0x90,    // 21
      0x91,    // 22
      0x94,    // 23
      0x95,    // 24
      0x96,    // 25
      0x97,    // 26
      0x98,    // 27
      0x99,    // 28
      0xA0,    // 29
      0xA1,    // 30
      0xb0,    // 31
      0xC0,    // 32
      0xC4,    // 33
      0xC5,    // 34
      0xC6,    // 35
      0xC8,    // 36
      0xCA,    // 37
      0xCD,    // 38
      0xCE,    // 39
      0xE0,    // 40
      0xE1,    // 41
      0xE2,    // 42
      0xE3,    // 43
      0xE4,    // 44
      0xE5,    // 45
      0xE6,    // 46
      0xE7,    // 47
      0xE8,    // 48
      0xEA,    // 49
      0xEC,    // 50
      0xEF     // 51
   } ;

// command name lookup table

static const char * cmdNames[TRC_NUM_CMDS] =
   {
      "? Cmd Name ?",                     // 0x00  00 - unknown cmd codes
      "CFA REQUEST EXT ERR CODE" ,        // 0x03  01
      "DEVICE RESET" ,                    // 0x08  02
      "RECALIBRATE" ,                     // 0x10  03
      "READ SECTORS" ,                    // 0x20  04
      "READ SECTORS EXT" ,                // 0x24  05
      "READ DMA EXT" ,                    // 0x25  06
      "READ MULTIPLE EXT" ,               // 0x29  08
      "WRITE SECTORS" ,                   // 0x30  09
      "WRITE SECTORS EXT" ,               // 0x34  10
      "WRITE DMA EXT" ,                   // 0x35  11
      "CFA WRITE SECTORS WO ERASE" ,      // 0x38  12
      "WRITE MULTIPLE EXT" ,              // 0x39  13
      "WRITE VERIFY" ,                    // 0x3C  14
      "WRITE DMA FUA EXT" ,               // 0x3D  15
      "READ VERIFY SECTORS" ,             // 0x40  16
      "READ VERIFY SECTORS EXT" ,         // 0x42  17
      "FORMAT TRACK" ,                    // 0x50  18
      "SEEK" ,                            // 0x70  19
      "CFA TRANSLATE SECTOR" ,            // 0x87  20
      "EXECUTE DEVICE DIAGNOSTIC" ,       // 0x90  21
      "INITIALIZE DEVICE PARAMETERS" ,    // 0x91  22
      "STANDBY IMMEDIATE" ,               // 0x94  23
      "IDLE IMMEDIATE" ,                  // 0x95  24
      "STANDBY" ,                         // 0x96  25
      "IDLE" ,                            // 0x97  26
      "CHECK POWER MODE" ,                // 0x98  27
      "SLEEP" ,                           // 0x99  28
      "PACKET" ,                          // 0xA0  29
      "IDENTIFY PACKET DEVICE" ,          // 0xA1  30
      "SMART",                            // 0xb0  31
      "CFA ERASE SECTORS" ,               // 0xC0  32
      "READ MULTIPLE" ,                   // 0xC4  33
      "WRITE MULTIPLE" ,                  // 0xC5  34
      "SET MULTIPLE MODE" ,               // 0xC6  35
      "READ DMA" ,                        // 0xC8  36
      "WRITE DMA" ,                       // 0xCA  37
      "CFA WRITE MULTIPLE WO ERASE" ,     // 0xCD  38
      "WRITE MULTIPLE FUA EXT" ,          // 0xCE  39
      "STANDBY IMMEDIATE" ,               // 0xE0  40
      "IDLE IMMEDIATE" ,                  // 0xE1  41
      "STANDBY" ,                         // 0xE2  42
      "IDLE" ,                            // 0xE3  43
      "READ BUFFER" ,                     // 0xE4  44
      "CHECK POWER MODE" ,                // 0xE5  45
      "SLEEP" ,                           // 0xE6  46
      "FLUSH CACHE" ,                     // 0xE7  47
      "WRITE BUFFER" ,                    // 0xE8  48
      "FLUSH CACHE EXT" ,                 // 0xEA  49
      "IDENTIFY DEVICE" ,                 // 0xEC  50
      "SET FEATURES" ,                    // 0xEF  51
   } ;

// cmd code to cmd ndx table -
// see function init_cmd_ndx_tbl() and macro TRC_CC2NDX()

char trc_CmdCodeNdx[256];

// function to initialize trc_CmdCodeNdx[]

static void init_cmd_ndx_tbl( void );

static void init_cmd_ndx_tbl( void )

{
   unsigned int ndx;

   for ( ndx = 0; ndx < sizeof( cmdCodeLst ); ndx ++ )
      trc_CmdCodeNdx[cmdCodeLst[ndx]] = ndx;
   // note: unknown cmd codes will have an ndx of 0.
}

// function to return the cmd name for a cmd code.

const char * trc_get_cmd_name( unsigned int cc )
{

   if ( trc_CmdCodeNdx[CMD_READ_SECTORS] == 0 )    // 1st call initialization
      init_cmd_ndx_tbl();
   if ( cc == CMD_SRST )                           // soft reset
      return "SOFT RESET";
   return cmdNames[(int)trc_CmdCodeNdx[(int)cc]];            // code->ndx->name
}

//**************************************************************

// ATA status names lookup table and lookup function

static struct
{
   unsigned char bitPos;
   const char * bitName;
} ataStatusNames[] =
   {
       { 0x80 , "BSY "  },
       { 0x40 , "DRDY " },
       { 0x20 , "DF "   },
       { 0x10 , "DSC "  },
       { 0x08 , "DRQ "  },
       { 0x04 , "CORR " },
       { 0x02 , "IDX "  },
       { 0x01 , "ERR "  }
   } ;

static char ataStatusNameBuf[48];

const char * trc_get_st_bit_name( unsigned char st )

{
   int ndx;

   if ( st & 0x80 )
      st = 0x80;
   *ataStatusNameBuf = 0;
   for ( ndx = 0; ndx < 8; ndx ++ )
   {
      if ( st & ataStatusNames[ndx].bitPos )
         strcat( ataStatusNameBuf, ataStatusNames[ndx].bitName );
   }
   return ataStatusNameBuf;
}

//**************************************************************

// ATA error names lookup table and lookup function

static struct
{
   unsigned char bitPos;
   const char * bitName;
} ataErrorNames[] =
   {
       { 0x80 , "BBK:ICRC " },
       { 0x40 , "UNC "      },
       { 0x20 , "MC "       },
       { 0x10 , "IDNF "     },
       { 0x08 , "MCR "      },
       { 0x04 , "ABRT "     },
       { 0x02 , "TK0NF "    },
       { 0x01 , "AMNF "     }
   } ;

static char ataErrorNameBuf[48];

const char * trc_get_er_bit_name( unsigned char er )

{
   int ndx;

   *ataErrorNameBuf = 0;
   for ( ndx = 0; ndx < 8; ndx ++ )
   {
      if ( er & ataErrorNames[ndx].bitPos )
         strcat( ataErrorNameBuf, ataErrorNames[ndx].bitName );
   }
   return ataErrorNameBuf;
}

//**************************************************************

// error name lookup table and lookup function

static struct
{
   int errCode;
   const char * errName;
} errNames[] =
   {
    {       1 ,  "Soft Reset timed out polling for device 0 to set BSY=0"  },
    {       2 ,  "Soft Reset timed out polling device 1"                   },
    {       3 ,  "Soft Reset timed out polling for device 1 to set BSY=0"  },

    {      11 ,  "Selected device is hung - reset required"                },
    {      12 ,  "Device selection timed out polling for BSY=0 DRQ=0"      },

    {      21 ,  "Non-Data command ended with bad status"                  },
    {      22 ,  "Non-Data command timed out waiting for an interrupt"     },
    {      23 ,  "Non-Data command timed out polling for BSY=0"            },
    {      24 ,  "Exec Dev Diag command timed out polling device 1"        },

    {      31 ,  "PIO Data In command terminated by error status"          },
    {      32 ,  "Device should be ready to transfer data but DRQ=0"       },
    {      33 ,  "PIO Data In command ended with bad status"               },
    {      34 ,  "PIO Data In command timed out waiting for an interrupt"  },
    {      35 ,  "PIO Data In command timed out polling for BSY=0"         },

    {      41 ,  "PIO Data Out command terminated by error status"         },
    {      42 ,  "Device should be ready to transfer data but DRQ=0"       },
    {      43 ,  "PIO Data Out command ended with bad status"              },
    {      44 ,  "PIO Data Out command timed out waiting for an interrupt" },
    {      45 ,  "PIO Data Out command timed out polling for BSY=0"        },
    {      46 ,  "Extra interrupt at start of a PIO Data Out command"      },
    {      47 ,  "PIO Data Out command timed out polling for BSY=0"        },

    {      51 ,  "Timeout waiting for BSY=0/DRQ=1 for cmd packet transfer" },
    {      52 ,  "Bad status at command packet transfer time"              },
    {      53 ,  "Timeout waiting for interrupt for data packet transfer"  },
    {      54 ,  "Timeout polling for BSY=0/DRQ=1 for a data packet"       },
    {      55 ,  "Bad status at data packet transfer time"                 },
    {      56 ,  "Timout waiting for final interrupt at end of command"    },
    {      57 ,  "Timeout polling for final BSY=0 at end of command"       },
    {      58 ,  "Bad status at end of command"                            },
    {      59 ,  "Byte count for data packet is zero"                      },

    {      61 ,  "Buffer overrun (host buffer too small)"                  },

    {      70 ,  "DMA channel and/or interrupt not setup"                  },
    {      71 ,  "End of command without complete data transfer"           },
    {      72 ,  "Timeout waiting for 1st transfer to complete"            },
    {      73 ,  "Timeout waiting for command to complete"                 },
    {      74 ,  "Bad status at end of command"                            },
    {      75 ,  "Timeout waiting for BSY=0/DRQ=1 for cmd packet transfer" },
    {      76 ,  "Bad status at command packet transfer time"              },
    {      78 ,  "End of command with BMIDE Error=1"                       },

    {      80 ,  "No tag available now"                                    },
    {      81 ,  "Timeout polling for SERV=1"                              },

    {      0  ,  "(no error)"                                              }
    // end of table
   } ;

const char * trc_get_err_name( int ec )

{
   int ndx = 0;

   while ( 1 )
   {
      if ( ec == errNames[ndx].errCode )
         return errNames[ndx].errName;
      if ( ! errNames[ndx].errCode )
         break;
      ndx ++ ;
   }
   return "? unknown error code ?";
}

//**************************************************************

static struct
{
   unsigned int pErrCode;
   const char * pErrName;
} pErrNames[] =
   {
      { FAILBIT0  , "slow setting BSY=1 or DRQ=1 after A0 cmd"   },
      { FAILBIT1  , "got interrupt before cmd packet xfer"       },
      { FAILBIT2  , "SC wrong at cmd packet xfer time"           },
      { FAILBIT3  , "byte count wrong at cmd packet xfer time"   },
      { FAILBIT4  , "SC (CD bit) wrong at data packet xfer time" },
      { FAILBIT5  , "SC (IO bit) wrong at data packet xfer time" },
      { FAILBIT6  , "byte count wrong at data packet xfer time"  },
      { FAILBIT7  , "byte count odd at data packet xfer time"    },
      { FAILBIT8  , "SC (CD and IO bits) wrong at end of cmd"    },
      { FAILBIT9  , "fail bit 9"                                 },
      { FAILBIT10 , "fail bit 10"                                },
      { FAILBIT11 , "fail bit 11"                                },
      { FAILBIT12 , "fail bit 12"                                },
      { FAILBIT13 , "fail bit 13"                                },
      { FAILBIT14 , "fail bit 14"                                },
      { FAILBIT15 , "extra interrupts detected"                  }
   } ;

//**************************************************************

// command or reset error display data

static int errDmpLine = 0;
static int errDmpLine2 = 0;

//**************************************************************

// start the display of a command or reset error display

void trc_err_dump1( void )

{

   errDmpLine = 1;
   errDmpLine2 = 0;
}

//**************************************************************

// return one line of a command or reset error display,
// returns NULL at end

const char * trc_err_dump2( void )
{

   if ( ata->reg_cmd_info.flg == TRC_FLAG_EMPTY )
      return NULL;
   if ( errDmpLine == 1 )
   {
      errDmpLine = 2;
      if ( ata->reg_cmd_info.flg == TRC_FLAG_SRST )
         snprintf( trcDmpBuf, TDB_SIZE, "ATA Reset: SR = %s (%s)",
                             trc_get_cmd_name( CMD_SRST ),
                             trc_get_type_name( ata->reg_cmd_info.ct ) );
      else
      if ( ata->reg_cmd_info.flg == TRC_FLAG_ATAPI )
         snprintf( trcDmpBuf, TDB_SIZE, "PACKET Command: %02X = %s (%s)",
                             ata->reg_cmd_info.cmd,
                             trc_get_cmd_name( ata->reg_cmd_info.cmd ),
                             trc_get_type_name( ata->reg_cmd_info.ct ) );
      else
         snprintf( trcDmpBuf, TDB_SIZE, "ATA Command: %02X = %s (%s)",
                             ata->reg_cmd_info.cmd,
                             trc_get_cmd_name( ata->reg_cmd_info.cmd ),
                             trc_get_type_name( ata->reg_cmd_info.ct ) );

      return trcDmpBuf;
   }
   if ( errDmpLine == 2 )
   {
      errDmpLine = 3;
      if ( ata->reg_cmd_info.flg == TRC_FLAG_ATA )
      {
         if ( ata->reg_cmd_info.lbaSize == LBA48 )
         {
            // LBA48 before and after
            snprintf( trcDmpBuf, TDB_SIZE, "LBA48 SC %ld %lXH, "
                                "before %lu.%lu %lX.%lXH, "
                                "after %lu.%lu %lX.%lXH",
                                 ata->reg_cmd_info.ns, ata->reg_cmd_info.ns,
                                 ata->reg_cmd_info.lbaHigh1, ata->reg_cmd_info.lbaLow1,
                                 ata->reg_cmd_info.lbaHigh1, ata->reg_cmd_info.lbaLow1,
                                 ata->reg_cmd_info.lbaHigh2, ata->reg_cmd_info.lbaLow2,
                                 ata->reg_cmd_info.lbaHigh2, ata->reg_cmd_info.lbaLow2 );
         }
         else
         if ( ata->reg_cmd_info.lbaSize == LBA28 )
         {
            // LBA28 before and after
            snprintf( trcDmpBuf, TDB_SIZE, "LBA28 SC %ld %lXH, "
                                "before %lu %lXH, "
                                "after %lu %lXH",
                                 ata->reg_cmd_info.ns, ata->reg_cmd_info.ns,
                                 ata->reg_cmd_info.lbaLow1, ata->reg_cmd_info.lbaLow1,
                                 ata->reg_cmd_info.lbaLow2, ata->reg_cmd_info.lbaLow2 );
         }
         else
         {
            // CHS before and after
            unsigned int cyl1, head1, sect1;
            unsigned int cyl2, head2, sect2;

            cyl1  = (unsigned int) ( ata->reg_cmd_info.ch1 << 8 ) | ata->reg_cmd_info.cl1;
            head1 = (unsigned int) ata->reg_cmd_info.dh1 & 0x0f;
            sect1 = (unsigned int) ata->reg_cmd_info.sn1;
            cyl2  = (unsigned int) ( ata->reg_cmd_info.ch2 << 8 ) | ata->reg_cmd_info.cl2;
            head2 = (unsigned int) ata->reg_cmd_info.dh2 & 0x0f;
            sect2 = (unsigned int) ata->reg_cmd_info.sn2;
            snprintf( trcDmpBuf, TDB_SIZE, "CHS SC %ld %lXH, "
                                "before %u.%u.%u %X.%X.%XH, "
                                "after %u.%u.%u %X.%X.%XH ",
                                 ata->reg_cmd_info.ns, ata->reg_cmd_info.ns,
                                 cyl1, head1, sect1, cyl1, head1, sect1,
                                 cyl2, head2, sect2, cyl2, head2, sect2 );
         }
         return trcDmpBuf;
      }
      if ( ata->reg_cmd_info.flg == TRC_FLAG_ATAPI )
      {
         if ( ata->reg_atapi_cp_size == 12 )
         {
            snprintf( trcDmpBuf, TDB_SIZE, "CDB %02X %02X %02X %02X  "
                                    "%02X %02X %02X %02X  "
                                    "%02X %02X %02X %02X ",
                     ata->reg_atapi_cp_data[0], ata->reg_atapi_cp_data[1],
                     ata->reg_atapi_cp_data[2], ata->reg_atapi_cp_data[3],
                     ata->reg_atapi_cp_data[4], ata->reg_atapi_cp_data[5],
                     ata->reg_atapi_cp_data[6], ata->reg_atapi_cp_data[7],
                     ata->reg_atapi_cp_data[8], ata->reg_atapi_cp_data[9],
                     ata->reg_atapi_cp_data[10], ata->reg_atapi_cp_data[11] );
         }
         else
         {
            snprintf( trcDmpBuf, TDB_SIZE, "CDB %02X %02X %02X %02X  "
                                    "%02X %02X %02X %02X  "
                                    "%02X %02X %02X %02X  "
                                    "%02X %02X %02X %02X ",
                     ata->reg_atapi_cp_data[0], ata->reg_atapi_cp_data[1],
                     ata->reg_atapi_cp_data[2], ata->reg_atapi_cp_data[3],
                     ata->reg_atapi_cp_data[4], ata->reg_atapi_cp_data[5],
                     ata->reg_atapi_cp_data[6], ata->reg_atapi_cp_data[7],
                     ata->reg_atapi_cp_data[8], ata->reg_atapi_cp_data[9],
                     ata->reg_atapi_cp_data[10], ata->reg_atapi_cp_data[11],
                     ata->reg_atapi_cp_data[12], ata->reg_atapi_cp_data[13],
                     ata->reg_atapi_cp_data[14], ata->reg_atapi_cp_data[15] );
         }
         return trcDmpBuf;
      }
   }
   if ( errDmpLine == 3 )
   {
      errDmpLine = 4;
      snprintf( trcDmpBuf, TDB_SIZE, "Driver ErrCode: %d %s ",
                          ata->reg_cmd_info.ec, trc_get_err_name( ata->reg_cmd_info.ec ) );
      return trcDmpBuf;
   }
   if ( errDmpLine == 4 )
   {
      errDmpLine = 5;
      if ( ata->reg_cmd_info.to )
      {
         snprintf( trcDmpBuf, TDB_SIZE, "                   "
                             "Driver timed out (see low level trace for details) !" );
         return trcDmpBuf;
      }
   }
   if ( errDmpLine == 5 )
   {
      errDmpLine = 6;
      snprintf( trcDmpBuf, TDB_SIZE, "Bytes transferred: %ld (%lXH); DRQ blocks: %ld (%lXH) ",
                        ata->reg_cmd_info.totalBytesXfer, ata->reg_cmd_info.totalBytesXfer,
                        ata->reg_cmd_info.drqPackets, ata->reg_cmd_info.drqPackets );
      return trcDmpBuf;
   }
   if ( errDmpLine == 6 )
   {
      errDmpLine = 7;
      snprintf( trcDmpBuf, TDB_SIZE, "Device Status: %02X = %s ", ata->reg_cmd_info.st2,
                        trc_get_st_bit_name( ata->reg_cmd_info.st2 ) );
      return trcDmpBuf;
   }
   if ( errDmpLine == 7 )
   {
      errDmpLine = 8;
      snprintf( trcDmpBuf, TDB_SIZE, "Device  Error: %02X = %s ", ata->reg_cmd_info.er2,
                         trc_get_er_bit_name( ata->reg_cmd_info.er2 ) );
      return trcDmpBuf;
   }
   if ( errDmpLine == 8 )
   {
      errDmpLine = 9;
      snprintf( trcDmpBuf, TDB_SIZE, "ATA Intf Regs: FR  ER  SC  SN  CL  CH  DH  CM  ST  AS  DC " );
      return trcDmpBuf;
   }
   if ( errDmpLine == 9 )
   {
      errDmpLine = 10;
      if ( ata->reg_cmd_info.flg == TRC_FLAG_SRST )
         snprintf( trcDmpBuf, TDB_SIZE, "   Cmd Params: "
                  // fr  er  sc  sn  cl  ch  dh  cm  st  as  dc
                    "--  --  --  --  --  --  --  --  --  --  04 " );
      else
         snprintf( trcDmpBuf, TDB_SIZE, "   Cmd Params: "
                  //  fr   er   sc    sn    cl    ch    dh    cm   st  as   dc
                    "%02X  --  %02X  %02X  %02X  %02X  %02X  %02X  --  --  %02X ",
                     ata->reg_cmd_info.fr1 & 0x00ff,
                     ata->reg_cmd_info.sc1 & 0x00ff,
                     ata->reg_cmd_info.sn1,
                     ata->reg_cmd_info.cl1, ata->reg_cmd_info.ch1, ata->reg_cmd_info.dh1,
                     ata->reg_cmd_info.cmd, ata->reg_cmd_info.dc1 );
      return trcDmpBuf;
   }
   if ( errDmpLine == 10 )
   {
      errDmpLine = 11;
      snprintf( trcDmpBuf, TDB_SIZE, "    After Cmd: "
                  // fr   er    sc    sn    cl    ch    dh   cm   st    as   dc
                    "--  %02X  %02X  %02X  %02X  %02X  %02X  --  %02X  %02X  -- ",
                     ata->reg_cmd_info.er2, ata->reg_cmd_info.sc2 & 0x00ff,
                     ata->reg_cmd_info.sn2, ata->reg_cmd_info.cl2, ata->reg_cmd_info.ch2,
                     ata->reg_cmd_info.dh2, ata->reg_cmd_info.st2, ata->reg_cmd_info.as2 );
      return trcDmpBuf;
   }
   if ( ( errDmpLine == 11 ) &&  ata->reg_cmd_info.failbits )
   {
      errDmpLine = 12;
      errDmpLine2 = 0;
      snprintf( trcDmpBuf, TDB_SIZE, "  ATA/ATAPI protocol errors bits (%04XH):",
                          ata->reg_cmd_info.failbits );
      return trcDmpBuf;
   }
   if ( errDmpLine == 12 )
   {
      while ( ( errDmpLine2 < 16 )
              &&
              ( ! ( ata->reg_cmd_info.failbits & pErrNames[errDmpLine2].pErrCode ) )
            )
         errDmpLine2 ++ ;
      if ( errDmpLine2 < 16 )
      {
         snprintf( trcDmpBuf, TDB_SIZE, "      - %s", pErrNames[errDmpLine2].pErrName );
         errDmpLine2 ++ ;
         return trcDmpBuf;
      }
   }
   return NULL;
}

//**********************************************************

// command types to trace, see TRC_TYPE_xxx in ataio.h and
// see trc_cht_types() below.

static unsigned int chtTypes = 0xffff; // default is trace all cmd types

// command history trace buffer

#define MAX_CHT 100

static struct
{
   // entry type, entry flag, command code, etc
   unsigned char flg;         // see TRC_FLAG_xxx in ataio.h
   unsigned char ct;          // see TRC_TYPE_xxx in ataio.h
   unsigned char cmd;         // command code
   long ns;                   // number of sectors (sector count)
   int mc;                    // multiple count
   unsigned int  fr1;         // feature (8 or 16 bits)
   unsigned char dh1;         // device head
   // starting CHS/LBA
   unsigned char lbaSize;     // CHS/LBA addr mode
   unsigned int  cyl;         // CHS cyl or ATAPI BCL
   unsigned char head;        // CHS head
   unsigned char sect;        // CHS sect
   unsigned long lbaLow1;     // LBA lower 32-bits
   // ending status and driver error codes
   unsigned char st2;         // status reg
   unsigned char er2;         // error reg
   unsigned char ec;          // detailed error code
   unsigned char to;          // not zero if time out error
   // ATAPI CDB size and CDB data
   unsigned char cdbSize;     // CDB size (12 or 16)
   unsigned char cdbBuf[16];  // CDB data
}  chtBuf[MAX_CHT];

static int chtCur = 0;
static int chtDmpLine = 0;
static int chtDmpNdx = 0;

static char * chtTypeName[] =
   { "?????",
     "DR   ", "DW   ",
     "ND   ", "PDI  ", "PDO  ",
     "RESET",
     "DPR  ", "DPW  ",
     "PN   ", "PR   ", "PW   " } ;

//**************************************************************

// set the commands types that are traced,
// see TRC_TYPE_xxx in ataio.h and chtTypes above.

void trc_cht_types( int type )

{

   if ( type < 1 )
      chtTypes = 0x0000;   // trace nothing
   else
      if ( type > 15 )
         chtTypes = 0xffff;   // trace all
      else
         chtTypes |= 0x0001 << type;  // selective
}

//**************************************************************

#if ATAIO_TRACE
// place an command or reset entry into
// the command history trace buffer

void trc_cht( void )

{
   int ndx;

   if ( ! ( ( 0x0001 << ata->reg_cmd_info.ct ) & chtTypes ) )
      return;
   // entry type, entry flag, command code, etc
   chtBuf[chtCur].flg = ata->reg_cmd_info.flg;
   chtBuf[chtCur].ct  = ata->reg_cmd_info.ct ;
   chtBuf[chtCur].cmd = ata->reg_cmd_info.cmd;
   chtBuf[chtCur].ns = ata->reg_cmd_info.ns;
   chtBuf[chtCur].mc = ata->reg_cmd_info.mc;
   chtBuf[chtCur].fr1 = ata->reg_cmd_info.fr1;
   chtBuf[chtCur].dh1 = ata->reg_cmd_info.dh1;
   // starting CHS/LBA
   chtBuf[chtCur].lbaSize = ata->reg_cmd_info.lbaSize;
   chtBuf[chtCur].cyl  = ( ata->reg_cmd_info.ch1 << 8 ) | ata->reg_cmd_info.cl1;
   chtBuf[chtCur].head = ata->reg_cmd_info.dh1 & 0x0f;
   chtBuf[chtCur].sect = ata->reg_cmd_info.sn1;
   chtBuf[chtCur].lbaLow1 = ata->reg_cmd_info.lbaLow1;
   // ending status and driver error codes
   chtBuf[chtCur].st2 = ata->reg_cmd_info.st2;
   chtBuf[chtCur].er2 = ata->reg_cmd_info.er2;
   chtBuf[chtCur].ec  = ata->reg_cmd_info.ec ;
   chtBuf[chtCur].to  = ata->reg_cmd_info.to ;
   // ATAPI CDB size and CDB data
   chtBuf[chtCur].cdbSize = ata->reg_atapi_cp_size;
   for ( ndx = 0; ndx < ata->reg_atapi_cp_size; ndx ++ )
      chtBuf[chtCur].cdbBuf[ndx] = ata->reg_atapi_cp_data[ndx];
   // move to next entry
   chtCur ++ ;
   if ( chtCur >= MAX_CHT )
      chtCur = 0;
}

#endif

//**************************************************************

// clear the command history trace buffer

void trc_cht_dump0( void )

{

   for ( chtCur = 0; chtCur < MAX_CHT; chtCur ++ )
      chtBuf[chtCur].flg = TRC_FLAG_EMPTY;
   chtCur = 0;
}

//**************************************************************

// start a dump of the command history trace buffer

void trc_cht_dump1( void )

{

   chtDmpLine = 1;
   chtDmpNdx = chtCur + 1;
   if ( chtDmpNdx >= MAX_CHT )
      chtDmpNdx = 0;
}

//**************************************************************

// return one line of the command history trace buffer,
// returns NULL at end.
//
// lines are formated in the style of ATADEMO commands.
// there are three ATADEMO commands per line (DEV, LBAx,
// and the I/O command). The three values at the end of the
// line (following the //) are the driver result error code,
// driver timeout flag, status register and error register.

static char esStr[24];
static char atStr[24];
static char saStr[24];
static char mcStr[24];

const char * trc_cht_dump2( void )

{

   if ( chtDmpLine == 1 )     // 1st line is 1st heading line
   {
      strcpy( trcDmpBuf,
        //0        1         2         3         4         5         6         7
        //123456789012345678901234567890123456789012345678901234567890123456789012
        //DEV n, LBAnn, ttttt xxH xxxxH nnnnn nnnnnnnnn    nnn ; nn nn xxH xxH
         "Dev n, LBAnn, Type- Cmd FR--- SC--- LBA--------- MC- ; EC TO ST- ER-" );
      chtDmpLine = 2;
      return trcDmpBuf;
   }
   if ( chtDmpLine == 2 )     // 2nd line is 2nd heading line
   {
      strcpy( trcDmpBuf,
        //0        1         2         3         4         5         6         7
        //123456789012345678901234567890123456789012345678901234567890123456789012
        //DEV n, CHS,   ttttt xxH xxxxH nnnnn ccccc hh sss nnn ; nn nn xxH xxH
         "Dev n, CHS,   Type- Cmd FR--- SC--- Cyl-- Hd Sec MC- ; EC TO ST- ER-" );
      chtDmpLine = 3;
      return trcDmpBuf;
   }
   // search for oldest entry
   while ( 1 )
   {
      if ( chtDmpNdx == chtCur )
         return NULL;
      if ( chtBuf[chtDmpNdx].flg != TRC_FLAG_EMPTY )
         break;
      chtDmpNdx ++ ;
      if ( chtDmpNdx >= MAX_CHT )
         chtDmpNdx = 0;
   }
   // return one trace table entry...
   // first format the result data
   snprintf( esStr, sizeof(esStr)-1, " ; %2d %2d %02XH %02XH",
                   chtBuf[chtDmpNdx].ec,
                   chtBuf[chtDmpNdx].to,
                   chtBuf[chtDmpNdx].st2,
                   chtBuf[chtDmpNdx].er2 );
   if ( chtBuf[chtDmpNdx].flg == TRC_FLAG_SRST )
   {
      // TRC_TYPE_ASR -> DEV n, LBAnn, RESET ; nn nn xxH xxH

      //0        1         2         3         4         5         6         7
      //123456789012345678901234567890123456789012345678901234567890123456789012
      //DEV n, LBAnn, RESET                                  ; nn nn xxH xxH

      snprintf( trcDmpBuf, TDB_SIZE,
         "DEV %d, LBA28, RESET                                 ",
            ( chtBuf[chtDmpNdx].dh1 & 0x10 ) ? 1 : 0 );
      strcat( trcDmpBuf, esStr );
   }
   else
   if ( chtBuf[chtDmpNdx].flg == TRC_FLAG_ATA )
   {
      // TRC_TYPE_ADMAI -> DEV n, LBAnn, DR      xxH sc lba    ; nn nn xxH xxH
      // TRC_TYPE_ADMAO -> DEV n, LBAnn, DW      xxH sc lba    ; nn nn xxH xxH
      // TRC_TYPE_AND   -> DEV n, LBAnn, ND  xxH xxH sc lba    ; nn nn xxH xxH
      // TRC_TYPE_APDI  -> DEV n, LBAnn, PDI xxH xxH sc lba mc ; nn nn xxH xxH
      // TRC_TYPE_APDO  -> DEV n, LBAnn, PDO xxH xxH sc lba mc ; nn nn xxH xxH

      // format the LBAxx/CHS and starting LBA/CHS
      if ( chtBuf[chtDmpNdx].lbaSize == 48 )
      {
         strcpy( atStr, "LBA48," );
         snprintf( saStr, sizeof(saStr)-1, " %12ld", chtBuf[chtDmpNdx].lbaLow1 );
      }
      else
      if ( chtBuf[chtDmpNdx].lbaSize == 28 )
      {
         strcpy( atStr, "LBA28," );
         snprintf( saStr, sizeof(saStr)-1, " %12ld", chtBuf[chtDmpNdx].lbaLow1 );
      }
      else
      {
         strcpy( atStr, "CHS,  " );
         snprintf( saStr, sizeof(saStr)-1, " %5u %2u %3u",
                  chtBuf[chtDmpNdx].cyl,
                  chtBuf[chtDmpNdx].head,
                  chtBuf[chtDmpNdx].sect );
      }

      // format the MC
      if (    ( chtBuf[chtDmpNdx].cmd == CMD_READ_MULTIPLE )
           || ( chtBuf[chtDmpNdx].cmd == CMD_READ_MULTIPLE_EXT )
           || ( chtBuf[chtDmpNdx].cmd == CMD_WRITE_MULTIPLE )
           || ( chtBuf[chtDmpNdx].cmd == CMD_WRITE_MULTIPLE_EXT )
           || ( chtBuf[chtDmpNdx].cmd == CMD_WRITE_MULTIPLE_FUA_EXT )
           || ( chtBuf[chtDmpNdx].cmd == CMD_CFA_WRITE_MULTIPLE_WO_ERASE )
         )
         snprintf( mcStr, sizeof(mcStr)-1, " %3d", chtBuf[chtDmpNdx].mc );
      else
         strcpy( mcStr, "    " );

        //0        1         2         3         4         5         6         7
        //123456789012345678901234567890123456789012345678901234567890123456789012
        //DEV n, LBAnn, ttttt xxH xxxxH nnnnn nnnnnnnnn    nnn ; nn nn xxH xxH
        //DEV n, CHS,   ttttt xxH xxxxH nnnnn ccccc hh sss nnn ; nn nn xxH xxH

      snprintf( trcDmpBuf, TDB_SIZE,
         "DEV %d, %s %s %02XH %4XH %5u",
            ( chtBuf[chtDmpNdx].dh1 & 0x10 ) ? 1 : 0,
            atStr,
            chtTypeName[chtBuf[chtDmpNdx].ct],
            chtBuf[chtDmpNdx].cmd,
            chtBuf[chtDmpNdx].fr1,
            chtBuf[chtDmpNdx].ns );
      strcat( trcDmpBuf, saStr );
      strcat( trcDmpBuf, mcStr );
      strcat( trcDmpBuf, esStr );
   }
   else
   if ( chtBuf[chtDmpNdx].flg == TRC_FLAG_ATAPI )
   {
      // TRC_TYPE_PDMAI -> not used by ATACT
      // TRC_TYPE_PDMAO -> not used by ATACT
      // TRC_TYPE_PND   -> DEV n, PN bcl cdb0 cdb1 ... cdbn ; nn nn xxH xxH
      // TRC_TYPE_PPDI  -> DEV n, PR bcl cdb0 cdb1 ... cdbn ; nn nn xxH xxH
      // TRC_TYPE_PPDO  -> DEV n, PW bcl cdb0 cdb1 ... cdbn ; nn nn xxH xxH

      if ( chtBuf[chtDmpNdx].cdbSize == 12 )
      {
         //DEV n, t n x x x x x x x x x x x x ; nn nn xxH xxH
         snprintf( trcDmpBuf, TDB_SIZE,
            "DEV %d, %3.3s %u %02XH %d %d %d %d %d %d %d %d %d %d %d",
               ( chtBuf[chtDmpNdx].dh1 & 0x10 ) ? 1 : 0,
               chtTypeName[chtBuf[chtDmpNdx].ct],
               chtBuf[chtDmpNdx].cyl,
               chtBuf[chtDmpNdx].cdbBuf[0], chtBuf[chtDmpNdx].cdbBuf[1],
               chtBuf[chtDmpNdx].cdbBuf[2], chtBuf[chtDmpNdx].cdbBuf[3],
               chtBuf[chtDmpNdx].cdbBuf[4], chtBuf[chtDmpNdx].cdbBuf[5],
               chtBuf[chtDmpNdx].cdbBuf[6], chtBuf[chtDmpNdx].cdbBuf[7],
               chtBuf[chtDmpNdx].cdbBuf[8], chtBuf[chtDmpNdx].cdbBuf[9],
               chtBuf[chtDmpNdx].cdbBuf[10], chtBuf[chtDmpNdx].cdbBuf[11] );
         strcat( trcDmpBuf, esStr );
      }
      else
      {
         //DEV n, tt n x x x x x x x x x x x x x x x x ; nn nn xxH xxH
         snprintf( trcDmpBuf, TDB_SIZE,
            "DEV %d, %3.3s %u %02XH %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
               ( chtBuf[chtDmpNdx].dh1 & 0x10 ) ? 1 : 0,
               chtTypeName[chtBuf[chtDmpNdx].ct],
               chtBuf[chtDmpNdx].cyl,
               chtBuf[chtDmpNdx].cdbBuf[0], chtBuf[chtDmpNdx].cdbBuf[1],
               chtBuf[chtDmpNdx].cdbBuf[2], chtBuf[chtDmpNdx].cdbBuf[3],
               chtBuf[chtDmpNdx].cdbBuf[4], chtBuf[chtDmpNdx].cdbBuf[5],
               chtBuf[chtDmpNdx].cdbBuf[6], chtBuf[chtDmpNdx].cdbBuf[7],
               chtBuf[chtDmpNdx].cdbBuf[8], chtBuf[chtDmpNdx].cdbBuf[9],
               chtBuf[chtDmpNdx].cdbBuf[10], chtBuf[chtDmpNdx].cdbBuf[11],
               chtBuf[chtDmpNdx].cdbBuf[12], chtBuf[chtDmpNdx].cdbBuf[13],
               chtBuf[chtDmpNdx].cdbBuf[14], chtBuf[chtDmpNdx].cdbBuf[15] );
         strcat( trcDmpBuf, esStr );
      }
   }
   chtDmpNdx ++ ;
   if ( chtDmpNdx >= MAX_CHT )
      chtDmpNdx = 0;
   return trcDmpBuf;
}

//**********************************************************

// Low-level trace buffer

#define MAX_LLT 500

static struct
{
   unsigned char addr;
   unsigned char data;
   unsigned char type;
   unsigned char rep;
} lltBuf[MAX_LLT];

static int lltCur = 0;
static int lltDmpLine = 0;
static int lltDmpNdx = 0;

static struct
{
   unsigned char typeId;      // trace entry type
   const char * typeNm;    // trace entry name
} type_nm[]
   =
   {
                 //0        1         2         3         4
                 //12345678901234567890123456789012345678901
                 //<rep> <opr> <--register---> <data - note>
      { TRC_LLT_INB     , "INB   " },
      { TRC_LLT_OUTB    , "OUTB  " },
      { TRC_LLT_INW     , "INW   " },
      { TRC_LLT_OUTW    , "OUTW  " },
      { TRC_LLT_INSB    , "INSB  " },
      { TRC_LLT_OUTSB   , "OUTSB " },
      { TRC_LLT_INSW    , "INSW  " },
      { TRC_LLT_OUTSW   , "OUTSW " },
      { TRC_LLT_INSD    , "INSD  " },
      { TRC_LLT_OUTSD   , "OUTSD " },

      { TRC_LLT_S_CFG   , "===== Start Dev Cnfg  "},
      { TRC_LLT_S_RST   , "===== Start Reset     "},
      { TRC_LLT_S_ND    , "===== Start ND cmd    "},
      { TRC_LLT_S_PDI   , "===== Start PDI cmd   "},
      { TRC_LLT_S_PDO   , "===== Start PDO cmd   "},
      { TRC_LLT_S_PI    , "===== Start PI cmd    "},
      { TRC_LLT_S_RWD   , "===== Start R/W DMA   "},
      { TRC_LLT_S_PID   , "===== Start PI DMA    "},
      { TRC_LLT_WINT    , "..... Wait for INTRQ  "},
      { TRC_LLT_INTRQ   , "..... I N T R Q       "},
      { TRC_LLT_PNBSY   , "..... Poll for BSY=0  "},
      { TRC_LLT_PRDY    , "..... Poll for DRDY=1 "},
      { TRC_LLT_TOUT    , "..... T I M E O U T   "},
      { TRC_LLT_ERROR   , "..... E R R O R       "},
      { TRC_LLT_DELAY1  , "..... DELAY ~80ms     "},
      { TRC_LLT_DELAY2  , "..... DELAY ~0-55ms   "},
      { TRC_LLT_E_CFG   , "===== End Dev Cnfg    "},
      { TRC_LLT_E_RST   , "===== End Reset       "},
      { TRC_LLT_E_ND    , "===== End ND cmd      "},
      { TRC_LLT_E_PDI   , "===== End PDI cmd     "},
      { TRC_LLT_E_PDO   , "===== End PDO cmd     "},
      { TRC_LLT_E_PI    , "===== End PI cmd      "},
      { TRC_LLT_E_RWD   , "===== End R/W DMA     "},
      { TRC_LLT_E_PID   , "===== End PI DMA      "},

      { TRC_LLT_DMA1    , "..... Enable DMA Ch   "},
      { TRC_LLT_DMA2    , "..... Poll DMA TC bit "},
      { TRC_LLT_DMA3    , "..... Disable DMA Ch  "},

      { TRC_LLT_DEBUG   , "..... DEBUG           "},
      { TRC_LLT_P_CMD   , "..... ATAPI Cmd Code  "},
      { TRC_LLT_R_BM_CR , "..... Rd BM Cmd Reg   "},
      { TRC_LLT_R_BM_SR , "..... Rd BM Stat Reg  "},
      { TRC_LLT_W_BM_CR , "..... Wr BM Cmd Reg   "},
      { TRC_LLT_W_BM_SR , "..... Wr BM Stat Reg  "},
      { 0               , "????? " }
   } ;

static const char * reg_nm[]  // register names for trace
   =
   {
      //0        1         2         3         4
      //12345678901234567890123456789012345678901
      //<rep> <opr> <--register---> <data - note>
                   "Data            " , // 0 data reg
                   "Error/Feature   " , // 1 error & feature
                   "SectorCount     " , // 2 sector count
                   "SectorNumber    " , // 3 sector number
                   "CylinderLow     " , // 4 cylinder low
                   "CylinderHigh    " , // 5 cylinder high
                   "DeviceHead      " , // 6 device head
                   "Status/Cmd      " , // 7 primary status & command
                   "AltStat/DevCtrl " , // 8 alternate status & device control
                   "DevAddr         " , // 9 device address
   } ;

//*********************************************************

#if ATAIO_TRACE

// place an entry into the low level trace buffer

void trc_llt( unsigned char addr,
              unsigned char data,
              unsigned char type )

{

   // if same as previous, incr rep count and return
   if ( ( addr == lltBuf[lltCur].addr )
        &&
        ( data == lltBuf[lltCur].data )
        &&
        ( type == lltBuf[lltCur].type )
      )
   {
      lltBuf[lltCur].rep = ( lltBuf[lltCur].rep >= 255L )
                           ? 255 : lltBuf[lltCur].rep + 1;
      return;
   }
   // incr buffer index
   lltCur ++ ;
   if ( lltCur >= MAX_LLT )
      lltCur = 0;
   // start new entry
   lltBuf[lltCur].addr = addr;
   lltBuf[lltCur].data = data;
   lltBuf[lltCur].type = type;
   lltBuf[lltCur].rep = 1;
}

#endif

//**************************************************************

// clear the low level trace buffer

void trc_llt_dump0( void )

{

   for ( lltCur = 0; lltCur < MAX_LLT; lltCur ++ )
   {
      lltBuf[lltCur].type = TRC_LLT_NONE;
   }
   lltCur = 0;
}

//**************************************************************

// start a dump of the low level trace buffer

void trc_llt_dump1( void )

{

   // complete most recent trace entry
   trc_llt( 0, 0, TRC_LLT_NONE );
   // setup to return trace data
   lltDmpLine = 0;
   lltDmpNdx = lltCur + 1;
   if ( lltDmpNdx >= MAX_LLT )
      lltDmpNdx = 0;
}

//**************************************************************

// return one line of the low level trace,
// returns NULL at end.

const char * trc_llt_dump2( void )

{
   int ndx;

   // increment output line number
   lltDmpLine ++ ;

   // return 1st line - the heading
   if ( lltDmpLine == 1 )     // 1st line is heading
   {
                        //0        1         2         3         4
                        //12345678901234567890123456789012345678901
      strcpy( trcDmpBuf, "<rep> <opr> <--register---> <data - note>" );
      return trcDmpBuf;
   }

   // find next non-zero trace entry
   while ( 1 )
   {
      if ( lltDmpNdx == lltCur )
         return NULL;
      if ( lltBuf[lltDmpNdx].type != 0 )
         break;
      lltDmpNdx ++ ;
      if ( lltDmpNdx >= MAX_LLT )
         lltDmpNdx = 0;
   }

   // put repeat count into buffer
   if ( lltBuf[lltDmpNdx].rep == 255 )
      strcpy( prtBuf, ">=255 " );
   else
      snprintf( prtBuf, sizeof(prtBuf)-1, "%5u ", lltBuf[lltDmpNdx].rep );
   strcpy( trcDmpBuf, prtBuf );

   // lookup trace entry type, put into buffer
   ndx = 0;
   while ( type_nm[ndx].typeId )
   {
      if ( lltBuf[lltDmpNdx].type == type_nm[ndx].typeId )
         break;
      ndx ++ ;
   }
   strcat( trcDmpBuf, type_nm[ ndx ].typeNm );

   // register write/read or something else
   if ( lltBuf[lltDmpNdx].type < TRC_LLT_S_CFG )
   {
      // reg name and value
      strcat( trcDmpBuf, reg_nm[ lltBuf[lltDmpNdx].addr ] );
      if ( lltBuf[lltDmpNdx].addr == CB_DATA )
         strcpy( prtBuf, "-- " );
      else
         snprintf( prtBuf, sizeof(prtBuf)-1, "%02X ", lltBuf[lltDmpNdx].data );
      strcat( trcDmpBuf, prtBuf );
      // write to Dev Ctrl
      if (    ( lltBuf[lltDmpNdx].addr == CB_DC )
           && ( lltBuf[lltDmpNdx].type == TRC_LLT_OUTB )
         )
      {
         if ( lltBuf[lltDmpNdx].data & CB_DC_SRST )
         {
            strcat( trcDmpBuf, "START: " );
            strcat( trcDmpBuf, trc_get_cmd_name( CMD_SRST ) );
            strcat( trcDmpBuf, ", " );
         }
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & CB_DC_HOB )
                       ? "HOB=1" : "HOB=0" );
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & CB_DC_SRST )
                       ? " SRST=1" : " SRST=0" );
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & CB_DC_NIEN )
                       ? " nIEN=1" : " nIEN=0" );
      }
      // write to Command reg
      if (    ( lltBuf[lltDmpNdx].addr == CB_CMD )
           && ( lltBuf[lltDmpNdx].type == TRC_LLT_OUTB )
         )
      {
         strcat( trcDmpBuf, "START: " );
         strcat( trcDmpBuf, trc_get_cmd_name( lltBuf[lltDmpNdx].data ) );
      }
      // write to Device/Head
      if (    ( lltBuf[lltDmpNdx].addr == CB_DH )
           && ( lltBuf[lltDmpNdx].type == TRC_LLT_OUTB )
         )
      {
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & 0x10 )
                       ? "DEV=1" : "DEV=0" );
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & 0x40 )
                       ? " LBA=1" : " LBA=0" );
      }
      // read of Status or Alt Status
      if (    (    ( lltBuf[lltDmpNdx].addr == CB_STAT )
                || ( lltBuf[lltDmpNdx].addr == CB_ASTAT )
              )
           && ( lltBuf[lltDmpNdx].type == TRC_LLT_INB )
         )
         strcat( trcDmpBuf, trc_get_st_bit_name( lltBuf[lltDmpNdx].data ) );
      // read of Error
      if (    ( lltBuf[lltDmpNdx].addr == CB_ERR )
           && ( lltBuf[lltDmpNdx].type == TRC_LLT_INB )
         )
         strcat( trcDmpBuf, trc_get_er_bit_name( lltBuf[lltDmpNdx].data ) );
   }
   else
   // start/end/debug/etc entry or something else
   if ( lltBuf[lltDmpNdx].type >= TRC_LLT_DEBUG )
   {
      snprintf( prtBuf, sizeof(prtBuf)-1, "%02X ", lltBuf[lltDmpNdx].data );
      strcat( trcDmpBuf, prtBuf );
      // write/read of BMIDE Command reg
      if (    ( lltBuf[lltDmpNdx].type == TRC_LLT_R_BM_CR )
           || ( lltBuf[lltDmpNdx].type == TRC_LLT_W_BM_CR )
         )
      {
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & 0x08 )
                       ? " Dir=1(MemWr)" : " Dir=0(MemRd)" );
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & 0x01 )
                       ? " Go=1(Start)" : " Go=0(Stop)" );
      }
      // write/read of BMIDE Status reg
      if (    ( lltBuf[lltDmpNdx].type == TRC_LLT_R_BM_SR )
           || ( lltBuf[lltDmpNdx].type == TRC_LLT_W_BM_SR )
         )
      {
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & 0x40 )
                       ? " D1=1" : " D1=0" );
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & 0x20 )
                       ? " D0=1" : " D0=0" );
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & 0x04 )
                       ? " Int=1" : " Int=0" );
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & 0x02 )
                       ? " Err=1" : " Err=0" );
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & 0x01 )
                       ? " Act=1" : " Act=0" );
      }
   }
   else
   // driver error
   if ( lltBuf[lltDmpNdx].type == TRC_LLT_ERROR )
   {
      snprintf( prtBuf, sizeof(prtBuf)-1, "%02X ", lltBuf[lltDmpNdx].data );
      strcat( trcDmpBuf, prtBuf );
      strcat( trcDmpBuf, trc_get_err_name( lltBuf[lltDmpNdx].data ) );
   }

   // increment to next trace entry
   lltDmpNdx ++ ;
   if ( lltDmpNdx >= MAX_LLT )
      lltDmpNdx = 0;

   // return trace entry string
   return trcDmpBuf;
}

// end ataiotrc.c
#endif // ARCH_ia32
