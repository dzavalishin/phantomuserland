#if 0 // not ready yet
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Arch-independent SMP support. Inter-CPU messaging.
 *
 *
**/

#define DEBUG_MSG_PREFIX "smp"
#include "debug_ext.h"
#define debug_level_flow 11
#define debug_level_error 10
#define debug_level_info 10

// Supposed that usual spin lock processes incoming IPIs and this one does not
// Ignore for now
#define acquire_spinlock_nocheck hal_spin_lock
#define acquire_spinlock hal_spin_lock
#define release_spinlock hal_spin_unlock

#include "newos.h"

#include "config.h"

#include <phantom_libc.h>
#include <kernel/smp.h>
#include <spinlock.h>
#include <hal.h>
#include <i386/proc_reg.h>
#include <machdep.h>

#define MSG_POOL_SIZE (_MAX_CPUS * 4)

struct smp_msg {
    struct smp_msg *next;
    int            message;
    unsigned long   data;
    unsigned long   data2;
    unsigned long   data3;
    void           *data_ptr;
    int            flags;
    int            ref_count;
    volatile bool  done;
    unsigned int   proc_bitmap;
    int            lock;
};

enum {
    MAILBOX_LOCAL = 1,
    MAILBOX_BCAST
};



// -----------------------------------------------------------------------
//
// Inter-CPU messaging
//
// -----------------------------------------------------------------------

static struct smp_msg *free_msgs = 0;
static volatile int free_msg_count = 0;
static hal_spinlock_t free_msg_spinlock;

static struct smp_msg *smp_msgs[MAX_CPUS]; // = { 0, };
static hal_spinlock_t cpu_msg_spinlock[MAX_CPUS]; // = { 0, };

static struct smp_msg *smp_broadcast_msgs = 0;
static hal_spinlock_t broadcast_msg_spinlock; // = 0;


// finds a free message and gets it
// NOTE: has side effect of disabling interrupts
static void find_free_message(struct smp_msg **msg)
{

retry:
    while(free_msg_count <= 0)
        ;
    int ie = hal_save_cli();
    hal_spin_lock(&free_msg_spinlock);

    if(free_msg_count <= 0) {
        // someone grabbed one while we were getting the lock,
        // go back to waiting for it
        hal_spin_unlock(&free_msg_spinlock);
        if(ie) hal_sti();
        goto retry;
    }

    *msg = free_msgs;
    free_msgs = (*msg)->next;
    free_msg_count--;

    hal_spin_unlock(&free_msg_spinlock);
}


static void return_free_message(struct smp_msg *msg)
{
    //	dprintf("return_free_message: returning msg 0x%x\n", msg);
    acquire_spinlock_nocheck(&free_msg_spinlock);
    msg->next = free_msgs;
    free_msgs = msg;
    free_msg_count++;
    release_spinlock(&free_msg_spinlock);
}



static struct smp_msg *smp_check_for_message(int curr_cpu, int *source_mailbox)
{
    struct smp_msg *msg;

    acquire_spinlock_nocheck(&cpu_msg_spinlock[curr_cpu]);
    msg = smp_msgs[curr_cpu];
    if(msg != 0) {
        smp_msgs[curr_cpu] = msg->next;
        release_spinlock(&cpu_msg_spinlock[curr_cpu]);
        //		dprintf(" found msg 0x%x in cpu mailbox\n", msg);
        *source_mailbox = MAILBOX_LOCAL;
    } else {
        // try getting one from the broadcast mailbox

        release_spinlock(&cpu_msg_spinlock[curr_cpu]);
        acquire_spinlock_nocheck(&broadcast_msg_spinlock);

        msg = smp_broadcast_msgs;
        while(msg != 0) {
            if(CHECK_BIT(msg->proc_bitmap, curr_cpu) != 0) {
                // we have handled this one already
                msg = msg->next;
                continue;
            }

            // mark it so we wont try to process this one again
            msg->proc_bitmap = SET_BIT(msg->proc_bitmap, curr_cpu);
            *source_mailbox = MAILBOX_BCAST;
            break;
        }
        release_spinlock(&broadcast_msg_spinlock);
        //		dprintf(" found msg 0x%x in broadcast mailbox\n", msg);
    }
    return msg;
}


static int smp_process_pending_ici(int curr_cpu)
{
    struct smp_msg *msg;
    bool do_halt = false;
    int source_mailbox = 0;
    int retval = INT_NO_RESCHEDULE;

    msg = smp_check_for_message(curr_cpu, &source_mailbox);
    if(msg == 0)
        return retval;

    //	dprintf("  message = %d\n", msg->message);
    switch(msg->message) {
    case SMP_MSG_INVL_PAGE_RANGE:
        arch_cpu_invalidate_TLB_range((addr_t)msg->data, (addr_t)msg->data2);
        break;
    case SMP_MSG_INVL_PAGE_LIST:
        arch_cpu_invalidate_TLB_list((addr_t *)msg->data, (int)msg->data2);
        break;
    case SMP_MSG_GLOBAL_INVL_PAGE:
        //arch_cpu_global_TLB_invalidate();
        invltlb();
        break;
    case SMP_MSG_RESCHEDULE:
        retval = INT_RESCHEDULE;
        break;
    case SMP_MSG_CPU_HALT:
        do_halt = true;
        SHOW_INFO( 0, "cpu %d halted!", curr_cpu);
        break;
    case SMP_MSG_1:
    default:
        SHOW_ERROR( 0, "smp_intercpu_int_handler: got unknown message %d", msg->message);
    }

    // finish dealing with this message, possibly removing it from the list
    smp_finish_message_processing(curr_cpu, msg, source_mailbox);

    // special case for the halt message
    // we otherwise wouldn't have gotten the opportunity to clean up
    if(do_halt) {
        //int_disable_interrupts();
        hal_cli();
        halt();
        for(;;);
    }

    return retval;
}

#endif
