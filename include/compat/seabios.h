#ifndef COMPAT_SEABIOS_H
#define COMPAT_SEABIOS_H

#warning PIT_TICK_RATE and PIT_TICK_INTERVAL are wrong!

#define PIT_TICK_RATE 1193180   // Underlying HZ of PIT
#define PIT_TICK_INTERVAL 65536 // Default interval for 18.2Hz timer



#include <compat/shorttype-def.h>
#include <phantom_types.h>
#include <hal.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <threads.h>

#include <dev/pci/pci_regs.h>


#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10




#define CONFIG_USB 1
#define CONFIG_USB_EHCI 0
#define CONFIG_USB_OHCI 1
#define CONFIG_USB_UHCI 1
#define CONFIG_USB_MOUSE 1
#define CONFIG_USB_KEYBOARD 1
#define CONFIG_USB_HUB 1
#define CONFIG_USB_MSC 0

















#define dprintf SHOW_FLOW
#define msleep hal_sleep_msec
#define yield() hal_sleep_msec(1)
#define memalign_high memalign
#define memalign_low memalign
#define memalign_tmphigh memalign

#define mutex_lock hal_mutex_lock
#define mutex_unlock hal_mutex_unlock
#define malloc_high malloc
#define malloc_low malloc
#define malloc_tmphigh malloc
#define malloc_tmplow malloc
#define malloc_fseg malloc

//void *memalign( int align, size_t size );

#define run_thread(___code, ___arg) hal_start_thread( (___code), ___arg, 0 )


u64 calc_future_tsc(u32 msecs);
u64 calc_future_tsc_usec(u32 usecs);
int check_tsc(u64 end);



void pci_config_writel(u16 bdf, u32 addr, u32 val);
void pci_config_writew(u16 bdf, u32 addr, u16 val);
void pci_config_writeb(u16 bdf, u32 addr, u8 val);
u32 pci_config_readl(u16 bdf, u32 addr);
u16 pci_config_readw(u16 bdf, u32 addr);
u8 pci_config_readb(u16 bdf, u32 addr);
void pci_config_maskw(u16 bdf, u32 addr, u16 off, u16 on);




#ifndef NULL
#  define NULL 0
#endif

#define PACKED __attribute__((packed))

#define warn_noalloc() panic("no mem")
#define warn_internalerror() panic("internal error")
#define warn_timeout() SHOW_ERROR0( 1, "timeout")

#define ASSERT32FLAT() do {} while(0)
#define MODE16 0
#define ASSERT16() panic("16 bit USB code called")

#define VAR16VISIBLE
#define VAR16


// TODO check - Used by 16 bit only code?
#define FLATPTR_TO_SEG(v) (v)
#define FLATPTR_TO_OFFSET(v) (v)









#define GET_FARVAR(seg, var) \
    (*((typeof(&(var)))MAKE_FLATPTR((seg), &(var))))
#define SET_FARVAR(seg, var, val) \
    do { GET_FARVAR((seg), (var)) = (val); } while (0)
#define GET_VAR(seg, var) (var)
#define SET_VAR(seg, var, val) do { (var) = (val); } while (0)
#define SET_SEG(SEG, value) ((void)(value))
#define GET_SEG(SEG) 0
#define GET_FLATPTR(ptr) (ptr)
#define SET_FLATPTR(ptr, val) do { (ptr) = (val); } while (0)

// TODO check use!
#define MAKE_FLATPTR(seg,off) ((void*)(((u32)(seg)<<4)+(u32)(off)))



#define memcpy_far( __toseg, __tooff, __fromseg, __fromoff, __len ) memcpy( __tooff, __fromoff, __len )




#define noinline __attribute__((noinline))








static inline u8 pci_bdf_to_bus(u16 bdf) {
    return bdf >> 8;
}
static inline u8 pci_bdf_to_devfn(u16 bdf) {
    return bdf & 0xff;
}
static inline u16 pci_bdf_to_busdev(u16 bdf) {
    return bdf & ~0x07;
}
static inline u8 pci_bdf_to_dev(u16 bdf) {
    return (bdf >> 3) & 0x1f;
}
static inline u8 pci_bdf_to_fn(u16 bdf) {
    return bdf & 0x07;
}
static inline u16 pci_to_bdf(int bus, int dev, int fn) {
    return (bus<<8) | (dev<<3) | fn;
}
static inline u16 pci_bus_devfn_to_bdf(int bus, u16 devfn) {
    return (bus << 8) | devfn;
}

