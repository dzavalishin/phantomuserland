#include <hal.h>
#include <kernel/vm.h>

// !!!! TEMP !!!!



//#error convert 16 bit data types to 32 bit ones


//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIO.H (driver's public data)
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
// See the make files EXAMPLE1.MAK and EXAMPLE2.MAK.
//
// This C source file is the header file for the for this driver
// and is used in the ATAIOxxx.C files and must also be used
// by any program using this driver code.
//********************************************************************

#define ATA_DRIVER_VERSION "16N"

//**************************************************************
//
// Global defines -- ATA register and register bits.
// command block & control block regs
//
//**************************************************************

// These are the offsets into pio_reg_addrs[]

#define CB_DATA  0   // data reg         in/out pio_base_addr1+0
#define CB_ERR   1   // error            in     pio_base_addr1+1
#define CB_FR    1   // feature reg         out pio_base_addr1+1
#define CB_SC    2   // sector count     in/out pio_base_addr1+2
#define CB_SN    3   // sector number    in/out pio_base_addr1+3
#define CB_CL    4   // cylinder low     in/out pio_base_addr1+4
#define CB_CH    5   // cylinder high    in/out pio_base_addr1+5
#define CB_DH    6   // device head      in/out pio_base_addr1+6
#define CB_STAT  7   // primary status   in     pio_base_addr1+7
#define CB_CMD   7   // command             out pio_base_addr1+7
#define CB_ASTAT 8   // alternate status in     pio_base_addr2+6
#define CB_DC    8   // device control      out pio_base_addr2+6
#define CB_DA    9   // device address   in     pio_base_addr2+7

// error reg (CB_ERR) bits

#define CB_ER_ICRC 0x80    // ATA Ultra DMA bad CRC
#define CB_ER_BBK  0x80    // ATA bad block
#define CB_ER_UNC  0x40    // ATA uncorrected error
#define CB_ER_MC   0x20    // ATA media change
#define CB_ER_IDNF 0x10    // ATA id not found
#define CB_ER_MCR  0x08    // ATA media change request
#define CB_ER_ABRT 0x04    // ATA command aborted
#define CB_ER_NTK0 0x02    // ATA track 0 not found
#define CB_ER_NDAM 0x01    // ATA address mark not found

#define CB_ER_P_SNSKEY 0xf0   // ATAPI sense key (mask)
#define CB_ER_P_MCR    0x08   // ATAPI Media Change Request
#define CB_ER_P_ABRT   0x04   // ATAPI command abort
#define CB_ER_P_EOM    0x02   // ATAPI End of Media
#define CB_ER_P_ILI    0x01   // ATAPI Illegal Length Indication

// ATAPI Interrupt Reason bits in the Sector Count reg (CB_SC)

#define CB_SC_P_TAG    0xf8   // ATAPI tag (mask)
#define CB_SC_P_REL    0x04   // ATAPI release
#define CB_SC_P_IO     0x02   // ATAPI I/O
#define CB_SC_P_CD     0x01   // ATAPI C/D

// bits 7-4 of the device/head (CB_DH) reg

#define CB_DH_LBA        0x40    // LBA bit
#define CB_DH_DEV0       0x00    // select device 0
#define CB_DH_DEV1       0x10    // select device 1
#define CB_DH_OBSOLETE   0xa0    // bits 7 and 5 both 1 (obsolete)

// status reg (CB_STAT and CB_ASTAT) bits

#define CB_STAT_BSY  0x80  // busy
#define CB_STAT_RDY  0x40  // ready
#define CB_STAT_DF   0x20  // device fault
#define CB_STAT_WFT  0x20  // write fault (old name)
#define CB_STAT_SKC  0x10  // seek complete (only SEEK command)
#define CB_STAT_SERV 0x10  // service (overlap/queued commands)
#define CB_STAT_DRQ  0x08  // data request
#define CB_STAT_CORR 0x04  // corrected (obsolete)
#define CB_STAT_IDX  0x02  // index (obsolete)
#define CB_STAT_ERR  0x01  // error (ATA)
#define CB_STAT_CHK  0x01  // check (ATAPI)

