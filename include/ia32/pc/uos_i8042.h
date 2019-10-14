/*
 * 8042 - PC/AT Keyboard Controller
 *
 * port 64h (read)   8042 status register. Can be read at any time.  See
 *                   table above for more information.
 * port 64h (write)  8042 command register.  Writing this port sets Bit 3
 *                   of the status register to 1 and the byte is treated
 *                   as a controller command.  Devices attached to the
 *                   8042 should be disabled before issuing commands that
 *                   return data since data in the output register will
 *                   be overwritten.
 * port 60h (read)   8042 output register (should only be read if Bit 0 of
 *                   status port is set to 1)
 * port 60h (write)  8042 data register.  Data should only be written if
 *                   Bit 1 of the status register is zero (register is empty).
 *                   When this port is written Bit 3 of the status register
 *                   is set to zero and the byte is treated as a data.  The
 *                   8042 uses this byte if it's expecting data for a previous
 *                   command, otherwise the data is written directly to the
 *                   keyboard.
 */
#define KBD_DATA        0x60    /* read/write keyboard data/commands */
#define KBDC_XT_CTL     0x61    /* read/write XT keyboard control */
#define KBDC_AT_CTL     0x64    /* read status, write AT kbdc cmds */

/*
 * XT Keyboard Control Register (port 61h read/write)
 */
#define KBDC_XT_ENABLE  0x40    /* enable KBCLK as an output */
#define KBDC_XT_CLEAR   0x80    /* clear XT keyboard data register */

/*
 * 8042 Status Register (port 64h read)
 */
#define KBSTS_DATAVL    0x01    /* output reg (60h) has data for system */
#define KBSTS_CMDBSY    0x02    /* input reg (60h/64h) has data for 8042 */
#define KBSTS_SYSFLG    0x04    /* system flag (0=cold, 1=warm boot) */
#define KBSTS_INCMD     0x08    /* data in input reg. is 1=command or 0=data */
#define KBSTS_UNLOCKED  0x10    /* keyboard key 1=unlocked or 0=locked */
#define KBSTS_AUX_DATAVL 0x20	/* output reg (60h) has aux data for system */
#define KBSTS_RTIMO     0x40    /* 1=receive timeout error */
#define KBSTS_PERR      0x80    /* receive parity error */

/*
 * 8042 Command Byte format
 */
#define KBCB_ENINTR	0x01    /* 1=enable `output register full' interrupt */
#define KBCB_AUXINTR	0x02    /* 1=enable `aux register full' interrupt */
#define KBCB_SYSFLG	0x04    /* system flag (0=cold, 1=warm boot) */
#define KBCB_IGNLOCK	0x08    /* ignore keyboard lock (inhibit override) */
#define KBCB_DISKBD	0x10    /* disable keyboard, drives clock line low */
#define KBCB_AUXDIS	0x20    /* disable aux device */
#define KBCB_TRANSL	0x40    /* 1=translate codes to Set 1, 0=pass Set 2 codes */

/*
 * 8042 Keyboard Test Reply Bytes
 */
#define KBTEST_OK       0x00    /* no error */
#define KBTEST_CLKLOW   0x01    /* keyboard clock line is stuck low */
#define KBTEST_CLKHIGH  0x02    /* keyboard clock line is stuck high */
#define KBTEST_DATLOW   0x03    /* keyboard data line is stuck low */
#define KBTEST_DATHIGH  0x04    /* keyboard data line is stuck high */

/*
 * 8042 Input Port Bits
 */
#define KBINP_JSET      0x01    /* manufacturer jumper set */
#define KBINP_RAM       0x10    /* motherboard RAM size */
#define KBINP_VMONO     0x40    /* monochrome video adapter */
#define KBINP_UNLOCKED  0x80    /* keyboard lock opened */

/*
 * 8042 Output Port Bits
 */
#define KBOUTP_SYSRST   0x01    /* system reset line inverted */
#define KBOUTP_A20      0x02    /* gate A20 */
#define KBOUTP_OUTFULL  0x10    /* output buffer full */
#define KBOUTP_INEMPTY  0x20    /* input buffer empty */
#define KBOUTP_KBDCLK   0x40    /* keyboard clock (output) */
#define KBOUTP_KBDDAT   0x80    /* keyboard data (output) */

/*
 * 8042 Commands Related to PC Systems  (Port 64h)
 */
