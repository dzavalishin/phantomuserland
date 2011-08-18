#include <kernel/trap.h>

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif

#include <ia32/eflags.h>
#include <ia32/seg.h>
#include <ia32/tss.h>
#include <ia32/proc_reg.h>
#include <ia32/pio.h>

#define VM86_R0_STACK_SIZE (1024*64)

//#define VM86_EXE_SIZE (1024*64*3)
#define VM86_EXE_SIZE (1024*64*2)


struct vm86tss
{
    struct i386_tss	       	tss;
    char                        redir[32];
} __attribute__ ((packed));


extern struct vm86tss	       	tss_vm86;



// dos small model - code has 64K, data+stack have 64K, stack grows down
struct vm86_settings
{
    void *      code;
    void *      data;
    void *      stackLow; // Start of stack - used to calc relmode ss
    void *      stackHi;  // End of stack - used to check addresses
};

extern struct vm86_settings  vm86_setup;





/* register data for calling real mode interrupts */
typedef union 
{
   struct {
      unsigned long edi;
      unsigned long esi;
      unsigned long ebp;
      unsigned long res; // todo what is it?
      unsigned long ebx;
      unsigned long edx;
      unsigned long ecx;
      unsigned long eax;
   } d;
   struct {
      unsigned short di, di_hi;
      unsigned short si, si_hi;
      unsigned short bp, bp_hi;
      unsigned short res, res_hi;
      unsigned short bx, bx_hi;
      unsigned short dx, dx_hi;
      unsigned short cx, cx_hi;
      unsigned short ax, ax_hi;
      unsigned short flags;
      unsigned short es;
      unsigned short ds;
      unsigned short fs;
      unsigned short gs;
      unsigned short ip;
      unsigned short cs;
      unsigned short sp;
      unsigned short ss;
   } x;
   struct {
      unsigned char edi[4];
      unsigned char esi[4];
      unsigned char ebp[4];
      unsigned char res[4];
      unsigned char bl, bh, ebx_b2, ebx_b3;
      unsigned char dl, dh, edx_b2, edx_b3;
      unsigned char cl, ch, ecx_b2, ecx_b3;
      unsigned char al, ah, eax_b2, eax_b3;
   } h;
} __attribute__ ((packed)) RM_REGS;









//void phantom_v86_run(void *code, size_t size);

void phantom_bios_int_10(void);
void phantom_bios_int_10_args(RM_REGS *regs);