// device control reg (CB_DC) bits

#define CB_DC_HOB    0x80  // High Order Byte (48-bit LBA)
#define CB_DC_HD15   0x00  // bit 3 is reserved
// #define CB_DC_HD15   0x08  // (old definition of bit 3)
#define CB_DC_SRST   0x04  // soft reset
#define CB_DC_NIEN   0x02  // disable interrupts

// BMIDE registers and bits

#define BM_COMMAND_REG    0            // offset to BM command reg
#define BM_CR_MASK_READ    0x00           // read from memory
#define BM_CR_MASK_WRITE   0x08           // write to memory
#define BM_CR_MASK_START   0x01           // start transfer
#define BM_CR_MASK_STOP    0x00           // stop transfer

#define BM_STATUS_REG     2            // offset to BM status reg
#define BM_SR_MASK_SIMPLEX 0x80           // simplex only
#define BM_SR_MASK_DRV1    0x40           // drive 1 can do dma
#define BM_SR_MASK_DRV0    0x20           // drive 0 can do dma
#define BM_SR_MASK_INT     0x04           // INTRQ signal asserted
#define BM_SR_MASK_ERR     0x02           // error
#define BM_SR_MASK_ACT     0x01           // active

#define BM_PRD_ADDR_LOW   4            // offset to prd addr reg low 16 bits
#define BM_PRD_ADDR_HIGH  6            // offset to prd addr reg high 16 bits

//**************************************************************
//
// Most mandtory and optional ATA commands
//
//**************************************************************

#define CMD_CFA_ERASE_SECTORS            0xC0
#define CMD_CFA_REQUEST_EXT_ERR_CODE     0x03
#define CMD_CFA_TRANSLATE_SECTOR         0x87
#define CMD_CFA_WRITE_MULTIPLE_WO_ERASE  0xCD
#define CMD_CFA_WRITE_SECTORS_WO_ERASE   0x38
#define CMD_CHECK_POWER_MODE1            0xE5
#define CMD_CHECK_POWER_MODE2            0x98
#define CMD_DEVICE_RESET                 0x08
#define CMD_EXECUTE_DEVICE_DIAGNOSTIC    0x90
#define CMD_FLUSH_CACHE                  0xE7
#define CMD_FLUSH_CACHE_EXT              0xEA
#define CMD_FORMAT_TRACK                 0x50
#define CMD_IDENTIFY_DEVICE              0xEC
#define CMD_IDENTIFY_DEVICE_PACKET       0xA1
#define CMD_IDENTIFY_PACKET_DEVICE       0xA1
#define CMD_IDLE1                        0xE3
#define CMD_IDLE2                        0x97
#define CMD_IDLE_IMMEDIATE1              0xE1
#define CMD_IDLE_IMMEDIATE2              0x95
#define CMD_INITIALIZE_DRIVE_PARAMETERS  0x91
#define CMD_INITIALIZE_DEVICE_PARAMETERS 0x91
#define CMD_NOP                          0x00
#define CMD_PACKET                       0xA0
#define CMD_READ_BUFFER                  0xE4
#define CMD_READ_DMA                     0xC8
#define CMD_READ_DMA_EXT                 0x25
#define CMD_READ_MULTIPLE                0xC4
#define CMD_READ_MULTIPLE_EXT            0x29
#define CMD_READ_SECTORS                 0x20
#define CMD_READ_SECTORS_EXT             0x24
#define CMD_READ_VERIFY_SECTORS          0x40
#define CMD_READ_VERIFY_SECTORS_EXT      0x42
#define CMD_RECALIBRATE                  0x10
#define CMD_SEEK                         0x70
#define CMD_SET_FEATURES                 0xEF
#define CMD_SET_MULTIPLE_MODE            0xC6
#define CMD_SLEEP1                       0xE6
#define CMD_SLEEP2                       0x99
#define CMD_SMART                        0xB0
#define CMD_STANDBY1                     0xE2
#define CMD_STANDBY2                     0x96
#define CMD_STANDBY_IMMEDIATE1           0xE0
#define CMD_STANDBY_IMMEDIATE2           0x94
#define CMD_WRITE_BUFFER                 0xE8
#define CMD_WRITE_DMA                    0xCA
#define CMD_WRITE_DMA_EXT                0x35
#define CMD_WRITE_DMA_FUA_EXT            0x3D
#define CMD_WRITE_MULTIPLE               0xC5
#define CMD_WRITE_MULTIPLE_EXT           0x39
#define CMD_WRITE_MULTIPLE_FUA_EXT       0xCE
#define CMD_WRITE_SECTORS                0x30
#define CMD_WRITE_SECTORS_EXT            0x34
#define CMD_WRITE_VERIFY                 0x3C

