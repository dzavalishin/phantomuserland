#include <kernel/config.h>

#if COMPILE_OHCI

/*
 ** Copyright 2003, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */

#include <compat/newos.h>
//#include "newos.h"

#include <kernel/vm.h>
#include <x86/phantom_pmap.h>
#include <hal.h>
#include <phantom_libc.h>
#include <malloc.h>
#include <time.h>
#include <threads.h>

#include "driver_map.h"

#include "ohci.h"

#include "usb_spec.h"
#include "usb_hc.h"


#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#define DEV_NAME "OHCI "
#define DEBUG_MSG_PREFIX "OHCI - "

#include "debug_ext.h"

#define phys_to_virt(oi, phys) (((addr_t)(phys) - (oi)->hcca_phys) + (addr_t)(oi)->hcca)

#pragma GCC diagnostic ignored "-Wunused-variable"
static ohci *oi_list = NULL;

static ohci_td *allocate_td(ohci *oi)
{
    ohci_td *td;

    // pull a td from the freelist
    hal_mutex_lock(&oi->td_freelist_lock);
    td = oi->td_freelist;
    if(td)
        oi->td_freelist = td->next;
    hal_mutex_unlock(&oi->td_freelist_lock);

    if(!td)
        return NULL;

    td->flags = 0;
    td->curr_buf_ptr = 0;
    td->next_td = 0;
    td->buf_end = 0;

    return td;
}

static void free_td(ohci *oi, ohci_td *td)
{
    hal_mutex_lock(&oi->td_freelist_lock);
    td->next = oi->td_freelist;
    oi->td_freelist = td;
    hal_mutex_unlock(&oi->td_freelist_lock);
}

static ohci_ed *create_ed(void)
{
    ohci_ed *ed;

    ed = malloc(sizeof(ohci_ed));
    if(!ed)
        return NULL;

    memset(ed, 0, sizeof(ohci_ed));

    //vm_get_page_mapping(vm_get_kernel_aspace_id(), (addr_t)ed, &ed->phys_addr);
    ed->phys_addr = kvtophys(ed);

    return ed;
}

static void enqueue_ed(ohci_ed *ed)
{
    ohci_queue *queue = ed->queue;
    ohci *oi = ed->oi;

    hal_mutex_lock(&oi->hc_list_lock);

    // stick it in our queue
    ed->prev_ed = NULL;
    ed->next_ed = queue->head;
    if(ed->next_ed != NULL)
        ed->next_ed->prev_ed = ed;
    queue->head = ed;

    // put it in the hardware's queue
    ed->next = *queue->head_phys;
    *queue->head_phys = (uint32)ed->phys_addr;

    hal_mutex_unlock(&oi->hc_list_lock);
}

static ohci_ed *create_endpoint(ohci *oi, usb_endpoint_descriptor *endpoint, int address, bool lowspeed)
{
    ohci_ed *ed;
    ohci_td *td;

    ed = create_ed();
    if(!ed)
        return ed;

    // save a pointer to the original usb endpoint structure
    ed->usb_ed = endpoint;
    ed->oi = oi;

    // figure out what queue it should be in
    switch(endpoint->attributes & 0x3) {
    case USB_ENDPOINT_ATTR_CONTROL:
        ed->queue = &oi->control_queue;
        break;
    case USB_ENDPOINT_ATTR_ISO:
        // not supported
        break;
    case USB_ENDPOINT_ATTR_BULK:
        ed->queue = &oi->bulk_queue;
        break;
    case USB_ENDPOINT_ATTR_INT:
        ed->queue = &oi->interrupt_queue[INT_Q_32MS]; // XXX find the correct queue and rotate through them
        break;
    }

    td = allocate_td(oi);

    // set some hardware flags based on the usb endpoint's data
    ed->flags = (endpoint->max_packet_size << 16) | ((endpoint->endpoint_address & 0xf) << 7) | (address & 0x7f);

    if((endpoint->attributes & 0x3) == USB_ENDPOINT_ATTR_CONTROL) {
        ed->flags |= OHCI_ED_FLAGS_DIR_TD;
    } else {
        if(endpoint->endpoint_address & 0x80)
            ed->flags |= OHCI_ED_FLAGS_DIR_IN;
        else
            ed->flags |= OHCI_ED_FLAGS_DIR_OUT;
    }
    if(lowspeed)
        ed->flags |= OHCI_ED_FLAGS_SPEED;

    // stick the null transfer descriptor in our endpoint descriptors list
    ed->tail_td = td;
    ed->tail = ed->head = td->phys_addr;
    ed->prev_ed = ed->next_ed = NULL;

    // enqueue this descriptor
    enqueue_ed(ed);

    return ed;
}

