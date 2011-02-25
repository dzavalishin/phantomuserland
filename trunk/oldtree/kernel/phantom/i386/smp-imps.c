/*
 *  <Insert copyright here : it must be BSD-like so anyone can use it>
 *
 *  Author:  Erich Boleyn  <erich@uruk.org>   http://www.uruk.org/~erich/
 *
 *  Source file implementing Intel MultiProcessor Specification (MPS)
 *  version 1.1 and 1.4 SMP hardware control for Intel Architecture CPUs,
 *  with hooks for running correctly on a standard PC without the hardware.
 *
 *  This file was created from information in the Intel MPS version 1.4
 *  document, order number 242016-004, which can be ordered from the
 *  Intel literature center.
 *
 *  General limitations of this code:
 *
 *   (1) : This code has never been tested on an MPS-compatible system with
 *           486 CPUs, but is expected to work.
 *   (2) : Presumes "int", "long", and "unsigned" are 32 bits in size, and
 *	     that 32-bit pointers and memory addressing is used uniformly.
 */

#define _SMP_IMPS_C

#define DEBUG_MSG_PREFIX "smp"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <i386/trap.h>

#include <i386/proc_reg.h>
#include <i386/seg.h>
#include <i386/isa/pic.h>
#include <x86/phantom_pmap.h>

#include <phantom_types.h>
#include <phantom_libc.h>
#include <kernel/vm.h>
#include <kernel/smp.h>
#include <kernel/init.h>

#include <time.h>
#include <errno.h>

#include <kernel/ia32/rtc.h>



static volatile int     smp_ap_booted = 0;

#if 1
#define CMOS_WRITE_BYTE(x,y)	rtcout(x,y) /* write unsigned char "y" at CMOS loc "x" */
#define CMOS_READ_BYTE(x)	rtcin(x) /* read unsigned char at CMOS loc "x" */

//#define PHYS_TO_VIRTUAL(x)	phystokv(x)	/* convert physical address "x" to virtual */
//#define VIRTUAL_TO_PHYS(x)	kvtophys(x)     /* convert virtual address "x" to physical */

#define PHYS_TO_VIRTUAL(x)	((void*)(x))	/* convert physical address "x" to virtual */
#define VIRTUAL_TO_PHYS(x)	kvtophys(x)     /* convert virtual address "x" to physical */

#define TEST_BOOTED(x)		smp_ap_booted	/* test bootaddr x to see if CPU started */
#define READ_MSR_LO(x)		( (u_int32_t)rdmsr(x) )	/* Read MSR low function */
#endif


/*
 *  Includes here
 */

#define IMPS_DEBUG 1

#include <kernel/ia32/apic.h>
#include "smp-imps.h"


/*
 *  Defines that are here so as not to be in the global header file.
 */
#define EBDA_SEG_ADDR			0x40E
#define BIOS_RESET_VECTOR		0x467
#define LAPIC_ADDR_DEFAULT		0xFEE00000uL
#define IOAPIC_ADDR_DEFAULT		0xFEC00000uL
#define CMOS_RESET_CODE			0xF
#define		CMOS_RESET_JUMP		0xa
#define CMOS_BASE_MEMORY		0x15


/*
 *  Static defines here for SMP use.
 */

#define DEF_ENTRIES	23

static int lapic_dummy = 0;
static struct {
    imps_processor proc[2];
    imps_bus bus[2];
    imps_ioapic ioapic;
    imps_interrupt intin[16];
    imps_interrupt lintin[2];
} defconfig = {
    {
        { IMPS_BCT_PROCESSOR, 0, 0, 0, 0, 0, "" },
        { IMPS_BCT_PROCESSOR, 1, 0, 0, 0, 0, "" }
    },

    {
        { IMPS_BCT_BUS, 0, {'E', 'I', 'S', 'A', ' ', ' '}},
        { 255, 1, {'P', 'C', 'I', ' ', ' ', ' '}}
    },

    { IMPS_BCT_IOAPIC, 0, 0, IMPS_FLAG_ENABLED, IOAPIC_ADDR_DEFAULT },

    {
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_EXTINT, 0, 0, 0, 0xFF, 0},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 1, 0xFF, 1},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 0, 0xFF, 2},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 3, 0xFF, 3},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 4, 0xFF, 4},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 5, 0xFF, 5},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 6, 0xFF, 6},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 7, 0xFF, 7},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 8, 0xFF, 8},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 9, 0xFF, 9},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 10, 0xFF, 10},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 11, 0xFF, 11},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 12, 0xFF, 12},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 13, 0xFF, 13},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 14, 0xFF, 14},
        { IMPS_BCT_IO_INTERRUPT, IMPS_INT_INT, 0, 0, 15, 0xFF, 15}
    },
    {
        { IMPS_BCT_LOCAL_INTERRUPT, IMPS_INT_EXTINT, 0, 0, 15, 0xFF, 0},
        { IMPS_BCT_LOCAL_INTERRUPT, IMPS_INT_NMI, 0, 0, 15, 0xFF, 1}
    }
};