#define CMD_SRST                          256   // fake command code for Soft Reset

//**************************************************************
//
// Public functions in ATAIOISA.C
//
//**************************************************************

extern int dma_isa_config( int chan );

extern int dma_isa_chs( int dev, int cmd,
                        unsigned int fr, unsigned int sc,
                        unsigned int cyl, unsigned int head, unsigned int sect,
                        unsigned int seg, unsigned int off,
                        long numSect );

extern int dma_isa_lba28( int dev, int cmd,
                          unsigned int fr, unsigned int sc,
                          unsigned long lba,
                          unsigned int seg, unsigned int off,
                          long numSect );

extern int dma_isa_lba48( int dev, int cmd,
                          unsigned int fr, unsigned int sc,
                          unsigned long lbahi, unsigned long lbalo,
                          unsigned int seg, unsigned int off,
                          long numSect );

extern int dma_isa_packet( int dev,
                           unsigned int cpbc,
                           unsigned int cpseg, unsigned int cpoff,
                           int dir,
                           long dpbc,
                           unsigned int dpseg, unsigned int dpoff,
                           unsigned long lba );

//**************************************************************
//
// Public data in ATAIOINT.C
//
//**************************************************************

// Interrupt mode flag (interrupts in use if != 0 )
// This value is READ ONLY - do not change.

extern int int_use_intr_flag;

//**************************************************************
//
// Public functions in ATAIOINT.C
//
//**************************************************************

extern int int_enable_irq( int shared, int irqNum,
                           unsigned int bmAddr, unsigned int ataAddr );

extern void int_disable_irq( void );

//**************************************************************
//
// Public data in ATAIOPCI.C
//
//**************************************************************

extern int dma_pci_prd_type;     // type of PRD list to use
#define PRD_TYPE_SIMPLE  0       // use PRD buffer, simple list
#define PRD_TYPE_COMPLEX 1       // use PRD buffer, complex list
#define PRD_TYPE_LARGE   2       // use I/O buffer, overlay I/O buffer

//extern long dma_pci_largeMaxB;   // max LARGE dma transfer size in bytes
//extern long dma_pci_largeMaxS;   // max LARGE dma transfer size in sectors

#if 0
extern unsigned long far * dma_pci_largePrdBufPtr;  // LARGE PRD buffer ptr
extern unsigned char far * dma_pci_largeIoBufPtr;   // LARGE PRD I/O address

extern unsigned long far * dma_pci_prd_ptr;  // current PRD buffer address
#else
// TODO - replace with physical addr?
#endif

extern int dma_pci_num_prd;                  // current number of PRD entries

//**************************************************************
//
// Public functions in ATAIOPCI.C
//
//**************************************************************

extern int dma_pci_config( unsigned int regAddr );

extern void dma_pci_set_max_xfer( unsigned int seg, unsigned int off,
                                  long bufSize );
/*
extern int dma_pci_chs( int dev, int cmd,
                        unsigned int fr, unsigned int sc,
                        unsigned int cyl, unsigned int head, unsigned int sect,
                        //unsigned int seg, unsigned int off,
                        void *physaddr,
                        long numSect );
*/
extern int dma_pci_lba28( int dev, int cmd,
                          unsigned int fr, unsigned int sc,
                          unsigned long lba,
                          //unsigned int seg, unsigned int off,
                          physaddr_t physaddr,
                          long numSect );

