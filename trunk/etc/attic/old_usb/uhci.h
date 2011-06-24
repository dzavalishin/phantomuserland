#if 0
#include <sys/cdefs.h>

//#include "uhcireg.h"
#include "uhci_hardware.h"
#include <queue.h>

#include <hal.h>

#include <kernel/bus/usb/usb_spec.h>

#define UHCI_MAXTD 256

typedef struct uhci {

	// data about the hcf

        //region_id 	reg_region;
        //physaddr_t      reg_physaddr;
        //ohci_regs *	regs;

    //! Locks access to queue, freelist and, possibly, all the rest
        hal_mutex_t     lock;

        queue_head_t    all_requests;

        uhci_td_t       td[UHCI_MAXTD];

        uhci_td_t       *free_td_list;

} uhci;


typedef struct uhci_req {
    uhci_td_t           td;

    queue_chain_t       all_chain;

    void (*callback)(struct uhci_req *);

} __packed uhci_req_t ;


#endif
