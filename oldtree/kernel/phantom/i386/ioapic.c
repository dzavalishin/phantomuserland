/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * IO APIC support
 *
**/

#define DEBUG_MSG_PREFIX "ioapic"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include "../misc.h"
#include <kernel/ia32/cpu.h>
#include <kernel/ia32/apic.h>
#include <kernel/ia32/apic_regs.h>
#include <ia32/phantom_pmap.h>

#include <hal.h>
#include <phantom_libc.h>

static volatile u_int32_t* ioapic;
static int max_redirect_entry = 23; // even first chips had 24 inputs
int have_io_apic = 0;




void phantom_io_apic_init( physaddr_t pa )
{
    printf("Activating IO APIC...");

    int rc = hal_alloc_vaddress( (void **)&ioapic, 1);
    if(rc) panic("Can't alloc IO APIC vaddr");

    int pte = create_pte(pa,
                         INTEL_PTE_VALID|INTEL_PTE_WRITE|
                         INTEL_PTE_WTHRU|INTEL_PTE_NCACHE
                        );
    phantom_map_page( (linaddr_t)ioapic, pte );

    int id = readIoApic(IOAPIC_ID);
    int ver = readIoApic(IOAPIC_VER);
    //int arb; // = readIoApic(IOAPIC_ARB);

    int max_redirect_entry = (ver >> 16) & 0xFF;

    printf("IO APIC ver. 0x%X, unit id 0x%X, %d inputs\n",
           0xFF && ver, id, max_redirect_entry+1
          );

    have_io_apic = 1;
#if 0
    int i;

    // Set them as is - this is wrong, we need to
    // use map from MPS BIOS or ACPI. See smp-imps.c
    for( i = 0; i < 16; i++ )
    {
        // ISA
        setIoApicInput( i, i, IOAPIC_EDGE_TRIGGERED, IOAPIC_LO_ACTIVE );
    }

    dumpIoApicInputs();


    // hardcode io apic id
    writeIoApic(IOAPIC_ID, 4);

    arb = readIoApic(IOAPIC_ARB);
    printf("IO APIC arbitration prio %d\n", arb );
#endif
}


u_int32_t readIoApic(u_int32_t reg)
{
   ioapic[0] = reg;
   return ioapic[4];
}
 
void writeIoApic( u_int32_t reg, u_int32_t value)
{
   ioapic[0] = reg;
   ioapic[4] = value;
}

void setIoApicId( int id )
{
    writeIoApic(IOAPIC_ID, id);
}

void setIoApicInput( int input, int vector, int level, int low_active )
{
    if( input > max_redirect_entry )
    {
        SHOW_ERROR( 0, "Attempt to set IO APIC input %d, > max %d", input, max_redirect_entry );
        return;
    }

    int h = readIoApic(IOAPIC_REDIR + input*2);
    int l = readIoApic(IOAPIC_REDIR + input*2 +1);

    h &= 0x00FFFFFF;
    l &= 0xFFFFFF00;


    l &= ~IOAPIC_REDIR_LEVEL;
    l |= level ? IOAPIC_REDIR_LEVEL : 0;

    l &= ~IOAPIC_REDIR_LOWACT;
    l |= low_active ? IOAPIC_REDIR_LOWACT : 0;

    //h |= dest; // 0 now
    l |= (0xFF & vector);

    writeIoApic(IOAPIC_REDIR + input*2, h);
    writeIoApic(IOAPIC_REDIR + input*2 +1, l);
}



void dumpIoApicInputs(void)
{
    int i;
    for( i = 0; i < 24; i++ )
    {
        int h = readIoApic(IOAPIC_REDIR + i*2);
        int l = readIoApic(IOAPIC_REDIR + i*2 +1);

        // 63:56 - topmost byte
        int dest = (h >> 24) & 0xFF;



        // 16 mask
        int masked  = (l >> 16) & 0x01;
        // 15 level
        int level   = (l >> 15) & 0x01;
        // 14 rem irr
        //int remirr  = (l >> 14) & 0x01;
        // 13 low active
        //int loact   = (l >> 13) & 0x01;
        // 12 delivery status
        int delivst = (l >> 12) & 0x01;
        // 11
        int destmod = (l >> 11) & 0x01;
        // 10:8
        int delmod  = (l >>  8) & 0x07;
        // 7:0 vector
        int vect    = (l >>  0) & 0xFF;

        char *mode = "?";
        switch( delmod )
        {
        case 0:         mode = "fixed "; break;
        case 1:         mode = "lowpri"; break;
        case 2:         mode = "smi   "; break;
        case 3:         mode = "reserv"; break;
        case 4:         mode = "nmi   "; break;
        case 5:         mode = "init  "; break;
        case 6:         mode = "resrv2"; break;
        case 7:         mode = "ExtINT"; break;
        }

        printf("%2X: vect %2X, dest %2d, mode=%s %s %s dest=%s%s\n",
               i, vect, dest, mode,
               masked ? "mask" : "on  ",
               level ? "level" : "edge ",
               destmod ? "logical" : "physical",
               delivst ? ", pending" : ""

               );
    }

}








#if 0


#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800
 
/** returns a 'true' value if the CPU supports APIC
 *  and if the local APIC hasn't been disabled in MSRs
 *  note that this requires CPUID to be supported.
 */
boolean cpuHasAPIC()
{
   dword a,d;
   cpuid(1,&a,&d);
   return d&CPUID_FLAG_APIC;
}
 
/** defines the physical address for local APIC registers
 */
void cpuSetAPICBase(phys_addr apic)
{
   dword a=(apic&0xfffff000) | IA32_APIC_BASE_MSR_ENABLE;
#ifdef __PHYSICAL_MEMORY_EXTENSION__
   dword d=(apic>>32) & 0x0f;
#else
   dword d=0;
#endif
 
   cpuSetMSR(IA32_APIC_BASE_MSR, a,d);
}
 
/** determines the physical address of the APIC registers page
 *  make sure you map it to virtual memory ;)
 */
phys_addr cpuGetAPICBase()
{
   dword a,d;
   cpuGetMSR(IA32_APIC_BASE_MSR,&a,&d);
#ifdef __PHYSICAL_MEMORY_EXTENSION__
   return (a&0xfffff000)|((d&0x0f)<<32);
#else
   return (a&0xfffff000);
#endif
}



#endif
