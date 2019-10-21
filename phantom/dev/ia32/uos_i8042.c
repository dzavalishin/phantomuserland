/*
 * Low-level routines for i8042 keyboard/mouse controller.
 *
 * Copyright (C) 2005 Serge Vakulenko
 */

#include <ia32/pio.h>
#include <phantom_libc.h>
#include <kernel/device.h>
#include <kernel/drivers.h>
#include <hal.h>
#include <errno.h>
#include <threads.h>

#include <compat/uos.h>
#include <ia32/pc/uos_i8042.h>

/*
 * Clear input/output buffers of keyboard controller.
 */
static void
i8042_wait ()
{
	int count, sts;

	for (count=0; count<10000; ++count) {
		sts = inb (KBDC_AT_CTL);
		if (! (sts & (KBSTS_CMDBSY | KBSTS_DATAVL))) {
			/* Input and output buffers are clear. */
			return;
		}
		if (sts & KBSTS_DATAVL) {
			/* Get data from input buffer. */
			inb (KBD_DATA);
		}
	}
	/*debug_printf ("i8042: busy\n");*/
}

/*
 * Read data from keyboard controller.
 * Return 1 on success, 0 when no data is available.
 */
int
i8042_read (unsigned char *data)
{
	int count, sts;

	for (count=0; count<10000; ++count) {
		sts = inb (KBDC_AT_CTL);
		if (sts & KBSTS_DATAVL) {
			/* Get data from input buffer. */
			*data = inb (KBD_DATA);
			return 1;
		}
	}
	return 0;
}

/*
 * Write a command to the port.
 */
static void
i8042_command (int cmd)
{
	i8042_wait ();
	outb_reverse (KBDC_WCMD, KBDC_AT_CTL);
	i8042_wait ();
	outb_reverse (cmd, KBD_DATA);
}

/*
 * Write to primary device.
 */
static void
i8042_kbd_write (int val)
{
	i8042_wait ();
	outb_reverse (val, KBD_DATA);
}

/*
 * Write to auxiliary device.
 */
void
i8042_aux_write (int val)
{
	i8042_wait ();
	outb_reverse (KBDC_WAUX, KBDC_AT_CTL);
	i8042_wait ();
	outb_reverse (val, KBD_DATA);
}

/*
 * Send two-byte command to the device on a primary port.
 */
int
i8042_kbd_command (int cmd, int param)
{
	int count;
	unsigned char ack;

	/* Send command and wait for an ack response. */
	/*debug_printf ("keyboard: command %02x-%02x\n", cmd, param);*/
	i8042_kbd_write (cmd);
	if (! i8042_read (&ack) || ack != KBDR_ACK) {
		/*debug_printf ("keyboard: no ACK after command\n");*/
		return 0;
	}

	/* Send a parameter and wait for an ack response. */
	i8042_kbd_write (param);
	for (count=0; count<100; ++count) {
		if (! i8042_read (&ack))
			continue;

		if (ack == KBDR_ACK) {
			return 1;
		}
		/*debug_printf ("keyboard: %02x (%d)\n", ack, count);*/
	}
	/*debug_printf ("keyboard: no ACK\n", cmd, param);*/
	return 0;
}

/*
 * Look to see if we can find a device on the primary port.
 */
int
i8042_kbd_probe ()
{
	int count;
	unsigned char ack;

	/* Send a reset and wait for an ack response. */
	i8042_kbd_write (KBDK_RESET);
	if (! i8042_read (&ack) || ack != KBDR_ACK) {
		/*debug_printf ("keyboard: probe: no ACK after reset\n");*/
		return 0;
	}

	/* Ensure that we see a basic assurance test response. */
	for (count=0; count<1000; ++count) {
		if (! i8042_read (&ack))
			continue;
		/*debug_printf ("keyboard: probe: %02x (%d)\n", ack, count);*/

		if (ack == KBDR_TEST_OK) {
			return 1;
		}
	}
	/*debug_printf ("keyboard: probe: no BAT after reset\n");*/
	return 0;
}

/*
 * Look to see if we can find a device on the aux port.
 */
int
i8042_aux_probe ()
{
	int count;
	unsigned char ack;

	/* Send a reset and wait for an ack response. */
	i8042_aux_write (KBDK_RESET);
	if (! i8042_read (&ack) || ack != KBDR_ACK) {
		/*debug_printf ("mouse: probe: no ACK after reset\n");*/
		return 0;
	}

	/* Ensure that we see a basic assurance test response. */
	for (count=0; count<1000; ++count) {
		if (! i8042_read (&ack))
			continue;
		/*debug_printf ("mouse: probe: %02x (%d)\n", ack, count);*/

		if (ack == KBDR_TEST_OK) {
			/* Check that we get a pointing device ID back */
			if (! i8042_read (&ack) || ack != 0) {
				/*debug_printf ("mouse: probe: no ACK after BAT\n");*/
				return 0;
			}
			return 1;
		}
	}
	/*debug_printf ("mouse: probe: no BAT after reset\n");*/
	return 0;
}

/*
 * Enable keyboard device and interrupt.
 */
void
i8042_kbd_enable ()
{
	/* Enable keyboard and AUX port. */
	//i8042_command (KBCB_ENINTR | KBCB_AUXINTR | KBCB_SYSFLG | KBCB_TRANSL);
	i8042_command (KBCB_ENINTR | KBCB_AUXINTR | KBCB_SYSFLG );
}

/*
 * Enable auxiliary device and interrupt.
 */
void
i8042_aux_enable ()
{
	/* Disable AUX port. */
	outb_reverse (KBDC_DISAUX, KBDC_AT_CTL);

	/* Send enable command to AUX device. It will reply with ACK. */
	i8042_aux_write (KBDK_ENABLE);

	/* Enable keyboard and AUX port. */
	//i8042_command (KBCB_ENINTR | KBCB_AUXINTR | KBCB_SYSFLG | KBCB_TRANSL);
	i8042_command (KBCB_ENINTR | KBCB_AUXINTR | KBCB_SYSFLG );
}