#define KBDC_RCMD	0x20    /* Read 8042 Command Byte via port 60h */
#define KBDC_WCMD	0x60    /* Write 8042 Command Byte via port 60h */
#define KBDC_DISAUX	0xA7    /* Disable Auxiliary Interface */
#define KBDC_ENAUX	0xA8    /* Enable Auxiliary Interface */
#define KBDC_SELFTEST	0xAA    /* Self Test: result at port 60h, 55h=OK, kbd disabled */
#define KBDC_KBDTEST	0xAB    /* Keyboard Interface Test: see KBTEST_XX results */
#define KBDC_DISKBD	0xAD    /* Disable Keyboard Interface: drive clock low */
#define KBDC_ENKBD	0xAE    /* Enable Keyboard Interface: enable clock */
#define KBDC_RINP	0xC0    /* Read Input Port */
#define KBDC_ROUTP	0xD0    /* Read Output Port */
#define KBDC_WOUTP	0xD1    /* Write Output Port, only bit A20 is writable */
#define KBDC_WAUX	0xD4    /* Send byte to auxiliary device */

/*
 * Commands System Issues to Keyboard (via 8042 port 60h)
 */
#define KBDK_SETLED	0xED	/* Set/Reset mode indicators */
#define KBDK_SETLED_SCROLL_LOCK	0x01
#define KBDK_SETLED_NUM_LOCK	0x02
#define KBDK_SETLED_CAPS_LOCK	0x04

#define KBDK_ECHO	0xEE	/* Diagnostic echo */
#define KBDK_SCANSET	0xF0	/* Select/Read Alternate Scan Code Sets */
#define KBDK_READ_ID	0xF2	/* Read 2-byte keyboard ID (83AB) */
#define KBDK_TYPEMATIC	0xF3	/* Set typematic rate/delay */
#define KBDK_ENABLE	0xF4	/* Enable keyboard */
#define KBDK_DFLT_DIS	0xF5	/* Reset to default and disable scanning */
#define KBDK_DEFAULT	0xF6	/* Reset to default and continue scanning */

/* Only for scan set 3. */
#define KBDK_AUTO	0xF7	/* Set all keys to typematic */
#define KBDK_MAKE_BREAK	0xF8	/* Set all keys to make/break */
#define KBDK_MAKE	0xF9	/* Set all keys to make */
#define KBDK_AUTO_MAKE_BREAK	0xFA	/* Set all to typematic make/break */
#define KBDK_KEY_AUTO	0xFB	/* Set single key to typematic */
#define KBDK_KEY_MAKE_BREAK	0xFC	/* Set single key to make/break */
#define KBDK_KEY_MAKE	0xFD	/* Set single key to make */

#define KBDK_RESEND	0xFE	/* Resend */
#define KBDK_RESET	0xFF	/* Reset */

/*
 * Keyboard Responses to System (via 8042 port 60h)
 */
#define KBDR_OVERRUN23	0x00	/* Key detection error or overrun error */
#define KBDR_TEST_OK	0xAA	/* Keyboard self test was successful */
#define KBDR_ECHO	0xEE	/* Response to the Echo command */
#define KBDR_BREAK	0xF0	/* Break code prefix */
#define KBDR_ACK	0xFA	/* Acknowledge */
#define KBDR_TEST_FAIL	0xFC	/* Keyboard self test failed */
#define KBDR_RESEND	0xFE	/* Request to resend data */
#define KBDR_OVERRUN1	0xFF	/* Key Detection Error or Overrun Error */

#define kbdc_wait()     { while (inb (KBDC_AT_CTL) & KBSTS_CMDBSY) \
				continue; }
#define kbdc_flush()    { kbdc_wait (); \
			  while (inb (KBDC_AT_CTL) & KBSTS_DATAVL) \
				inb (KBD_DATA); }
#define kbdc_cmd(cmd)   { kbdc_wait (); \
			  outb (KBDC_AT_CTL, cmd); }
#define kbdc_out(cmd)   { kbdc_wait (); \
			  outb (KBD_DATA, cmd); }

/*
 * Read data from keyboard controller.
 * Return 1 on success, 0 when no data is available.
 */
int i8042_read (unsigned char *val);

/*
 * Send two-byte command to the device on a primary port.
 */
int i8042_kbd_command (int cmd, int param);

/*
 * Look to see if we can find a device on the primary port.
 */
int i8042_kbd_probe (void);

/*
 * Enable keyboard device and interrupt.
 */
void i8042_kbd_enable (void);

/*
 * Write to auxiliary device.
 */
void i8042_aux_write (int val);

/*
 * Look to see if we can find a device on the aux port.
 */
int i8042_aux_probe (void);

/*
 * Enable auxiliary device and interrupt.
 */
void i8042_aux_enable (void);
