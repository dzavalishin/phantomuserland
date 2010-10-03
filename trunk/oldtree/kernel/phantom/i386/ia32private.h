#include <i386/seg.h>

void phantom_load_main_tss(void);
void set_descriptor_limit( struct real_descriptor *d, unsigned limit );