/*
 *  Exported globals here.
 */

//volatile int imps_release_cpus = 0;
static int imps_enabled = 0;
static int imps_num_cpus = 1;
unsigned imps_lapic_addr = ((unsigned)(&lapic_dummy)) - LAPIC_ID;
unsigned char imps_cpu_apic_map[IMPS_MAX_CPUS];
unsigned char imps_apic_cpu_map[IMPS_MAX_CPUS];


/*
 *  MPS checksum function
 *
 *  Function finished.
 */

static int get_checksum(unsigned start, int length)
{
    unsigned sum = 0;

    while (length-- > 0) {
        sum += *((unsigned char *) (start++));
    }

    return (sum&0xFF);
}


/*
 *  APIC ICR write and status check function.
 */

static errno_t
send_ipi(unsigned int dst, unsigned int v)
{
    int to, send_status;

    IMPS_LAPIC_WRITE(LAPIC_ICR+0x10, (dst << 24));
    IMPS_LAPIC_WRITE(LAPIC_ICR, v);

    /* Wait for send to finish */
    to = 0;
    do {
        // TODO return to uSec spinwait
        //UDELAY(100);
        phantom_spinwait(1);

        send_status = IMPS_LAPIC_READ(LAPIC_ICR) & LAPIC_ICR_STATUS_PEND;
    } while (send_status && (to++ < 1000));

    return (to < 1000) ? ETIMEDOUT : 0;
}


void dump_mp_gdt(void *addr)
{
    //extern u_int 	MP_GDT;
    struct real_descriptor *rd = addr;

    int i;

    for( i = 0; i < 5; i++, rd++ )
    {
        printf("%d: base %x lim %x, acc %x gran %x\n",
               i,
               rd->base_low + (rd->base_med<<16) + (rd->base_high<<24),
               rd->limit_low + (rd->limit_high<<16),
               rd->access, rd->granularity
              );
    }
}



static unsigned bootaddr;
static void *   ap_stack;


/*
 *  Primary function for booting individual CPUs.
 *
 *  This must be modified to perform whatever OS-specific initialization
 *  that is required.
 */