static void ohci_done_list_processor(void *_oi)
{
    ohci *oi = (ohci *)_oi;
    ohci_td *td;
    ohci_td *done_list;
    addr_t td_phys;

    hal_set_thread_name("OHCI Done");

    for(;;) {
        hal_sem_acquire(&oi->done_list_sem);

        if(oi->hcca->done_head != 0)
            SHOW_FLOW(6, "WDH 0x%x (0x%lx)", oi->hcca->done_head, phys_to_virt(oi, oi->hcca->done_head));

        done_list = NULL;

        // pull all of the entries from the done list, put them in another list in reverse order
        td_phys = oi->hcca->done_head & 0xfffffff0;
        while(td_phys != 0) {
            td = (ohci_td *)phys_to_virt(oi, td_phys);

            td_phys = td->next_td;

            td->next = done_list;
            done_list = td;
        }

        // ack the interrupt
        oi->hcca->done_head = 0;
        oi->regs->interrupt_status = INT_WDH;

        td = done_list;
        while(done_list) {
            td = done_list;
            bool transfer_complete = false;

            SHOW_FLOW(6, "dealing with %p: CC 0x%x", td, OHCI_TD_FLAGS_CC(td->flags));

            if(OHCI_TD_FLAGS_CC(td->flags) == OHCI_CC_NoError) {
                if(td->last_in_transfer) {
                    SHOW_FLOW(6, "transfer %p now finished", td->usb_transfer);

                    td->usb_transfer->status = NO_ERROR;
                    transfer_complete = true;
                }
            } else {
                // there was an error, the endpoint is halted
                td->usb_transfer->status = ERR_GENERAL; // XXX make smarter
                transfer_complete = true;

                SHOW_FLOW(6, "stalled transfer, part of usb transfer %p", td->usb_transfer);

                // walk the head pointer of this endpoint and remove the rest of this transfer's descriptors
                // the next one after this one should be at the head of the list
                td_phys = td->ed->head & 0xfffffff0;
                while(td_phys != 0) {
                    ohci_td *temp_td = (ohci_td *)phys_to_virt(oi, td_phys);

                    SHOW_FLOW(6, "stalled transfer at %p (0x%lx), usb transfer %p", temp_td, td_phys, temp_td->usb_transfer);

                    if(temp_td->usb_transfer != td->usb_transfer)
                        break;

                    // this transfer descriptor is part of the errored transfer
                    td_phys = temp_td->next_td;
                    free_td(oi, temp_td);
                }
                SHOW_FLOW(6, "setting head to 0x%lx, tail is at 0x%x", td_phys, td->ed->tail);
                td->ed->head = td_phys; // set the new head pointer
                oi->regs->command_status = COMMAND_CLF | COMMAND_BLF;
            }

            if(transfer_complete) {
                if(td->usb_transfer->callback != NULL)
                    td->usb_transfer->callback(td->usb_transfer, td->usb_transfer->cookie);
                //if(td->usb_transfer->completion_cond >= 0)
                hal_cond_broadcast(&td->usb_transfer->completion_cond);
            }

            done_list = td->next;
            free_td(oi, td);
        }
    }

    //return 0;
}

static void ohci_interrupt(void *arg)
{
    ohci *oi = (ohci *)arg;
    uint32 int_stat = oi->regs->interrupt_status;
    //int ret = INT_NO_RESCHEDULE;
    int i;

    if((int_stat & oi->regs->interrupt_enable) == 0)
        return;// 0; //INT_NO_RESCHEDULE;

    SHOW_FLOW(7, "oi %p int 0x%x (%d) enabled 0x%x disabled 0x%x", oi, int_stat, oi->rh_ports, oi->regs->interrupt_enable, oi->regs->interrupt_disable);

    if(int_stat & INT_SO) {
        SHOW_FLOW0(8, "\tscheduler overrun");
    }

    if(int_stat & INT_WDH) {
        SHOW_FLOW0(8, "\twriteback done head");
        int_stat &= ~INT_WDH; // dont ack it here
        //sem_release_etc(oi->done_list_sem, 1, SEM_FLAG_NO_RESCHED);
        hal_sem_release(&oi->done_list_sem);
        //ret = INT_RESCHEDULE;
    }

    if(int_stat & INT_SOF) {
        SHOW_FLOW0(8, "\tstart of frame");
    }

    if(int_stat & INT_RD) {
        SHOW_FLOW0(8, "\tresume detected");
    }

    if(int_stat & INT_UE) {
        SHOW_FLOW0(8, "\tunrecoverable error");
    }

    if(int_stat & INT_FNO) {
        SHOW_FLOW0(8, "\tframe number overrun");
    }

    if(int_stat & INT_RHSC) {
        SHOW_FLOW0(8, "\troot status change");
        SHOW_FLOW(8, "\trh_status 0x%x", oi->regs->rh_status);
        for(i = 0; i < oi->rh_ports; i++)
            SHOW_FLOW(8, "\t\trh_port_status %d: 0x%x", i, oi->regs->rh_port_status[i]);

    }

    if(int_stat & INT_OC) {
        SHOW_FLOW0(8, "\townership change");
    }

    if(int_stat & INT_MIE) {
        SHOW_FLOW0(8, "\tmaster interrupt enable");
    }

    oi->regs->interrupt_status = int_stat;

    //return 0; //ret;
}