static inline u32 pci_vd(u16 vendor, u16 device) {
    return (device << 16) | vendor;
}
static inline u16 pci_vd_to_ven(u32 vd) {
    return vd & 0xffff;
}
static inline u16 pci_vd_to_dev(u32 vd) {
    return vd >> 16;
}








static inline void writel(void *addr, u32 val) {
    *(volatile u32 *)addr = val;
}
static inline void writew(void *addr, u16 val) {
    *(volatile u16 *)addr = val;
}
static inline void writeb(void *addr, u8 val) {
    *(volatile u8 *)addr = val;
}
static inline u32 readl(const void *addr) {
    return *(volatile const u32 *)addr;
}
static inline u16 readw(const void *addr) {
    return *(volatile const u16 *)addr;
}
static inline u8 readb(const void *addr) {
    return *(volatile const u8 *)addr;
}


static inline u32 __ffs(u32 word)
{
    asm("bsf %1,%0"
        : "=r" (word)
        : "rm" (word));
    return word;
}
static inline u32 __fls(u32 word)
{
    asm("bsr %1,%0"
        : "=r" (word)
        : "rm" (word));
    return word;
}


#define ALIGN(x,a)              __ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN_DOWN(x,a)         ((x) & ~((typeof(x))(a)-1))

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define DIV_ROUND_CLOSEST(x, divisor)({                 \
            typeof(divisor) __divisor = divisor;        \
            (((x) + ((__divisor) / 2)) / (__divisor));  \
        })


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})



#define barrier() __asm__ __volatile__("": : :"memory")


// ps2port.h

// Keyboard commands
#define ATKBD_CMD_SETLEDS       0x10ed
#define ATKBD_CMD_SSCANSET      0x10f0
#define ATKBD_CMD_GETID         0x02f2
#define ATKBD_CMD_ENABLE        0x00f4
#define ATKBD_CMD_RESET_DIS     0x00f5
#define ATKBD_CMD_RESET_BAT     0x02ff

// Mouse commands
#define PSMOUSE_CMD_SETSCALE11  0x00e6
#define PSMOUSE_CMD_SETSCALE21  0x00e7
#define PSMOUSE_CMD_SETRES      0x10e8
#define PSMOUSE_CMD_GETINFO     0x03e9
#define PSMOUSE_CMD_GETID       0x02f2
#define PSMOUSE_CMD_SETRATE     0x10f3
#define PSMOUSE_CMD_ENABLE      0x00f4
#define PSMOUSE_CMD_DISABLE     0x00f5
#define PSMOUSE_CMD_RESET_BAT   0x02ff

struct usbkeyinfo {
    union {
        struct {
            u8 modifiers;
            u8 repeatcount;
            u8 keys[6];
        };
        u64 data;
    };
};









struct extended_bios_data_area_s {
    u8 size;
    //u8 reserved1[0x21];
    //struct segoff_s far_call_pointer;
    u8 mouse_flag1;
    u8 mouse_flag2;
    u8 mouse_data[0x08];
    // 0x30
    //u8 other1[0x0d];

    // 0x3d
    //struct fdpt_s fdpt[2];

    // 0x5d
    //u8 other2[0xC4];

    // 0x121 - Begin custom storage.
    u8 ps2ctr;
    struct usbkeyinfo usbkey_last;

    int RTCusers;

    // El Torito Emulation data
    //struct cdemu_s cdemu;

    // Buffer for disk DPTE table
    //struct dpte_s dpte;

    // Locks for removable devices
    //u8 cdrom_locks[CONFIG_MAX_EXTDRIVE];

    //u16 boot_sequence;

    // Stack space available for code that needs it.
    //u8 extra_stack[512] __aligned(8);
} PACKED;




extern struct extended_bios_data_area_s usb_ebda2;