static int
boot_cpu(imps_processor *proc)
{
    int apicid = proc->apic_id, success = 1, to;
    unsigned accept_status;
    unsigned bios_reset_vector = (int)PHYS_TO_VIRTUAL(BIOS_RESET_VECTOR);

    int ver = IMPS_LAPIC_READ(LAPIC_VER);
    SHOW_FLOW( 0, "APIC ver = 0x%x (%d)", ver, APIC_VERSION(ver) );

    // TODO define size? guard page?
    ap_stack = calloc(1, 64*1024);

    /*
     * Copy boot code for secondary CPUs here.  Find it in between
     * "patch_code_start" and "patch_code_end" symbols.  The other CPUs
     * will start there in 16-bit real mode under the 1MB boundary.
     * "patch_code_start" should be placed at a 4K-aligned address
     * under the 1MB boundary.
     */

    //return 0;

    //panic("boot SMP cpu code is not ready");

    //extern char patch_code_start[];
    //extern char patch_code_end[];
    //bootaddr = (512-64)*1024;
    //memcpy((char *)bootaddr, patch_code_start, patch_code_end - patch_code_start);

    install_ap_tramp((void *)bootaddr);

    smp_ap_booted = 0;

    //dump_mp_gdt((void *)&MP_GDT);

    /*
     *  Generic CPU startup sequence starts here.
     */

    /* set BIOS reset vector */
    CMOS_WRITE_BYTE(CMOS_RESET_CODE, CMOS_RESET_JUMP);
    //*((volatile unsigned *) bios_reset_vector) = ((bootaddr & 0xFF000) << 12);
    //*((volatile unsigned *) bios_reset_vector) = ((bootaddr & 0xFF000) << 12);

    *((volatile unsigned short *) 0x469) = bootaddr >> 4;
    *((volatile unsigned short *) 0x467) = bootaddr & 0xf;


    /* clear the APIC error register */
    IMPS_LAPIC_WRITE(LAPIC_ESR, 0);
    accept_status = IMPS_LAPIC_READ(LAPIC_ESR);

    /* assert INIT IPI */
    send_ipi(apicid, LAPIC_ICR_TM_LEVEL | LAPIC_ICR_LEVELASSERT | LAPIC_ICR_DM_INIT);

    //UDELAY(10000);
    phantom_spinwait(10);


    /* de-assert INIT IPI */
    send_ipi(apicid, LAPIC_ICR_TM_LEVEL | LAPIC_ICR_DM_INIT);

    phantom_spinwait(10);
    //UDELAY(10000);

#if 1
    /*
     *  Send Startup IPIs if not an old pre-integrated APIC.
     */

    if (proc->apic_ver >= APIC_VER_NEW) {
        int i;
        for (i = 1; i <= 2; i++) {
            send_ipi(apicid, LAPIC_ICR_DM_SIPI | ((bootaddr >> 12) & 0xFF));
            //UDELAY(1000);
            phantom_spinwait(1);
        }
    }
#endif

    SHOW_FLOW0( 0, "Wait for CPU to start" );


    /*
     *  Check to see if other processor has started.
     */
#define WAIT_4_START 1000
    to = 0;
    while (!TEST_BOOTED(bootaddr) && to++ < WAIT_4_START)
    {
        //UDELAY(10000);
        phantom_spinwait(10);
        phantom_spinwait(1000);
    }
    if (to >= WAIT_4_START) {
        SHOW_INFO0( 0, "CPU Not Responding, DISABLED" );
        success = 0;
    } else {
        SHOW_INFO( 0, "#%d  Application Processor (AP)", imps_num_cpus);
    }



    /*
     *  Generic CPU startup sequence ends here, the rest is cleanup.
     */

    /* clear the APIC error register */
    IMPS_LAPIC_WRITE(LAPIC_ESR, 0);
    accept_status = IMPS_LAPIC_READ(LAPIC_ESR);

    /* clean up BIOS reset vector */
    CMOS_WRITE_BYTE(CMOS_RESET_CODE, 0);
    *((volatile unsigned *) bios_reset_vector) = 0;

    smp_ap_booted = 0;

    //dump_mp_gdt();

    return success;
}


/*
 *  read bios stuff and fill tables
 */

static void
add_processor(imps_processor *proc)
{
    int apicid = proc->apic_id;

    SHOW_INFO( 0, "  Starting processor [APIC id %d ver %d]:  ",
                  apicid, proc->apic_ver);
    if (!(proc->flags & IMPS_FLAG_ENABLED)) {
        SHOW_INFO0( 0, "  - DISABLED");
        return;
    }
    if (proc->flags & (IMPS_CPUFLAG_BOOT)) {
        SHOW_INFO0( 0, "  - #0  BootStrap Processor (BSP)");
        return;
    }
    if (boot_cpu(proc)) {

        imps_cpu_apic_map[imps_num_cpus] = apicid;
        imps_apic_cpu_map[apicid] = imps_num_cpus;
        imps_num_cpus++;

        SHOW_INFO( 0, "  Processor [APIC id %d] reports OK", apicid);
    }
}

#define MAXBUSIDS 10
static char *bus_ids[MAXBUSIDS];

static void
add_bus(imps_bus *bus)
{
    char str[8];

    memcpy(str, bus->bus_type, 6);
    str[6] = 0;
    SHOW_INFO( 0, "  Bus id %d is %s\n", bus->id, str);


    if(bus->id < MAXBUSIDS)
        bus_ids[bus->id] = strdup(str);

    /*  XXXXX  add OS-specific code here */
}

static void
add_ioapic(imps_ioapic *ioapic)
{
    SHOW_INFO( 0, "  I/O APIC id %d ver %d, address: 0x%x  ",
                  ioapic->id, ioapic->ver, ioapic->addr);
    if (!(ioapic->flags & IMPS_FLAG_ENABLED)) {
        SHOW_INFO0( 0, "  - DISABLED");
        return;
    }

    // TODO init IO APIC from here passing address and id. later work with io apic by id
    setIoApicId( ioapic->id );
}