extern int dma_pci_lba48( int dev, int cmd,
                          unsigned int fr, unsigned int sc,
                          unsigned long lbahi, unsigned long lbalo,
                          physaddr_t physAddr,
                          //unsigned int seg, unsigned int off,
                          long numSect );

extern int dma_pci_packet( int dev,
                           unsigned int cpbc,
                           //unsigned int cpseg, unsigned int cpoff,
                           unsigned char * cpAddr,
                           int dir,
                           long dpbc,
                           physaddr_t dp_physAddr,
//                           unsigned int dpseg, unsigned int dpoff,
                           unsigned long lba );

//**************************************************************
//
// Public data in ATAIOPIO.C
//
//**************************************************************

extern unsigned int pio_base_addr1;
extern unsigned int pio_base_addr2;

//extern unsigned int pio_memory_seg;
extern int pio_memory_dt_opt;
#define PIO_MEMORY_DT_OPT0 0  // use Data reg at offset 0H
#define PIO_MEMORY_DT_OPT8 1  // use Data reg at offset 8H
#define PIO_MEMORY_DT_OPTB 2  // use Data reg at offsets 400-7ffH
#define PIO_MEMORY_DT_OPTR 3  // randomly select these options

extern unsigned int pio_bmide_base_addr;

extern void * pio_reg_addrs[10];

extern unsigned char pio_last_write[10];
extern unsigned char pio_last_read[10];

extern int pio_xfer_width;

//**************************************************************
//
// Public functions in ATAIOPIO.C
//
//**************************************************************

// configuration functions

extern void pio_set_iobase_addr( unsigned int base1,
                                 unsigned int base2,
                                 unsigned int base3 );

//extern void pio_set_memory_addr( unsigned int seg );

// normal register read/write functions

extern unsigned char pio_inbyte( unsigned int addr );

extern void pio_outbyte( unsigned int addr, unsigned char data );

extern unsigned int pio_inword( unsigned int addr );

extern void pio_outword( unsigned int addr, unsigned int data );

// Normal PIO DRQ block transfer functions. These functions
// handle both PCMCIA memory mode and all I/O mode transfers.

extern void pio_drq_block_in( unsigned int addrDataReg,
                              void *addr,
                              long wordCnt );

extern void pio_drq_block_out( unsigned int addrDataReg,
                              void *addr,
                               long wordCnt );

// These functions can be called directly but are normally
// called by the pio_drq_block_in() and pio_drq_block_out()
// functions. These functions handle only I/O mode transfers.

extern void pio_rep_inbyte( unsigned int addrDataReg,
                            void *addr,
                            long byteCnt );

extern void pio_rep_outbyte( unsigned int addrDataReg,
                            void *addr,
                             long byteCnt );

extern void pio_rep_inword( unsigned int addrDataReg,
                            void *addr,
                            long wordCnt );

extern void pio_rep_outword( unsigned int addrDataReg,
                            void *addr,
                             long wordCnt );

extern void pio_rep_indword( unsigned int addrDataReg,
                            void *addr,
                             long dwordCnt );

extern void pio_rep_outdword( unsigned int addrDataReg,
                            void *addr,
                              long dwordCnt );

//**************************************************************
//
// Public data in ATAIOREG.C
//
//**************************************************************

// last ATAPI command packet size and data

extern int reg_atapi_cp_size;
extern unsigned char reg_atapi_cp_data[16];

// flag to control the ~110ms delay for ATAPI devices,
// no delay if the flag is zero.

extern int reg_atapi_delay_flag;

// the values in these variables are placed into the Feature,
// Sector Count, Sector Number and Device/Head register by
// reg_packet() before the A0H command is issued.  reg_packet()
// sets these variables to zero before returning.  These variables
// are initialized to zero.  Only bits 3,2,1,0 of reg_atapi_reg_dh
// are used.

extern unsigned char reg_atapi_reg_fr;
extern unsigned char reg_atapi_reg_sc;
extern unsigned char reg_atapi_reg_sn;
extern unsigned char reg_atapi_reg_dh;

extern long reg_buffer_size;

