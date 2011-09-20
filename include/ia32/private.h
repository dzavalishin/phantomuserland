#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif

#include <ia32/seg.h>
#include <ia32/tss.h>
#include <errno.h>

void phantom_load_main_tss(void);
void set_descriptor_limit( struct real_descriptor *d, unsigned limit );

extern struct real_descriptor 	gdt[];
//struct real_descriptor 	ldt[LDTSZ];

extern struct i386_tss	       	cpu_tss[];
//extern struct vm86tss		tss_vm86;

errno_t get_uldt_sel( u_int16_t *osel, linaddr_t sel_base, size_t sel_limit, int code, int is32 );
errno_t get_uldt_cs_ds(
                       linaddr_t cs_base, u_int16_t *ocs, size_t cs_limit,
                       linaddr_t ds_base, u_int16_t *ods, size_t ds_limit
                      );

// load segment limit
unsigned ia32_lsl( unsigned selector );