static void
add_int(imps_interrupt *ii)
{
    char * itname = "?";
    char *busname = "? bus ";

    if(ii->source_bus_id < MAXBUSIDS)
        busname = bus_ids[ii->source_bus_id];

    if( 0 == busname ) busname = "?? bus";

    int in_level = IOAPIC_EDGE_TRIGGERED;
    int in_lowhi = IOAPIC_LO_ACTIVE;

    if( 0 == strcmp( "ISA", busname ) )
    {
        in_level = IOAPIC_EDGE_TRIGGERED;
        in_lowhi = IOAPIC_LO_ACTIVE;
    }

    if( 0 == strcmp( "PCI", busname ) )
    {
        in_level = IOAPIC_LEVEL_TRIGGERED;
        in_lowhi = IOAPIC_LO_ACTIVE; // TODO check
    }

    switch(ii->type)
    {
    case IMPS_BCT_IO_INTERRUPT:
        itname = "io";
        // TODO will fail if more than one IO APIC - need to pass io apic id here to select one
        setIoApicInput( ii->dest_apic_intin, ii->source_bus_irq, in_level, in_lowhi );
        break;

    case IMPS_BCT_LOCAL_INTERRUPT:
        itname = "local";
        break;
    }


    SHOW_INFO( 0,  "Interrupt type %d (%s), int_type %d, flags %d",
           ii->type, itname, ii->int_type, ii->flags
          );

    SHOW_INFO( 0,  " - %s (%d) irq %2X -> apic %d in %2X",
               busname,
               ii->source_bus_id, ii->source_bus_irq,
               ii->dest_apic_id, ii->dest_apic_intin
             );


}


static void
imps_read_config_table(unsigned start, int count)
{
    while (count-- > 0) {
        switch (*((unsigned char *)start)) {
        case IMPS_BCT_PROCESSOR:
            add_processor((imps_processor *)start);
            start += 12;	/* 20 total */
            break;
        case IMPS_BCT_BUS:
            add_bus((imps_bus *)start);
            break;
        case IMPS_BCT_IOAPIC:
            add_ioapic((imps_ioapic *)start);
            break;
        case IMPS_BCT_IO_INTERRUPT:
            add_int((imps_interrupt *)start);
            break;
        case IMPS_BCT_LOCAL_INTERRUPT:
            add_int((imps_interrupt *)start);
            break;

        default:
            break;
        }
        start += 8;
    }
}


static int
imps_bad_bios(imps_fps *fps_ptr)
{
    int sum;
    imps_cth *local_cth_ptr
        = (imps_cth *) PHYS_TO_VIRTUAL(fps_ptr->cth_ptr);

    if (fps_ptr->feature_info[0] > IMPS_FPS_DEFAULT_MAX) {
        SHOW_ERROR( 0, "    Invalid MP System Configuration type %d",
                      fps_ptr->feature_info[0]);
        return 1;
    }

    if (fps_ptr->cth_ptr) {
        sum = get_checksum((unsigned)local_cth_ptr,
                           local_cth_ptr->base_length);
        if (local_cth_ptr->sig != IMPS_CTH_SIGNATURE || sum) {
            SHOW_ERROR( 0, "    Bad MP Config Table sig 0x%x and/or checksum 0x%x", (unsigned)(fps_ptr->cth_ptr), sum);
            return 1;
        }
        if (local_cth_ptr->spec_rev != fps_ptr->spec_rev) {
            SHOW_ERROR( 0, "    Bad MP Config Table sub-revision # %d", local_cth_ptr->spec_rev);
            return 1;
        }
        if (local_cth_ptr->extended_length) {
            sum = (get_checksum(((unsigned)local_cth_ptr)
                                + local_cth_ptr->base_length,
                                local_cth_ptr->extended_length)
                   + local_cth_ptr->extended_checksum) & 0xFF;
            if (sum) {
                SHOW_ERROR( 0, "    Bad Extended MP Config Table checksum 0x%x", sum);
                return 1;
            }
        }
    } else if (!fps_ptr->feature_info[0]) {
        SHOW_ERROR0( 0, "    Missing configuration information");
        return 1;
    }

    return 0;
}