#define GET_EBDA2(eseg, var) (usb_ebda2.var)
#define SET_EBDA2(eseg, ___var, ___val) ((usb_ebda2.___var) = ___val)


#define GET_GLOBAL(var) (var)

#define SET_GLOBAL(var, val) do {               \
        ASSERT32FLAT();                         \
        (var) = (val);                          \
    } while (0)














#define PCI_CLASS_SERIAL_USB		0x0c03
#define PCI_CLASS_SERIAL_USB_UHCI	0x0c0300
#define PCI_CLASS_SERIAL_USB_OHCI	0x0c0310
#define PCI_CLASS_SERIAL_USB_EHCI	0x0c0320



int pci_next(int bdf, int *pmax);
#define foreachpci(BDF, MAX)                    \
    for (MAX=0x0100, BDF=pci_next(0, &MAX)      \
         ; BDF >= 0                             \
         ; BDF=pci_next(BDF+1, &MAX))






#define DISK_SECTOR_SIZE  512
#define CDROM_SECTOR_SIZE 2048


// Remove any trailing blank characters (spaces, new lines, carriage returns)
static inline void
nullTrailingSpace(char *buf)
{
    int len = strlen(buf);
    char *end = &buf[len-1];
    while (end >= buf && *end <= ' ')
        *(end--) = '\0';
}

// Copy a string - truncating it if necessary.
static inline char *
strtcpy(char *dest, const char *src, size_t len)
{
    char *d = dest;
    while (--len && *src != '\0')
        *d++ = *src++;
    *d = '\0';
    return dest;
}


#define MAXDESCSIZE 80

#define DISK_RET_SUCCESS       0x00
#define DISK_RET_EPARAM        0x01
#define DISK_RET_EADDRNOTFOUND 0x02
#define DISK_RET_EWRITEPROTECT 0x03
#define DISK_RET_ECHANGED      0x06
#define DISK_RET_EBOUNDARY     0x09
#define DISK_RET_EBADTRACK     0x0c
#define DISK_RET_ECONTROLLER   0x20
#define DISK_RET_ETIMEOUT      0x80
#define DISK_RET_ENOTLOCKED    0xb0
#define DISK_RET_ELOCKED       0xb1
#define DISK_RET_ENOTREMOVABLE 0xb2
#define DISK_RET_ETOOMANYLOCKS 0xb4
#define DISK_RET_EMEDIA        0xC0
#define DISK_RET_ENOTREADY     0xAA

// TODO killme
#define CMD_RESET   0x00
#define CMD_READ    0x02
#define CMD_WRITE   0x03
#define CMD_VERIFY  0x04
#define CMD_FORMAT  0x05
#define CMD_SEEK    0x07
#define CMD_ISREADY 0x10


struct drive_s {
    u8 type;            // Driver type (DTYPE_*)
    u8 floppy_type;     // Type of floppy (only for floppy drives).
    //struct chs_s lchs;  // Logical CHS
    u64 sectors;        // Total sectors count
    u32 cntl_id;        // Unique id for a given driver type.
    u8 removable;       // Is media removable (currently unused)

    // Info for EDD calls
    u8 translation;     // type of translation
    u16 blksize;        // block size
    //struct chs_s pchs;  // Physical CHS
};

struct disk_op_s {
    u64 lba;
    void *buf_fl;
    struct drive_s *drive_g;
    u16 count;
    u8 command;
};

struct cdbres_read_capacity {
    u32 sectors;
    u32 blksize;
} PACKED;

struct cdbres_inquiry {
    u8 pdt;
    u8 removable;
    u8 reserved_02[2];
    u8 additional;
    u8 reserved_05[3];
    char vendor[8];
    char product[16];
    char rev[4];
} PACKED;


// TODO killme
#define DTYPE_NONE     0x00
#define DTYPE_FLOPPY   0x01
#define DTYPE_ATA      0x02
#define DTYPE_ATAPI    0x03
#define DTYPE_RAMDISK  0x04
#define DTYPE_CDEMU    0x05
#define DTYPE_USB      0x06
#define DTYPE_VIRTIO   0x07
#define DTYPE_AHCI     0x08


#endif // COMPAT_SEABIOS_H
