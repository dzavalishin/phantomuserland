#define TULIP_IOTYPE  PCI_USES_MASTER | PCI_USES_IO | PCI_ADDR0
#define TULIP_SIZE 0x80

/* This is a mysterious value that can be written to CSR11 in the 21040 (only)
   to support a pre-NWay full-duplex signaling mechanism using short frames.
   No one knows what it should be, but if left at its default value some
   10base2(!) packets trigger a full-duplex-request interrupt. */
#define FULL_DUPLEX_MAGIC       0x6969

static const int csr0 = 0x01A00000 | 0x8000;

/*  The possible media types that can be set in options[] are: */
#define MEDIA_MASK 31
static const char * const medianame[32] = {
    "10baseT", "10base2", "AUI", "100baseTx",
    "10baseT-FDX", "100baseTx-FDX", "100baseT4", "100baseFx",
    "100baseFx-FDX", "MII 10baseT", "MII 10baseT-FDX", "MII",
    "10baseT(forced)", "MII 100baseTx", "MII 100baseTx-FDX", "MII 100baseT4",
    "MII 100baseFx-HDX", "MII 100baseFx-FDX", "Home-PNA 1Mbps", "Invalid-19",
};

/* This much match tulip_tbl[]!  Note 21142 == 21143. */
enum tulip_chips {
    DC21040=0, DC21041=1, DC21140=2, DC21142=3, DC21143=3,
    LC82C168, MX98713, MX98715, MX98725, AX88141, AX88140, PNIC2, COMET,
    COMPEX9881, I21145, XIRCOM
};

enum pci_id_flags_bits {
    /* Set PCI command register bits before calling probe1(). */
    PCI_USES_IO=1, PCI_USES_MEM=2, PCI_USES_MASTER=4,
    /* Read and map the single following PCI BAR. */
    PCI_ADDR0=0<<4, PCI_ADDR1=1<<4, PCI_ADDR2=2<<4, PCI_ADDR3=3<<4,
    PCI_ADDR_64BITS=0x100, PCI_NO_ACPI_WAKE=0x200, PCI_NO_MIN_LATENCY=0x400,
    PCI_UNUSED_IRQ=0x800,
};

struct pci_id_info {
    char *name;
    struct match_info {
        u32 pci, pci_mask, subsystem, subsystem_mask;
        u32 revision, revision_mask;                            /* Only 8 bits. */
    } id;
    enum pci_id_flags_bits pci_flags;
    int io_size;                                /* Needed for I/O region check or ioremap(). */
    int drv_flags;                              /* Driver use, intended as capability flags. */
};

