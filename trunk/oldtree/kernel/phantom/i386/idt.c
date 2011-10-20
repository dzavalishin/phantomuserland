/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * ia32 idt support
 *
**/

#define DEBUG_MSG_PREFIX "idt"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/config.h>

#include <ia32/seg.h>
#include <phantom_types.h>
#include <phantom_libc.h>

#include <kernel/ia32/idt.h>
#include <kernel/ia32/apic.h>

//extern struct real_gate idt[];

struct idt_init_entry
{
    unsigned entrypoint;
    unsigned short vector;
    unsigned short type;
};
extern struct idt_init_entry idt_inittab[];


/* Fill a gate in the IDT.  */
#define fill_idt_gate(int_num, entry, selector, access, dword_count) \
	fill_gate(&idt[int_num], entry, selector, access, dword_count)


/* Fill a gate with particular values.  */
void
fill_gate(struct real_gate *gate, unsigned offset, unsigned short selector,
	  unsigned char access, unsigned char word_count)
{
	gate->offset_low = offset & 0xffff;
	gate->selector = selector;
	gate->word_count = word_count;
	gate->access = access | ACC_P;
	gate->offset_high = (offset >> 16) & 0xffff;
}






/** Load the IDT pointer into the processor.  */
void phantom_load_idt(void)
{
    struct pseudo_descriptor pdesc;

    pdesc.limit = sizeof(idt)-1;
    pdesc.linear_base = kvtolin(&idt);
    set_idt(&pdesc);
}


/* defined in intr.S */
extern u_int32_t int_entry_table[];

/* defined in apic_idt.S */
extern u_int32_t apic_entry_table[];

#if HAVE_KOLIBRI
/* defined in intr.S */
extern u_int32_t kolibri_entry_table[];
#endif // HAVE_KOLIBRI



void phantom_fill_idt(void)
{
    struct idt_init_entry *iie = idt_inittab;

    /* Initialize the exception vectors from the idt_inittab.  */
    while (iie->entrypoint)
    {
        fill_idt_gate(iie->vector, iie->entrypoint, KERNEL_CS, iie->type, 0);
        iie++;
    }

    // Init PIC vectors
    int i;
    for (i = 0; i < 16; i++)
    {
        //printf("IRQ%d @0x%X, ", i, int_entry_table[i] );
        fill_idt_gate(PIC_INT_BASE + i,
                      int_entry_table[i], KERNEL_CS,
                      ACC_PL_K|ACC_INTR_GATE, 0);
    }

    // Init APIC vectors
    for (i = 0; i < 32; i++)
    {
        //printf("APIC IRQ%d @0x%X, ", i, int_entry_table[i] );
        fill_idt_gate(APIC_INT_BASE + i,
                      apic_entry_table[i], KERNEL_CS,
                      ACC_PL_K|ACC_INTR_GATE, 0);
    }


#if HAVE_KOLIBRI
    fill_idt_gate( KOLIBRI_INT,
                   kolibri_entry_table[0], KERNEL_CS,
                   ACC_PL_U|ACC_INTR_GATE, 0);
                   //ACC_PL_K|ACC_INTR_GATE, 0);
#endif // HAVE_KOLIBRI

}