// Extended error information returned by
// reg_reset(), reg_non_data_*(), reg_pio_data_in_*(),
// reg_pio_data_out_*(), reg_packet(), dma_isa_*()
// and dma_pci_*() functions.

struct REG_CMD_INFO
{
   // entry type, flag and command code
   unsigned char flg;         // see TRC_FLAG_xxx in ataio.h
   unsigned char ct;          // see TRC_TYPE_xxx in ataio.h
   unsigned char cmd;         // command code
   // before regs
   unsigned int  fr1;         // feature (8 or 16 bits)
   unsigned int  sc1;         // sec cnt (8 or 16 bits)
   unsigned char sn1;         // sec num
   unsigned char cl1;         // cyl low
   unsigned char ch1;         // cyl high
   unsigned char dh1;         // device head
   unsigned char dc1;         // device control
   // after regs
   unsigned char st2;         // status reg
   unsigned char as2;         // alt status reg
   unsigned char er2;         // error reg
   unsigned int  sc2;         // sec cnt (8 or 16 bits)
   unsigned char sn2;         // sec num
   unsigned char cl2;         // cyl low
   unsigned char ch2;         // cyl high
   unsigned char dh2;         // device head
   // driver error codes
   unsigned char ec;          // detailed error code
   unsigned char to;          // not zero if time out error
   // additional result info
   long totalBytesXfer;       // total bytes transfered
   long drqPackets;           // number of PIO DRQ packets
   long drqPacketSize;        // number of bytes in current DRQ block
   unsigned int failbits;     // failure bits (protocol errors)
      #define FAILBIT15 0x8000   // extra interrupts detected
      #define FAILBIT14 0x4000
      #define FAILBIT13 0x2000
      #define FAILBIT12 0x1000
      #define FAILBIT11 0x0800
      #define FAILBIT10 0x0400
      #define FAILBIT9  0x0200
      #define FAILBIT8  0x0100   // SC( CD/IO bits) wrong at end of cmd
      #define FAILBIT7  0x0080   // byte count odd at data packet xfer time
      #define FAILBIT6  0x0040   // byte count wrong at data packet xfer time
      #define FAILBIT5  0x0020   // SC (IO bit) wrong at data packet xfer time
      #define FAILBIT4  0x0010   // SC (CD bit) wrong at data packet xfer time
      #define FAILBIT3  0x0008   // byte count wrong at cmd packet xfer time
      #define FAILBIT2  0x0004   // SC wrong at cmd packet xfer time
      #define FAILBIT1  0x0002   // got interrupt before cmd packet xfer
      #define FAILBIT0  0x0001   // slow setting BSY=1 or DRQ=1 after AO cmd
   // sector count, multiple count, and CHS/LBA info
   long ns;                   // number of sectors (sector count)
   int mc;                    // multiple count
   unsigned char lbaSize;     // size of LBA used
      #define LBACHS 0           // last command used CHS
      #define LBA28  28          // last command used 28-bit LBA
      #define LBA32  32          // last command used 32-bit LBA (Packet)
      #define LBA48  48          // last command used 48-bit LBA
      #define LBA64  64          // future use?
   unsigned long lbaLow1;     // lower 32-bits of LBA before
   unsigned long lbaHigh1;    // upper 32-bits of LBA before
   unsigned long lbaLow2;     // lower 32-bits of LBA after
   unsigned long lbaHigh2;    // upper 32-bits of LBA after
} ;

extern struct REG_CMD_INFO reg_cmd_info;

// Configuration data for device 0 and 1
// returned by the reg_config() function.

extern int reg_config_info[2];

#define REG_CONFIG_TYPE_NONE  0
#define REG_CONFIG_TYPE_UNKN  1
#define REG_CONFIG_TYPE_ATA   2
#define REG_CONFIG_TYPE_ATAPI 3

// flag to control the slow data transfer option:
// 0 = no slow data transfer
// !0= slow data transfer before this DRQ packet

extern long reg_slow_xfer_flag;

// flag bits for 'incompatible' controllers and devices

extern int reg_incompat_flags;   // see #defines...