static struct pci_id_info pci_id_tbl[] = {
    { "Digital DC21040 Tulip", { 0x00021011, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 0x80, DC21040 },
    { "Digital DC21041 Tulip", { 0x00141011, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 0x80, DC21041 },
    { "Digital DS21140A Tulip", { 0x00091011, 0xffffffff, 0,0, 0x20,0xf0 },
      TULIP_IOTYPE, 0x80, DC21140 },
    { "Digital DS21140 Tulip", { 0x00091011, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 0x80, DC21140 },
    { "Digital DS21143 Tulip", { 0x00191011, 0xffffffff, 0,0, 65,0xff },
      TULIP_IOTYPE, TULIP_SIZE, DC21142 },
    { "Digital DS21142 Tulip", { 0x00191011, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, TULIP_SIZE, DC21142 },
    { "Kingston KNE110tx (PNIC)", { 0x000211AD, 0xffffffff, 0xf0022646, 0xffffffff, 0, 0 },
      TULIP_IOTYPE, 256, LC82C168 },
    { "Lite-On 82c168 PNIC", { 0x000211AD, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 256, LC82C168 },
    { "Macronix 98713 PMAC", { 0x051210d9, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 256, MX98713 },
    { "Macronix 98715 PMAC", { 0x053110d9, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 256, MX98715 },
    { "Macronix 98725 PMAC", { 0x053110d9, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 256, MX98725 },
    { "ASIX AX88141", { 0x1400125B, 0xffffffff, 0,0, 0x10, 0xf0 },
      TULIP_IOTYPE, 128, AX88141 },
    { "ASIX AX88140", { 0x1400125B, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 128, AX88140 },
    { "Lite-On LC82C115 PNIC-II", { 0xc11511AD, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 256, PNIC2 },
    { "ADMtek AN981 Comet", { 0x09811317, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 256, COMET },
    { "ADMTek AN983 Comet", { 0x12161113, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 256, COMET },
    { "ADMtek Centaur-P", { 0x09851317, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 256, COMET },
    { "ADMtek Centaur-C", { 0x19851317, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 256, COMET },
    { "Compex RL100-TX", { 0x988111F6, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 128, COMPEX9881 },
    { "Intel 21145 Tulip", { 0x00398086, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 128, I21145 },
    { "Xircom Tulip clone", { 0x0003115d, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 128, XIRCOM },
    { "Davicom DM9102", { 0x91021282, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 0x80, DC21140 },
    { "Davicom DM9100", { 0x91001282, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 0x80, DC21140 },
    { "Macronix mxic-98715 (EN1217)", { 0x12171113, 0xffffffff, 0, 0, 0, 0 },
      TULIP_IOTYPE, 256, MX98715 },
    { 0, { 0, 0, 0, 0, 0, 0 }, 0, 0, 0 },
};

enum tbl_flag {
    HAS_MII=1, HAS_MEDIA_TABLE=2, CSR12_IN_SROM=4, ALWAYS_CHECK_MII=8,
    HAS_PWRDWN=0x10, MC_HASH_ONLY=0x20, /* Hash-only multicast filter. */
    HAS_PNICNWAY=0x80, HAS_NWAY=0x40,   /* Uses internal NWay xcvr. */
    HAS_INTR_MITIGATION=0x100, IS_ASIX=0x200, HAS_8023X=0x400,
};

/* Note: this table must match  enum tulip_chips  above. */
static struct tulip_chip_table {
    char *chip_name;
    int flags;
} tulip_tbl[] = {
    { "Digital DC21040 Tulip", 0},
    { "Digital DC21041 Tulip", HAS_MEDIA_TABLE | HAS_NWAY },
    { "Digital DS21140 Tulip", HAS_MII | HAS_MEDIA_TABLE | CSR12_IN_SROM },
    { "Digital DS21143 Tulip", HAS_MII | HAS_MEDIA_TABLE | ALWAYS_CHECK_MII 
      | HAS_PWRDWN | HAS_NWAY   | HAS_INTR_MITIGATION },
    { "Lite-On 82c168 PNIC", HAS_MII | HAS_PNICNWAY },
    { "Macronix 98713 PMAC", HAS_MII | HAS_MEDIA_TABLE | CSR12_IN_SROM },
    { "Macronix 98715 PMAC", HAS_MEDIA_TABLE },
    { "Macronix 98725 PMAC", HAS_MEDIA_TABLE },
    { "ASIX AX88140", HAS_MII | HAS_MEDIA_TABLE | CSR12_IN_SROM 
      | MC_HASH_ONLY | IS_ASIX },
    { "ASIX AX88141", HAS_MII | HAS_MEDIA_TABLE | CSR12_IN_SROM | MC_HASH_ONLY 
      | IS_ASIX },
    { "Lite-On PNIC-II", HAS_MII | HAS_NWAY | HAS_8023X },
    { "ADMtek Comet", MC_HASH_ONLY },
    { "Compex 9881 PMAC",       HAS_MII | HAS_MEDIA_TABLE | CSR12_IN_SROM },
    { "Intel DS21145 Tulip", HAS_MII | HAS_MEDIA_TABLE | ALWAYS_CHECK_MII 
      | HAS_PWRDWN | HAS_NWAY },
    { "Xircom tulip work-alike", HAS_MII | HAS_MEDIA_TABLE | ALWAYS_CHECK_MII 
      | HAS_PWRDWN | HAS_NWAY },
    { 0, 0 },
};

/* A full-duplex map for media types. */
enum MediaIs {
    MediaIsFD = 1, MediaAlwaysFD=2, MediaIsMII=4, MediaIsFx=8,
    MediaIs100=16};

static const char media_cap[32] =
{0,0,0,16,  3,19,16,24,  27,4,7,5, 0,20,23,20, 20,31,0,0, };
static u8 t21040_csr13[] = {2,0x0C,8,4,  4,0,0,0, 0,0,0,0, 4,0,0,0};

/* 21041 transceiver register settings: 10-T, 10-2, AUI, 10-T, 10T-FD */
static u16 t21041_csr13[] = { 0xEF01, 0xEF09, 0xEF09, 0xEF01, 0xEF09, };
static u16 t21041_csr14[] = { 0xFFFF, 0xF7FD, 0xF7FD, 0x7F3F, 0x7F3D, };
static u16 t21041_csr15[] = { 0x0008, 0x0006, 0x000E, 0x0008, 0x0008, };

/* not used
static u16 t21142_csr13[] = { 0x0001, 0x0009, 0x0009, 0x0000, 0x0001, };
*/
static u16 t21142_csr14[] = { 0xFFFF, 0x0705, 0x0705, 0x0000, 0x7F3D, };
/* not used
static u16 t21142_csr15[] = { 0x0008, 0x0006, 0x000E, 0x0008, 0x0008, };
*/

/* Offsets to the Command and Status Registers, "CSRs".  All accesses
   must be longword instructions and quadword aligned. */
enum tulip_offsets {
    CSR0=0,     CSR1=0x08,  CSR2=0x10,  CSR3=0x18,  CSR4=0x20,  CSR5=0x28,
    CSR6=0x30,  CSR7=0x38,  CSR8=0x40,  CSR9=0x48, CSR10=0x50, CSR11=0x58,
    CSR12=0x60, CSR13=0x68, CSR14=0x70, CSR15=0x78, CSR16=0x80, CSR20=0xA0
};

/* The bits in the CSR5 status registers, mostly interrupt sources. */
enum status_bits {
    TimerInt=0x800, TPLnkFail=0x1000, TPLnkPass=0x10,
    NormalIntr=0x10000, AbnormalIntr=0x8000,
    RxJabber=0x200, RxDied=0x100, RxNoBuf=0x80, RxIntr=0x40,
    TxFIFOUnderflow=0x20, TxJabber=0x08, TxNoBuf=0x04, TxDied=0x02, TxIntr=0x01,
};

/* The configuration bits in CSR6. */
enum csr6_mode_bits {
	TxOn=0x2000, RxOn=0x0002, FullDuplex=0x0200,
	AcceptBroadcast=0x0100, AcceptAllMulticast=0x0080,
	AcceptAllPhys=0x0040, AcceptRunt=0x0008,
};


enum desc_status_bits {
    DescOwnded=0x80000000, RxDescFatalErr=0x8000, RxWholePkt=0x0300,
};

struct medialeaf {
    u8 type;
    u8 media;
    unsigned char *leafdata;
};

struct mediatable {
    u16 defaultmedia;
    u8 leafcount, csr12dir;                             /* General purpose pin directions. */
    unsigned has_mii:1, has_nonmii:1, has_reset:6;
    u32 csr15dir, csr15val;                             /* 21143 NWay setting. */
    struct medialeaf mleaf[0];
};

struct mediainfo {
    struct mediainfo *next;
    int info_type;
    int index;
    unsigned char *info;
};

/* EEPROM Address width definitions */
#define EEPROM_ADDRLEN 6
#define EEPROM_SIZE    128              /* 2 << EEPROM_ADDRLEN */

/* The EEPROM commands include the alway-set leading bit. */
#define EE_WRITE_CMD    (5 << addr_len)
#define EE_READ_CMD     (6 << addr_len)
#define EE_ERASE_CMD    (7 << addr_len)

/* EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK    0x02    /* EEPROM shift clock. */
#define EE_CS           0x01    /* EEPROM chip select. */
#define EE_DATA_WRITE   0x04    /* EEPROM chip data in. */
#define EE_WRITE_0      0x01
#define EE_WRITE_1      0x05
#define EE_DATA_READ    0x08    /* EEPROM chip data out. */
#define EE_ENB          (0x4800 | EE_CS)

/* Delay between EEPROM clock transitions.  Even at 33Mhz current PCI
   implementations don't overrun the EEPROM clock.  We add a bus
   turn-around to insure that this remains true.  */
#define eeprom_delay()  inl(ee_addr)

/* Size of transmit and receive buffers */
//#define BUFLEN 1536
// We alloc in pages, so make it bigger a bit
#define BUFLEN 2048

/* Ring-wrap flag in length field, use for last ring entry.
   0x01000000 means chain on buffer2 address,
   0x02000000 means use the ring start address in CSR2/3.
   Note: Some work-alike chips do not function correctly in chained mode.
   The ASIX chip works only in chained mode.
   Thus we indicate ring mode, but always write the 'next' field for
   chained mode as well. */
#define DESC_RING_WRAP 0x02000000

/* transmit and receive descriptor format */
struct tulip_rx_desc {
    volatile u32 status;
    u32 length;
    u32 buffer1, buffer2;
};

struct tulip_tx_desc {
    volatile u32 status;
    u32 length;
    u32 buffer1, buffer2;
};
