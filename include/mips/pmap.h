
// Total PTE entries - just array
#define NPTE 0x100000

// Total TLB entries - TODO BUG FIXME - must be defined in board or read runtime
#define NTLBE 48

#ifndef ASSEMBLER

struct mips_pt_entry
{
	
};

typedef struct mips_pt_entry pt_entry_t;

#endif