#define REG_INCOMPAT_DMA_DELAY   0x0001   // set to 1 for delay before
                                          // and after stopping the
                                          // DMA engine
#define REG_INCOMPAT_DMA_POLL    0x0002   // set to 1 for no polling
                                          // of Alt Status during
                                          // DMA transfers

#define REG_INCOMPAT_DEVREG      0x0004   // set bits 7 and 5 to
                                          // to 1 in the Device
                                          // (Drive/Head) register

//**************************************************************
//
// Public functions in ATAIOREG.C
//
//**************************************************************

// config and reset funcitons

extern int reg_config( void );

extern int reg_reset( int skipFlag, int devRtrn );

// ATA Non-Data command funnctions (for CHS, LBA28 and LBA48)

extern int reg_non_data_chs( int dev, int cmd,
                             unsigned int fr, unsigned int sc,
                             unsigned int cyl, unsigned int head, unsigned int sect );

extern int reg_non_data_lba28( int dev, int cmd,
                               unsigned int fr, unsigned int sc,
                               unsigned long lba );

extern int reg_non_data_lba48( int dev, int cmd,
                               unsigned int fr, unsigned int sc,
                               unsigned long lbahi, unsigned long lbalo );

// PIO Data In/Out data transfer call back function

extern void ( * reg_drq_block_call_back ) ( struct REG_CMD_INFO * );

// ATA PIO Data In command functions (for CHS, LBA28 and LBA48)

extern int reg_pio_data_in_chs( int dev, int cmd,
                                unsigned int fr, unsigned int sc,
                                unsigned int cyl, unsigned int head, unsigned int sect,
                                unsigned int seg, unsigned int off,
                                long numSect, int multiCnt );

extern int reg_pio_data_in_lba28( int dev, int cmd,
                                  unsigned int fr, unsigned int sc,
                                  unsigned long lba,
                                  //unsigned int seg, unsigned int off,
                                  void *addr,
                                  long numSect, int multiCnt );

extern int reg_pio_data_in_lba48( int dev, int cmd,
                                  unsigned int fr, unsigned int sc,
                                  unsigned long lbahi, unsigned long lbalo,
                                  //unsigned int seg, unsigned int off,
                                  void *addr,
                                  long numSect, int multiCnt );

// ATA PIO Data Out command functions (for CHS, LBA28 and LBA48)

extern int reg_pio_data_out_chs( int dev, int cmd,
                                 unsigned int fr, unsigned int sc,
                                 unsigned int cyl, unsigned int head, unsigned int sect,
                                 unsigned int seg, unsigned int off,
                                 long numSect, int multiCnt );

extern int reg_pio_data_out_lba28( int dev, int cmd,
                                   unsigned int fr, unsigned int sc,
                                   unsigned long lba,
                                   //unsigned int seg, unsigned int off,
                                   void *addr,
                                   long numSect, int multiCnt );

extern int reg_pio_data_out_lba48( int dev, int cmd,
                                   unsigned int fr, unsigned int sc,
                                   unsigned long lbahi, unsigned long lbalo,
                                   void *addr,
                                   long numSect, int multiCnt );

// ATAPI Packet PIO function

extern int reg_packet( int dev,
                       unsigned int cpbc,
                       void *cpAddr,
                       //unsigned int cpseg, unsigned int cpoff,
                       int dir,
                       long dpbc,
                       void *dpAddr,
                       //unsigned int dpseg, unsigned int dpoff,
                       unsigned long lba );

//**************************************************************
//
// Public data in ATAIOTMR.C
//
//**************************************************************

extern long tmr_time_out;           // command time out in seconds

extern long tmr_cmd_start_time;     // command start time

extern long tmr_1s_count;           // number of I/O port reads required
                                    //    for a 1s delay
extern long tmr_1ms_count;          // number of I/O port reads required
                                    //    for a 1ms delay
extern long tmr_1us_count;          // number of I/O port reads required
                                    //    for a 1us delay
extern long tmr_500ns_count;        // number of I/O port reads required
                                    //    for a 500ns delay

