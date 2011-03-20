

#define ARM_PT_SHIFT		10
#define ARM_PT_SIZE		1024
#define ARM_PD_SHIFT		16
// Bytes. 32 bits per entry.
#define ARM_PD_SIZE		0x4000

// Number of page dir entries (page tables) - depends on what transl scheme we use?
#define NPDE 4096


#define ARM_PDE_TYPE_INVALID	0x00000000
#define ARM_PDE_TYPE_PAGETBL	0x00000001
#define ARM_PDE_TYPE_SECTION	0x00000002
#define ARM_PDE_TYPE_RESERVE	0x00000003
#define ARM_PDE_TYPE_MASK       0x00000003
#define ARM_PDE_BUFFERABLE      0x00000004
#define ARM_PDE_CACHEABLE       0x00000008
#define ARM_PDE_DOMAIN_MASK	0x000001E0
#define ARM_PDE_AP_KR		0x00000000
#define ARM_PDE_AP_KRW          0x00000400
#define ARM_PDE_AP_KRWUR	0x00000800
#define ARM_PDE_AP_KRWURW	0x00000C00
#define ARM_PDE_AP_MASK         0x00000C00
#define ARM_PDE_PFN_PAGETBL	0xFFFFFC00
#define ARM_PDE_PFN_SECTION	0xFFF00000


#define ARM_PTE_TYPE_MASK       0x00000003
#define ARM_PTE_TYPE_SM_PAGE    0x00000002

#define ARM_PTE_SM_PAGE_SHIFT   12 // where addr field starts in pte
#define ARM_PTE_SM_PAGE_BITS    10 // how many bits wide it is

#define ARM_PTE_CACHED          (1<<3)
#define ARM_PTE_BUFFERED        (1<<2)