static int ohci_create_endpoint(hc_cookie *cookie, hc_endpoint **hc_endpoint,
                                usb_endpoint_descriptor *usb_endpoint, int address, bool lowspeed)
{
    ohci *oi = (ohci *)cookie;

    *hc_endpoint = create_endpoint(oi, usb_endpoint, address, lowspeed);
    if(*hc_endpoint == NULL)
        return ERR_NO_MEMORY;

    return NO_ERROR;
}

static int ohci_destroy_endpoint(hc_cookie *cookie, hc_endpoint *hc_endpoint)
{
	(void) cookie;
	(void) hc_endpoint;

    // XXX implement

    return NO_ERROR;
}

static int ohci_enqueue_transfer(hc_cookie *cookie, hc_endpoint *endpoint, usb_hc_transfer *transfer)
{
    ohci *oi = (ohci *)cookie;
    ohci_ed *ed = (ohci_ed *)endpoint;
    usb_endpoint_descriptor *usb_ed = ed->usb_ed;

    switch(usb_ed->attributes & 0x3) {
    case USB_ENDPOINT_ATTR_CONTROL: {
        ohci_td *td0, *td1, *td2, *td3;
        int dir;

        // the direction of the transfer is based off of the first byte in the setup data
        dir = *((uint8 *)transfer->setup_buf) & 0x80 ? 1 : 0;

        hal_mutex_lock(&oi->hc_list_lock);

        SHOW_FLOW(6, "head 0x%x, tail 0x%x", ed->head, ed->tail);

        // allocate the transfer descriptors needed
        td0 = ed->tail_td;
        td2 = allocate_td(oi);
        if(transfer->data_buf != NULL)
            td1 = allocate_td(oi);
        else
            td1 = td2;
        td3 = allocate_td(oi); // this will be the new null descriptor

        // setup descriptor
        td0->flags = OHCI_TD_FLAGS_CC_NOT | OHCI_TD_FLAGS_DIR_SETUP | OHCI_TD_FLAGS_IRQ_NONE | OHCI_TD_FLAGS_TOGGLE_0;

        //vm_get_page_mapping(vm_get_kernel_aspace_id(), (addr_t)transfer->setup_buf, (addr_t *)&td0->curr_buf_ptr);
        td0->curr_buf_ptr = kvtophys(transfer->setup_buf);

        td0->buf_end = td0->curr_buf_ptr + transfer->setup_len - 1;
        td0->next_td = td1->phys_addr; // might be td2
        td0->transfer_head = td0;
        td0->transfer_next = td1;
        td0->usb_transfer = transfer;
        td0->ed = ed;
        td0->last_in_transfer = false;

        SHOW_FLOW(6, "td0 %p (0x%lx) flags 0x%x, curr_buf_ptr 0x%x, buf_end 0x%x, next_td 0x%x",
                  td0, td0->phys_addr, td0->flags, td0->curr_buf_ptr, td0->buf_end, td0->next_td);

        if(transfer->data_buf != NULL) {
            // data descriptor
            td1->flags = OHCI_TD_FLAGS_CC_NOT | OHCI_TD_FLAGS_TOGGLE_1| OHCI_TD_FLAGS_IRQ_NONE | OHCI_TD_FLAGS_ROUNDING |
                (dir ? OHCI_TD_FLAGS_DIR_IN : OHCI_TD_FLAGS_DIR_OUT);

            //vm_get_page_mapping(vm_get_kernel_aspace_id(), (addr_t)transfer->data_buf, (addr_t *)&td1->curr_buf_ptr);
            td1->curr_buf_ptr = kvtophys(transfer->data_buf);

            td1->buf_end = td1->curr_buf_ptr + transfer->data_len - 1;
            td1->next_td = td2->phys_addr;
            td1->transfer_head = td0;
            td1->transfer_next = td2;
            td1->usb_transfer = transfer;
            td1->ed = ed;
            td1->last_in_transfer = false;

            SHOW_FLOW(6, "td1 %p (0x%lx) flags 0x%x, curr_buf_ptr 0x%x, buf_end 0x%x, next_td 0x%x",
                      td1, td1->phys_addr, td1->flags, td1->curr_buf_ptr, td1->buf_end, td1->next_td);
        }

        // ack descriptor
        td2->flags = OHCI_TD_FLAGS_CC_NOT | OHCI_TD_FLAGS_TOGGLE_1 |
            (dir ? OHCI_TD_FLAGS_DIR_OUT : OHCI_TD_FLAGS_DIR_IN);
        td2->curr_buf_ptr = 0;
        td2->buf_end = 0;
        td2->next_td = td3->phys_addr;
        td2->transfer_head = td0;
        td2->transfer_next = NULL;
        td2->usb_transfer = transfer;
        td2->ed = ed;
        td2->last_in_transfer = true;

        SHOW_FLOW(6, "td2 %p (0x%lx) flags 0x%x, curr_buf_ptr 0x%x, buf_end 0x%x, next_td 0x%x",
                  td2, td2->phys_addr, td2->flags, td2->curr_buf_ptr, td2->buf_end, td2->next_td);

        // next null descriptor is td3, it should be nulled out from allocate_td()
        td3->ed = NULL;
        td3->usb_transfer = NULL;
        td3->transfer_head = td3->transfer_next = NULL;

        SHOW_FLOW(6, "td3 %p (0x%lx) flags 0x%x, curr_buf_ptr 0x%x, buf_end 0x%x, next_td 0x%x",
                  td3, td3->phys_addr, td3->flags, td3->curr_buf_ptr, td3->buf_end, td3->next_td);

        ed->tail_td = td3;
        ed->tail = td3->phys_addr;

        oi->regs->command_status = COMMAND_CLF;

        hal_mutex_unlock(&oi->hc_list_lock);
        break;
    }
    case USB_ENDPOINT_ATTR_INT: {
        ohci_td *td0, *td1;

        hal_mutex_lock(&oi->hc_list_lock);

        td0 = ed->tail_td;
        td1 = allocate_td(oi);

        // XXX only deals with non page boundary data transfers
        td0->flags = OHCI_TD_FLAGS_CC_NOT | OHCI_TD_FLAGS_ROUNDING;

        //vm_get_page_mapping(vm_get_kernel_aspace_id(), (addr_t)transfer->data_buf, (addr_t *)&td0->curr_buf_ptr);
        td0->curr_buf_ptr = kvtophys(transfer->data_buf);

        td0->buf_end = td0->curr_buf_ptr + transfer->data_len - 1;
        td0->next_td = td1->phys_addr;
        td0->transfer_head = td0;
        td0->transfer_next = td1;
        td0->usb_transfer = transfer;
        td0->ed = ed;
        td0->last_in_transfer = true;

        SHOW_FLOW(6, "td0 %p (0x%lx) flags 0x%x, curr_buf_ptr 0x%x, buf_end 0x%x, next_td 0x%x",
                  td0, td0->phys_addr, td0->flags, td0->curr_buf_ptr, td0->buf_end, td0->next_td);

        // new tail
        td1->ed = NULL;
        td1->usb_transfer = NULL;
        td1->transfer_head = td1->transfer_next = NULL;

        SHOW_FLOW(6, "td1 %p (0x%lx) flags 0x%x, curr_buf_ptr 0x%x, buf_end 0x%x, next_td 0x%x",
                  td1, td1->phys_addr, td1->flags, td1->curr_buf_ptr, td1->buf_end, td1->next_td);

        ed->tail_td = td1;
        ed->tail = td1->phys_addr;

        oi->regs->command_status = COMMAND_BLF;

        hal_mutex_unlock(&oi->hc_list_lock);
        break;
    }
    default:
        printf("unsupported transfer type %d", ed->usb_ed->attributes & 0x3);
        return ERR_UNIMPLEMENTED;
    }

    return NO_ERROR;
}