//**************************************************************
//
// Public functions in ATAIOTMR.C
//
//**************************************************************

extern long tmr_read_bios_timer( void );

extern void tmr_set_timeout( void );

extern int tmr_chk_timeout( void );

extern void tmr_get_delay_counts( void );

extern void tmr_delay_1ms( long count );

extern void tmr_delay_1us( long count );

extern void tmr_delay_ata( void );

extern void tmr_delay_atapi( int dev );

extern void tmr_delay_xfer( void );

//**************************************************************
//
// Public functions in ATAIOTRC.C
//
//**************************************************************

// number of commands in lookup table
#define TRC_NUM_CMDS 52

// macro to convert a command code to a command index
#define TRC_CC2NDX(cc) (trc_CmdCodeNdx[cc])

// command code to command index table
extern char trc_CmdCodeNdx[256];

// command history trace entry types used by reg_cmd_info.flg,
// trc_get_cmd_name(), etc.
#define TRC_FLAG_EMPTY 0
#define TRC_FLAG_SRST  1
#define TRC_FLAG_ATA   2
#define TRC_FLAG_ATAPI 3

// command types (protocol types) used by reg_cmd_info.ct,
// trc_cht_types(), etc.  this is a bit shift value.
// NOTE: also see trc_get_type_name() in ATAIOTRC.C !
#define TRC_TYPE_ALL    16    // trace all cmd types
#define TRC_TYPE_NONE    0    // don't trace any cmd types
#define TRC_TYPE_ADMAI   1    // ATA DMA In
#define TRC_TYPE_ADMAO   2    // ATA DMA Out
#define TRC_TYPE_AND     3    // ATA PIO Non-Data
#define TRC_TYPE_APDI    4    // ATA PIO Data In
#define TRC_TYPE_APDO    5    // ATA PIO Data Out
#define TRC_TYPE_ASR     6    // ATA Soft Reset
#define TRC_TYPE_PDMAI   7    // ATAPI DMA In
#define TRC_TYPE_PDMAO   8    // ATAPI DMA Out
#define TRC_TYPE_PND     9    // ATAPI PIO Non-Data
#define TRC_TYPE_PPDI   10    // ATAPI PIO Data In
#define TRC_TYPE_PPDO   11    // ATAPI PIO Data Out

extern const char * trc_get_type_name( unsigned char ct );

extern const char * trc_get_cmd_name( unsigned int cc );

extern const char * trc_get_st_bit_name( unsigned char st );

extern const char * trc_get_er_bit_name( unsigned char er );

extern const char * trc_get_err_name( int ec );

extern void trc_err_dump1( void );
extern const char * trc_err_dump2( void );

extern void trc_cht_types( int type );
extern void trc_cht_dump0( void );
extern void trc_cht_dump1( void );
extern const char * trc_cht_dump2( void );

extern void trc_llt_dump0( void );
extern void trc_llt_dump1( void );
extern const char * trc_llt_dump2( void );

//********************************************************************
//
// The remainder of this file is ATADRVR's private data -
// this data and these functions should not be used outside
// of the ATADRVR source files.
//
//********************************************************************

// macro used to read Alt Status, delay and read Alt Status again,
// this delay (~400ns) is required by the older ATA command protocols.

#define ATA_DELAY() {pio_inbyte(CB_ASTAT);tmr_delay_ata();pio_inbyte(CB_ASTAT);}

// marco used to read Alt Status, delay and read Alt Status again,
// this delay (~80ms) is required for some poorly designed ATAPI
// device to get the status updated correctly.

#define ATAPI_DELAY(dev) {pio_inbyte(CB_ASTAT);tmr_delay_atapi(dev);pio_inbyte(CB_ASTAT);}

//**************************************************************
//
// Private data in ATAIOINT.C
//
//**************************************************************

// Interrupt counter (incremented each time there is an interrupt)

extern volatile int int_intr_cntr;

// Interrupt flag (!= 0 if there was an interrupt)

extern volatile int int_intr_flag;

// BMIDE Status register I/O address

extern unsigned int int_bmide_addr;

// BMIDE Status register at time of last interrupt