static void
imps_read_bios(imps_fps *fps_ptr)
{
    int apicid;
    unsigned cth_start, cth_count;
    imps_cth *local_cth_ptr
        = (imps_cth *)PHYS_TO_VIRTUAL(fps_ptr->cth_ptr);
    char *str_ptr;

    SHOW_INFO( 0, "Intel MultiProcessor Spec 1.%d BIOS support detected",
                  fps_ptr->spec_rev);

    /*
     *  Do all checking of errors which would definitely
     *  lead to failure of the SMP boot here.
     */

    if (imps_bad_bios(fps_ptr)) {
        SHOW_ERROR0( 0, "    Disabling MPS support");
        return;
    }

    if (fps_ptr->feature_info[1] & IMPS_FPS_IMCRP_BIT) {
        str_ptr = "IMCR and PIC";
    } else {
        str_ptr = "Virtual Wire";
    }
    if (fps_ptr->cth_ptr) {
        imps_lapic_addr = local_cth_ptr->lapic_addr;
    } else {
        imps_lapic_addr = LAPIC_ADDR_DEFAULT;
    }
    SHOW_INFO( 0, "    APIC config: \"%s mode\"    Local APIC address: 0x%x",
                  str_ptr, imps_lapic_addr);
    if (imps_lapic_addr != (READ_MSR_LO(0x1b) & 0xFFFFF000)) {
        SHOW_ERROR0( 0, "Inconsistent Local APIC address, Disabling SMP support");
        return;
    }
    imps_lapic_addr = (int)PHYS_TO_VIRTUAL(imps_lapic_addr);

    /*
     *  Setup primary CPU.
     */
    apicid = IMPS_LAPIC_READ(LAPIC_SPIV);
    IMPS_LAPIC_WRITE(LAPIC_SPIV, apicid|LAPIC_SPIV_ENABLE_APIC);
    apicid = APIC_ID(IMPS_LAPIC_READ(LAPIC_ID));
    imps_cpu_apic_map[0] = apicid;
    imps_apic_cpu_map[apicid] = 0;

    if (fps_ptr->cth_ptr) {
        char str1[16], str2[16];
        memcpy(str1, local_cth_ptr->oem_id, 8);
        str1[8] = 0;
        memcpy(str2, local_cth_ptr->prod_id, 12);
        str2[12] = 0;
        SHOW_INFO( 0, "  OEM id: %s  Product id: %s", str1, str2);
        cth_start = ((unsigned) local_cth_ptr) + sizeof(imps_cth);
        cth_count = local_cth_ptr->entry_count;
    } else {
        *((volatile unsigned *) IOAPIC_ADDR_DEFAULT) =  IOAPIC_ID;
        defconfig.ioapic.id
            = APIC_ID(*((volatile unsigned *)
                        (IOAPIC_ADDR_DEFAULT+IOAPIC_RW)));
        *((volatile unsigned *) IOAPIC_ADDR_DEFAULT) =  IOAPIC_VER;
        defconfig.ioapic.ver
            = APIC_VERSION(*((volatile unsigned *)
                             (IOAPIC_ADDR_DEFAULT+IOAPIC_RW)));
        defconfig.proc[apicid].flags
            = IMPS_FLAG_ENABLED|IMPS_CPUFLAG_BOOT;
        defconfig.proc[!apicid].flags = IMPS_FLAG_ENABLED;
        imps_num_cpus = 2;
        if (fps_ptr->feature_info[0] == 1
            || fps_ptr->feature_info[0] == 5) {
            memcpy(defconfig.bus[0].bus_type, "ISA   ", 6);
        }
        if (fps_ptr->feature_info[0] == 4
            || fps_ptr->feature_info[0] == 7) {
            memcpy(defconfig.bus[0].bus_type, "MCA   ", 6);
        }
        if (fps_ptr->feature_info[0] > 4) {
            defconfig.proc[0].apic_ver = 0x10;
            defconfig.proc[1].apic_ver = 0x10;
            defconfig.bus[1].type = IMPS_BCT_BUS;
        }
        if (fps_ptr->feature_info[0] == 2) {
            defconfig.intin[2].type = 255;
            defconfig.intin[13].type = 255;
        }
        if (fps_ptr->feature_info[0] == 7) {
            defconfig.intin[0].type = 255;
        }
        cth_start = (unsigned) &defconfig;
        cth_count = DEF_ENTRIES;
    }
    imps_read_config_table(cth_start, cth_count);

    /* %%%%% ESB read extended entries here */

    imps_enabled = 1;
}