#if 1
#pragma GCC diagnostic ignored "-Wunused-function"
static void ohci_test(ohci *oi) // __attribute__((__unused__))
{
    static usb_endpoint_descriptor desc = {
        sizeof(usb_endpoint_descriptor),
        USB_DESCRIPTOR_ENDPOINT,
        0, 0, 8, 0
    };
    static const usb_request GET_DESCRIPTOR = {
        USB_REQTYPE_DEVICE_IN,
        USB_REQUEST_GET_DESCRIPTOR,
        USB_DESCRIPTOR_DEVICE << 8, 0, 8
    };
    static const usb_request SET_ADDR = {
        USB_REQTYPE_DEVICE_OUT,
        USB_REQUEST_SET_ADDRESS,
        1, 0, 0
    };
    static unsigned char buf[512];
    unsigned char *aligned_buf = (unsigned char *)((((addr_t)buf) & 0xffffffc0) + 0x40);
    static usb_hc_transfer transfer;
    ohci_ed *ed0, *ed1;

    ed0 = create_endpoint(oi, &desc, 0, 0);

    memcpy(aligned_buf, &GET_DESCRIPTOR, 8);
    transfer.setup_buf = aligned_buf;
    transfer.setup_len = 8;
    transfer.data_buf = aligned_buf;
    transfer.data_len = 8;
    transfer.callback = NULL;

    hal_mutex_t mutex;

    hal_mutex_init( &mutex, "OHCI test" );
    hal_cond_init(&transfer.completion_cond, "OHCI compl");

    ohci_enqueue_transfer(oi, ed0, &transfer);

    // TODO WRONG use sem back?
    hal_cond_wait( &transfer.completion_cond, &mutex ); //, 1, SEM_FLAG_TIMEOUT, 5000000, NULL);

    SHOW_FLOW(5, "transaction completed with err 0x%x", transfer.status);
    {
        usb_device_descriptor *dev = (usb_device_descriptor *)aligned_buf;

        SHOW_FLOW(5, "device: len %d class 0x%x subclass 0x%x protocol 0x%x",
                  dev->length, dev->device_class, dev->device_subclass, dev->device_protocol);
    }
    // TODO implement
    //hal_sem_delete(&transfer.completion_cond);

    memcpy(aligned_buf, &SET_ADDR, 8);
    transfer.setup_buf = aligned_buf;
    transfer.setup_len = 8;
    transfer.data_buf = NULL;
    transfer.data_len = 0;
    transfer.callback = NULL;
    hal_cond_init(&transfer.completion_cond, "OHCI compl");

    ohci_enqueue_transfer(oi, ed0, &transfer);

    // TODO WRONG use sem back?
    hal_cond_wait( &transfer.completion_cond, &mutex ); //, 1, SEM_FLAG_TIMEOUT, 5000000, NULL);
    SHOW_FLOW(5, "transaction completed with err 0x%x", transfer.status);
    // TODO kill
    //hal_cond_delete(&transfer.completion_cond);

    // new address
    ed1 = create_endpoint(oi, &desc, 1, 0);

    memcpy(aligned_buf, &GET_DESCRIPTOR, 8);
    transfer.setup_buf = aligned_buf;
    transfer.setup_len = 8;
    transfer.data_buf = aligned_buf;
    transfer.data_len = 8;
    transfer.callback = NULL;
    hal_cond_init(&transfer.completion_cond, "OHCI compl");

    ohci_enqueue_transfer(oi, ed1, &transfer);

    //hal_sem_acquire_etc(transfer.completion_cond, 1, SEM_FLAG_TIMEOUT, 5000000, NULL);
    hal_cond_wait(&transfer.completion_cond, &mutex );

    SHOW_FLOW(5, "transaction completed with err 0x%x", transfer.status);
    //hal_sem_delete(&transfer.completion_cond);

}
#endif