extern volatile unsigned char int_bm_status;

// ATA Status register I/O address

extern unsigned int int_ata_addr;

// ATA Status register at time of last interrupt

extern volatile unsigned char int_ata_status;

//**************************************************************
//
// Private functions in ATAIOINT.C
//
//**************************************************************

extern void int_save_int_vect( void );

extern void int_restore_int_vect( void );

//**************************************************************
//
// Private data in ATAIOSUB.C
//
//**************************************************************

// Private functions in ATAIOSUB.C

extern void sub_zero_return_data( void );
extern void sub_setup_command( void );
extern void sub_trace_command( void );
extern int sub_select( int dev );
extern unsigned char sub_readBusMstrCmd( void );
extern unsigned char sub_readBusMstrStatus( void );
extern void sub_writeBusMstrCmd( unsigned char x );
extern void sub_writeBusMstrStatus( unsigned char x );

//**************************************************************
//
// Private data in ATAIOTRC.C
//
//**************************************************************

// low level trace entry type values

#define TRC_LLT_NONE     0  // unused entry

#define TRC_LLT_INB      1  // in byte
#define TRC_LLT_OUTB     2  // out byte
#define TRC_LLT_INW      3  // in word
#define TRC_LLT_OUTW     4  // out word
#define TRC_LLT_INSB     5  // rep in byte
#define TRC_LLT_OUTSB    6  // rep out byte
#define TRC_LLT_INSW     7  // rep in word
#define TRC_LLT_OUTSW    8  // rep out word
#define TRC_LLT_INSD     9  // rep in dword
#define TRC_LLT_OUTSD   10  // rep out dword

#define TRC_LLT_S_CFG   11  // start config
#define TRC_LLT_S_RST   12  // start reset
#define TRC_LLT_S_ND    13  // start ND cmd
#define TRC_LLT_S_PDI   14  // start PDI cmd
#define TRC_LLT_S_PDO   15  // start PDO cmd
#define TRC_LLT_S_PI    16  // start packet cmd
#define TRC_LLT_S_RWD   17  // start ata R/W DMA cmd
#define TRC_LLT_S_PID   18  // start packet DMA cmd
#define TRC_LLT_WINT    21  // wait for int
#define TRC_LLT_INTRQ   22  // int received
#define TRC_LLT_PNBSY   23  // poll for not busy
#define TRC_LLT_PRDY    24  // poll for ready
#define TRC_LLT_TOUT    25  // timeout
#define TRC_LLT_ERROR   26  // error detected
#define TRC_LLT_DELAY1  27  // delay ~110ms
#define TRC_LLT_DELAY2  28  // delay 0-55ms
#define TRC_LLT_E_CFG   31  // end   config
#define TRC_LLT_E_RST   32  // end   reset
#define TRC_LLT_E_ND    33  // end   ND cmd
#define TRC_LLT_E_PDI   34  // end   PDI cmd
#define TRC_LLT_E_PDO   35  // end   PDO cmd
#define TRC_LLT_E_PI    36  // end   packet CMD
#define TRC_LLT_E_RWD   37  // end   ata R/W DMA cmd
#define TRC_LLT_E_PID   38  // end   packet DMA cmd

#define TRC_LLT_DMA1    41  // enable/start ISA DMA channel
#define TRC_LLT_DMA2    42  // poll the ISA DMA TC bit
#define TRC_LLT_DMA3    43  // disable/stop ISA DMA channel

#define TRC_LLT_DEBUG   51  // debug data
#define TRC_LLT_P_CMD   52  // packet cmd code
#define TRC_LLT_R_BM_CR 53  // read bus master cmd reg
#define TRC_LLT_R_BM_SR 54  // read bus master status reg
#define TRC_LLT_W_BM_CR 55  // write bus master cmd reg
#define TRC_LLT_W_BM_SR 56  // write bus master status reg

//**************************************************************
//
// Private functions in ATAIOTRC.C
//
//**************************************************************

extern void trc_cht( void );

extern void trc_llt( unsigned char addr,
                     unsigned char data,
                     unsigned char type );

// end ataio.h