/*
 *  Given a region to check, this actually looks for the "MP Floating
 *  Pointer Structure".  The return value indicates if the correct
 *  signature and checksum for a floating pointer structure of the
 *  appropriate spec revision was found.  If so, then do not search
 *  further.
 *
 *  NOTE:  The memory scan will always be in the bottom 1 MB.
 *
 *  This function presumes that "start" will always be aligned to a 16-bit
 *  boundary.
 *
 *  Function finished.
 */

static int
imps_scan(unsigned start, unsigned length)
{
    SHOW_FLOW( 2, "Scanning from 0x%x for %d bytes", start, length );

    while (length > 0) {
        imps_fps *fps_ptr = (imps_fps *) PHYS_TO_VIRTUAL(start);

        if (fps_ptr->sig == IMPS_FPS_SIGNATURE
            && fps_ptr->length == 1
            && (fps_ptr->spec_rev == 1 || fps_ptr->spec_rev == 4)
            && !get_checksum(start, 16)) {

            SHOW_FLOW( 2, "Found MP Floating Structure Pointer at %x", start);

            imps_read_bios(fps_ptr);
            return 1;
        }

        length -= 16;
        start += 16;
    }

    return 0;
}

#if 0
/*
 *  This is the primary function to "force" SMP support, with
 *  the assumption that you have consecutively numbered APIC ids.
 */
int
imps_force(int ncpus)
{
    int apicid, i;
    imps_processor p;

    printf("Intel MultiProcessor \"Force\" Support\n");

    imps_lapic_addr = (READ_MSR_LO(0x1b) & 0xFFFFF000);
    imps_lapic_addr = (int)PHYS_TO_VIRTUAL(imps_lapic_addr);

    /*
     *  Setup primary CPU.
     */
    apicid = IMPS_LAPIC_READ(LAPIC_SPIV);
    IMPS_LAPIC_WRITE(LAPIC_SPIV, apicid|LAPIC_SPIV_ENABLE_APIC);
    apicid = APIC_ID(IMPS_LAPIC_READ(LAPIC_ID));
    imps_cpu_apic_map[0] = apicid;
    imps_apic_cpu_map[apicid] = 0;

    p.type = 0;
    p.apic_ver = 0x10;
    p.signature = p.features = 0;

    for (i = 0; i < ncpus; i++) {
        if (apicid == i) {
            p.flags = IMPS_FLAG_ENABLED | IMPS_CPUFLAG_BOOT;
        } else {
            p.flags = IMPS_FLAG_ENABLED;
        }
        p.apic_id = i;
        add_processor(&p);
    }

    return imps_num_cpus;
}

#endif


/*
 *  This is the primary function for probing for MPS compatible hardware
 *  and BIOS information.  Call this during the early stages of OS startup,
 *  before memory can be messed up.
 *
 *  The probe looks for the "MP Floating Pointer Structure" at locations
 *  listed at the top of page 4-2 of the spec.
 *
 *  Environment requirements from the OS to run:
 *
 *   (1) : A non-linear virtual to physical memory mapping is probably OK,
 *	     as (I think) the structures all fall within page boundaries,
 *	     but a linear mapping is recommended.  Currently assumes that
 *	     the mapping will remain identical over time (which should be
 *	     OK since it only accesses memory which shouldn't be munged
 *	     by the OS anyway).
 *   (2) : The OS only consumes memory which the BIOS says is OK to use,
 *	     and not any of the BIOS standard areas (the areas 0x400 to
 *	     0x600, the EBDA, 0xE0000 to 0xFFFFF, and unreported physical
 *	     RAM).  Sometimes a small amount of physical RAM is not
 *	     reported by the BIOS, to be used to store MPS and other
 *	     information.
 *   (3) : It must be possible to read the CMOS.
 *   (4) : There must be between 512K and 640K of lower memory (this is a
 *	     sanity check).
 *
 *  Function finished.
 */