static ohci *ohci_init_hc(int interrupt_line, physaddr_t reg_base, size_t reg_size)
{
    int i;
    ohci *oi;
    ohci_ed *null_ed;
    ohci_td *null_td;
    uint32 saved_interval;
    uint32 largest_packet;

    oi = kmalloc(sizeof(ohci));
    if(!oi)
        goto err;

    SHOW_INFO(0, "allocated structure at %p", oi);

    memset(oi, 0, sizeof(*oi));

    // find the irq
    oi->irq = interrupt_line;

    SHOW_INFO(0, "irq %d", oi->irq);

    // XXX remove
    //if(oi->irq == 10)        goto err;


    // map the controller's registers
    /*
    oi->reg_region = vm_map_physical_memory(vm_get_kernel_aspace_id(), "ohci regs", (void **)&oi->regs,
                                            REGION_ADDR_ANY_ADDRESS, pinfo->u.h0.base_register_sizes[0], LOCK_RW|LOCK_KERNEL,
                                            pinfo->u.h0.base_registers[0]);
    if(oi->reg_region < 0) {
        SHOW_ERROR0(0, "ohci_init: error creating register region");
        goto err;
    }*/

    int reg_npages = (reg_size-1)/4096+1;
    if(hal_alloc_vaddress((void **)&oi->regs, reg_npages ))
        panic("put of vaddresses");


    hal_pages_control_etc( reg_base, oi->regs, reg_npages, page_map, page_rw, INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );


    SHOW_INFO(0, "regs at 0x%lx, mapped to %p, size 0x%lx (%d pages)", oi->regs, reg_base, reg_size, reg_npages);


    u_int8_t rev_maj = (oi->regs->revision >> 4) & 0xf;
    u_int8_t rev_min = oi->regs->revision & 0xf;
    // print and check the hardware rev
    SHOW_INFO(0, "hardware rev %d.%d%s", rev_maj, rev_min,
              oi->regs->revision & 0x100 ? " legacy support" : "");

    if(rev_maj != 1 || rev_min != 0) {
        SHOW_ERROR0(0, "hardware rev not supported, bailing...");
        goto err1;
    }


    // create a region for the hcca memory
    /*
    oi->hcca_region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "ohci hcca", (void **)&oi->hcca,
                                                 REGION_ADDR_ANY_ADDRESS, OHCI_HCCA_SIZE, REGION_WIRING_WIRED_CONTIG, LOCK_RW|LOCK_KERNEL);
    if(oi->hcca_region < 0) {
        SHOW_ERROR0(0, "ohci_init: error creating hcca region");
        goto err1;
    }
    vm_get_page_mapping(vm_get_kernel_aspace_id(), (addr_t)oi->hcca, &oi->hcca_phys);
    */

    oi->hcca = malloc(OHCI_HCCA_SIZE);
    oi->hcca_phys = kvtophys(oi->hcca);

    SHOW_INFO(0, "hcca region at %p, physical address 0x%lx", oi->hcca, oi->hcca_phys);

    // take the hardware back from SMM or anyone else that has had it before
    if(oi->regs->control & CONTROL_IR) {
        SHOW_INFO0(1, "SMM has control of the HC");
        oi->regs->command_status = oi->regs->command_status | COMMAND_OCR;

        // wait a total of 10ms for the host controller to come back to us
        for(i = 0; i < 10; i++) {
            if((oi->regs->control & CONTROL_IR) == 0) {
                // it was released
                break;
            }
            //cpu_spin(1000); // 1ms
            phantom_spinwait_msec(1);
        }
        if(i >= 10) {
            // SMM didn't release it
            SHOW_ERROR0(0, "SMM will not release control of the HC");
            goto err2;
        }
    }

    // create a thread to process the done list for this hc
    //oi->done_list_sem = sem_create(0, "ohci done list sem");
    hal_sem_init(&oi->done_list_sem, "OHCI done");
    oi->done_list_processor = thread_create_kernel_thread("ohci done list processor",
                                                          &ohci_done_list_processor, oi);
    // TODO put back
    //thread_set_priority(oi->done_list_processor, THREAD_RT_LOW_PRIORITY);
    //thread_resume_thread(oi->done_list_processor);

    // set up some queues
    oi->control_queue.head = NULL;
    oi->control_queue.head_phys = &oi->regs->control_head_ed;
    oi->bulk_queue.head = NULL;
    oi->bulk_queue.head_phys = &oi->regs->bulk_head_ed;
    hal_mutex_init(&oi->hc_list_lock,"OHCI list");

    // create a pool of transfer descriptors out of the remainder of the hcca region
    {
        addr_t ptr;
        ohci_td *td;

        ptr = (addr_t)oi->hcca + sizeof(ohci_hcca);
        oi->td_freelist = NULL;

        hal_mutex_init(&oi->td_freelist_lock,"OHCI free");

        while(ptr < ((addr_t)oi->hcca + OHCI_HCCA_SIZE)) {
            td = (ohci_td *)ptr;
            td->phys_addr = oi->hcca_phys + ((addr_t)ptr - (addr_t)oi->hcca);
            td->next = oi->td_freelist; // add it to the freelist
            oi->td_freelist = td;
            ptr += sizeof(ohci_td);
        }
    }

    // allocate a null endpoint and transfer descriptor for the 63 interrupt queues we'll have
    for(i = 0; i < 63; i++) {
        // allocate a null transfer descriptor
        null_td = allocate_td(oi);

        // create a null endpoint descriptor
        null_ed = create_ed();
        null_ed->flags = 0;
        null_ed->head = null_ed->tail = null_td->phys_addr;
        null_ed->next = 0;
        null_ed->next_ed = NULL;

        oi->interrupt_queue[i].head = null_ed;
        oi->interrupt_queue[i].head_phys = &null_ed->next;
    }

    // set up the interrupt 'tree' shown on page 10 of the OHCI spec 1.0a
    for(i=0; i < 2; i++) {
        oi->interrupt_queue[INT_Q_2MS + i].head->next_ed = oi->interrupt_queue[INT_Q_1MS].head;
        oi->interrupt_queue[INT_Q_2MS + i].head->next = *oi->interrupt_queue[INT_Q_1MS].head_phys;
    }
    for(i=0; i < 4; i++) {
        oi->interrupt_queue[INT_Q_4MS + i].head->next_ed = oi->interrupt_queue[INT_Q_2MS + i/2].head;
        oi->interrupt_queue[INT_Q_4MS + i].head->next = *oi->interrupt_queue[INT_Q_2MS + i/2].head_phys;
    }
    for(i=0; i < 8; i++) {
        oi->interrupt_queue[INT_Q_8MS + i].head->next_ed = oi->interrupt_queue[INT_Q_4MS + i/4].head;
        oi->interrupt_queue[INT_Q_8MS + i].head->next = *oi->interrupt_queue[INT_Q_4MS + i/4].head_phys;
    }
    for(i=0; i < 16; i++) {
        oi->interrupt_queue[INT_Q_16MS + i].head->next_ed = oi->interrupt_queue[INT_Q_8MS + i/8].head;
        oi->interrupt_queue[INT_Q_16MS + i].head->next = *oi->interrupt_queue[INT_Q_8MS + i/8].head_phys;
    }
    for(i=0; i < 32; i++) {
        oi->interrupt_queue[INT_Q_32MS + i].head->next_ed = oi->interrupt_queue[INT_Q_16MS + i/16].head;
        oi->interrupt_queue[INT_Q_32MS + i].head->next = *oi->interrupt_queue[INT_Q_16MS + i/16].head_phys;
        oi->hcca->interrupt_table[i] = oi->interrupt_queue[INT_Q_32MS + i].head->phys_addr;
    }

    // install the interrupt handler
    //int_set_io_interrupt_handler(oi->irq, &ohci_interrupt, oi, "ohci");
    if( hal_irq_alloc( oi->irq, &ohci_interrupt, oi, HAL_IRQ_SHAREABLE ) )
    {
        //if(DEBUG)
        panic("IRQ %d is busy\n", oi->irq );
//WW();
        //return -1;
    }

    // save the frame interval from the card
    saved_interval = oi->regs->frame_interval & 0x3fff;
    // calculate the largest packet size
    largest_packet = ((saved_interval - 210) * 6) / 7;
    SHOW_FLOW(1, "largest packet %d", largest_packet);

    // reset the controller
    oi->regs->command_status = COMMAND_HCR;
    for(i = 0; i < 100; i++) {
        if((oi->regs->control & COMMAND_HCR) == 0) {
            // it reset
            break;
        }
        //cpu_spin(10); // 1us
        // TODO need 1uSec, not 1msec!
        phantom_spinwait_msec(1);
    }
    if(i >= 100) {
        SHOW_ERROR0(0, "failed to reset the HC");
        goto err3;
    }

    // restore the frame interval register
    oi->regs->frame_interval = (largest_packet << 16) | saved_interval | 0x80000000;
    oi->regs->periodic_start = (saved_interval * 9) / 10; // 90% of frame interval
    oi->regs->ls_threshold = 8*8*8*7/6*2;
    SHOW_FLOW(1, "ls threshold %d", oi->regs->ls_threshold);

    oi->hcca->done_head = 0;
    oi->regs->HCCA = oi->hcca_phys;
    oi->regs->bulk_head_ed = 0;
    oi->regs->bulk_current_ed = 0;
    oi->regs->control_head_ed = 0;
    oi->regs->control_current_ed = 0;
    oi->regs->interrupt_disable = 0xffffffff;
    oi->regs->interrupt_status = 0xffffffff;

    // start it up
    oi->regs->control = CONTROL_HCFS_OPERATIONAL | CONTROL_CLE | CONTROL_BLE | CONTROL_PLE;

    //thread_snooze(5000);
    hal_sleep_msec(5);

    // reset a couple of the root ports
    // XXX move this into root hub code
    oi->rh_ports = (oi->regs->rh_descriptor_a & 0xff);
    SHOW_FLOW(1, "%d root ports", oi->rh_ports);

    oi->regs->rh_descriptor_a |= 0x0100;
    oi->regs->rh_descriptor_b |= 0x60000;

    for(i=0; i < oi->rh_ports; i++)
        oi->regs->rh_port_status[i] = 0x100; // set port power
    //thread_snooze(1000);
    hal_sleep_msec(1);

    for(i=0; i < oi->rh_ports; i++)
        oi->regs->rh_port_status[i] = 0x10; // reset port
    //thread_snooze(10000);
    hal_sleep_msec(10);

    for(i=0; i < oi->rh_ports; i++)
        oi->regs->rh_port_status[i] = 0x2; // enable port
    //thread_snooze(10000);
    hal_sleep_msec(10);

    // enable all interrupts except Start Of Frame
    oi->regs->interrupt_enable = INT_MIE | INT_OC | /* INT_RHSC | */ INT_FNO | INT_UE | INT_RD | INT_WDH | INT_SO;

    //	thread_snooze(1000000);

    //	ohci_test(oi);

    return oi;

err3:
    hal_mutex_destroy(&oi->hc_list_lock);
    hal_mutex_destroy(&oi->td_freelist_lock);
err2:
    // TODO impl
    //vm_delete_region(vm_get_kernel_aspace_id(), oi->hcca_region);
err1:
    // TODO impl
    //vm_delete_region(vm_get_kernel_aspace_id(), oi->reg_region);
err:
    if(oi)
        free(oi);
    return NULL;
}

