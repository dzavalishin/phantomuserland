#if 0

#ifdef ARCH_ia32

#define DEBUG_MSG_PREFIX "uhci"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/vm.h>
#include <x86/phantom_pmap.h>
#include <hal.h>
#include <phantom_libc.h>
#include <malloc.h>
#include <time.h>
#include <errno.h>
#include <i386/pio.h>

//#include "driver_map.h"

#include <queue.h>

#include "uhci.h"
//#include "uhcireg.h"

#include "uhci_hardware.h"

#include "usb_spec.h"
#include "usb_hc.h"







void
UHCI_LinkDescriptors(uhci_td *first, uhci_td *second)
{
	first->link_phy = second->this_phy | TD_DEPTH_FIRST;
	first->link_log = second;
}


#endif // ARCH_ia32

#endif