int
imps_probe(void)
{
    assert( 0 == hal_alloc_phys_pages_low( &bootaddr, 4 ) );

    int picmask = phantom_pic_get_irqmask();
    phantom_pic_disable_all();


    /*
     *  Determine possible address of the EBDA
     */
    unsigned ebda_addr = *((unsigned short *)
                           PHYS_TO_VIRTUAL(EBDA_SEG_ADDR)) << 4;

    /*
     *  Determine amount of installed lower memory (not *available*
     *  lower memory).
     *
     *  NOTE:  This should work reliably as long as we verify the
     *         machine is at least a system that could possibly have
     *         MPS compatibility to begin with.
     */
    unsigned mem_lower = ((CMOS_READ_BYTE(CMOS_BASE_MEMORY+1) << 8)
                          | CMOS_READ_BYTE(CMOS_BASE_MEMORY))       << 10;

#ifdef IMPS_DEBUG
    imps_enabled = 0;
    imps_num_cpus = 1;
#endif

    /*
     *  Sanity check : if this isn't reasonable, it is almost impossibly
     *    unlikely to be an MPS compatible machine, so return failure.
     */
    if (mem_lower < 512*1024 || mem_lower > 640*1024) {
        //return 0;
        goto free;
    }

    if (ebda_addr > mem_lower - 1024
        || ebda_addr + *((unsigned char *) PHYS_TO_VIRTUAL(ebda_addr))
        * 1024 > mem_lower) {
        ebda_addr = 0;
    }

    int ret = 0;

    if (((ebda_addr && imps_scan(ebda_addr, 1024))
         || (!ebda_addr && imps_scan(mem_lower - 1024, 1024))
         || imps_scan(0xF0000, 0x10000)) && imps_enabled) {
        ret = imps_num_cpus;
        goto free;
    }

    /*
     *  If no BIOS info on MPS hardware is found, then return failure.
     */
free:
    hal_free_phys_pages_low( bootaddr, 4 );
    phantom_pic_set_irqmask(picmask);

    return ret;
}


// -----------------------------------------------------------------------
//
// SMP - secondary (non-boot) CPU startup.
// See mboot.S for low level code.
//
// Be VERY careful here as things are quite fragile before smp_ap_booted = 1;
//
// We come to C code with special GDT, no IDT, no LDT, no TSS. We CAN NOT
// use things like malloc() and everything else with mutex/cond/sem code
// inside.
//
// -----------------------------------------------------------------------







static void do_smp_ap_start(void);


// We get here from trampoline running on the next Application Processor
// Note that we're on the TEMP stack, having WRONG GDT, NO LDT/IDT, ints
// disabled and minimal possible CPU (CR0, etc) state
void smp_ap_start(void)
{
    static const char *msg = "AP idle thread unwind";
    __asm __volatile("movl %0, %%esp" : "=rm" (ap_stack));
    __asm __volatile("pushl %0" : : "r" (msg));
    __asm __volatile("pushl %0" : : "r" (panic));
    __asm __volatile("pushl %0" : : "r" (panic));

    // For stack trace to finish here
    __asm __volatile("movl $0, %%ebp" : );

    // So that do_smp_ap_start will have good stack right from start
    do_smp_ap_start();
    panic(msg);
}

#define REAL_SMP 0


static void do_smp_ap_start(void)
{

    phantom_load_gdt();
    phantom_paging_start(); // NB! Can't call GET_CPU_ID() before this line for APIC page mapping is not available

    relocate_apic();
    int ncpu = GET_CPU_ID();

    phantom_load_cpu_tss(ncpu);

    phantom_import_cpu_thread(ncpu);
//halt();
    // We can report boot only after phantom_import_cpu_thread,
    // cause it gives us new stack
    smp_ap_booted = 1;

    phantom_load_idt(); // We can do this here as nothing relies on IDT before

    phantom_setup_apic(); // now setup my own apic

    hal_sti();

    printf( "!! SMP AP %d START !!\n", ncpu );

    // Todo hack. need some good way to tell them they can go.
    while (trap_panic == phantom_trap_handlers[T_PAGE_FAULT])
        ia32_pause();

    while(1)
    {
#if REAL_SMP
        //hal_sleep_msec(10);
        ia32_pause(); // tell CPU we have nothing to do - must be halt here
        phantom_scheduler_yield();
#else
        halt();
#endif
    }

    halt();
}



int is_smp(void)
{
    return imps_num_cpus > 1;
}

int ncpus(void)
{
    return imps_num_cpus;
}