#if 0
static int ohci_init(int (*hc_init_callback)(void *callback_cookie, void *cookie), void *callback_cookie)
{
    int i;
    pci_module_hooks *pci;
    pci_info pinfo;
    ohci *oi;
    int count = 0;

    if(module_get(PCI_BUS_MODULE_NAME, 0, (void **)&pci) < 0) {
        SHOW_ERROR0(0, "ohci_detect: no pci bus found..");
        return -1;
    }

    for(i = 0; pci->get_nth_pci_info(i, &pinfo) >= NO_ERROR; i++) {
        if(pinfo.class_base == OHCI_BASE_CLASS &&
           pinfo.class_sub == OHCI_SUB_CLASS &&
           pinfo.class_api == OHCI_INTERFACE) {

#if 0
            {
                int j;

                for(j=0; j < 6; j++) {
                    dprintf(" %d: base 0x%x size 0x%x\n", j, pinfo.u.h0.base_registers[j], pinfo.u.h0.base_register_sizes[j]);
                }
            }
#endif

            oi = ohci_init_hc(&pinfo);
            if(oi) {
                // add it to our list of ohci hcfs
                oi->next = oi_list;
                oi_list = oi;
                count++;

                // register it with the bus
                hc_init_callback(callback_cookie, oi);
            }
        }
    }
    module_put(PCI_BUS_MODULE_NAME);

    return count;
}
#endif


phantom_device_t * driver_ohci_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    printf( DEV_NAME " probe\n" );

    if(pci->sub_class != OHCI_SUB_CLASS || pci->interface != OHCI_INTERFACE)
    {
        printf( DEV_NAME "got subclass %X (need %X), interface %X (need %X)\n", pci->sub_class, OHCI_SUB_CLASS, pci->interface, OHCI_INTERFACE );
//getchar();
        return 0;
    }

    physaddr_t phys_base;
    size_t phys_size;

    int i;
    for (i = 0; i < 6; i++)
    {
        if (pci->base[i] > 0xffff)
        {
            phys_base = pci->base[i];
            phys_size = pci->size[i];
            printf( DEV_NAME "base 0x%lx, size 0x%lx\n", phys_base, phys_size);
        } else if( pci->base[i] > 0) {
            //nic->io_port = pci->base[i];
            if(debug_level_info) printf( DEV_NAME "io_port 0x%x\n", pci->base[i]);
        }
    }

    printf( DEV_NAME " init\n" );
    ohci *priv = ohci_init_hc(pci->interrupt, phys_base, phys_size);

    if(priv == 0)
        printf( DEV_NAME " init failed\n" );
    else
        printf( DEV_NAME " ready\n" );
//getchar();

return 0;
}


/*
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct usb_hc_module_hooks ohci_hooks = {
    0, //&ohci_init, // bus initialization
    0, //&ohci_uninit,
    &ohci_create_endpoint,
    &ohci_destroy_endpoint,
    &ohci_enqueue_transfer,
}; // __attribute__ ((unused));

static module_header ohci_module_header = {
    USB_HC_MODULE_NAME_PREFIX "/ohci/v1",
    MODULE_CURR_VERSION,
    0, // dont keep loaded
    &ohci_hooks,
    &ohci_module_init,
    &ohci_module_uninit
};

module_header *modules[]  = {
    &ohci_module_header,
    NULL
};*/


#endif // COMPILE_OHCI

