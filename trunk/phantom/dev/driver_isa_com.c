#define DEBUG_MSG_PREFIX "com"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/drivers.h>

#include <i386/pio.h>
#include <phantom_libc.h>

#include <hal.h>

#include <x86/comreg.h>

/*

Format of BIOS Data Segment at segment 40h:
		{items in curly braces not documented by IBM}
Offset	Size	Description
 00h	WORD	Base I/O address of 1st serial I/O port, zero if none
 02h	WORD	Base I/O address of 2nd serial I/O port, zero if none
 04h	WORD	Base I/O address of 3rd serial I/O port, zero if none
 06h	WORD	Base I/O address of 4th serial I/O port, zero if none
		    Note: Above fields filled in turn by POST as it finds serial
		    ports. POST never leaves gaps. DOS and BIOS serial device
		    numbers may be redefined by re-assigning these fields.
 08h	WORD	Base I/O address of 1st parallel I/O port, zero if none
 0Ah	WORD	Base I/O address of 2nd parallel I/O port, zero if none
 0Ch	WORD	Base I/O address of 3rd parallel I/O port, zero if none
 0Eh	WORD	[non-PS] Base I/O address of 4th parallel I/O port, zero if none


 */

typedef struct
{
    int 	baudRate;
    int 	dataBits;
    int 	stopBits;
    int 	parity;
} com_port_t;

static void com_setbaud(struct phantom_device *dev, int speed);



static int
com_detect(int unit, int addr)
{
    int     oldctl, oldmsb;
    char    *type = "8250";
    int     i;

    oldctl = inb(LINE_CTL(addr));	 /* Save old value of LINE_CTL */
    oldmsb = inb(BAUD_MSB(addr));	 /* Save old value of BAUD_MSB */
    outb(LINE_CTL(addr), 0);	 /* Select INTR_ENAB */
    outb(BAUD_MSB(addr), 0);
    if (inb(BAUD_MSB(addr)) != 0)
    {
        outb(LINE_CTL(addr), oldctl);
        outb(BAUD_MSB(addr), oldmsb);
        return 0;
    }
    outb(LINE_CTL(addr), iDLAB);	 /* Select BAUD_MSB */
    outb(BAUD_MSB(addr), 255);
    if (inb(BAUD_MSB(addr)) != 255)
    {
        outb(LINE_CTL(addr), oldctl);
        outb(BAUD_MSB(addr), oldmsb);
        return 0;
    }
    outb(LINE_CTL(addr), 0);	 /* Select INTR_ENAB */
    if (inb(BAUD_MSB(addr)) != 0)	 /* Check that it has kept it's value*/
    {
        outb(LINE_CTL(addr), oldctl);
        outb(BAUD_MSB(addr), oldmsb);
        return 0;
    }

    /* Com port found, now check what chip it has */

    for(i = 0; i < 256; i++)	 /* Is there Scratch register */
    {
        outb(SCR(addr), i);
        if (inb(SCR(addr)) != i)
            break;
    }
    if (i == 256)
    {				 /* Yes == 450 or 460 */
        outb(SCR(addr), 0);
        type = "82450 or 16450";
        outb(FIFO_CTL(addr), iFIFOENA | iFIFO14CH);	 /* Enable fifo */
        if ((inb(FIFO_CTL(addr)) & iFIFO14CH) != 0)
        {				 /* Was it successfull */
            /* if both bits are not set then broken xx550 */
            if ((inb(FIFO_CTL(addr)) & iFIFO14CH) == iFIFO14CH)
            {
                type = "82550 or 16550";
                //comfifo[unit] = 1;
            }
            else
            {
                type = "82550 or 16550 with non-working FIFO";
            }
            outb(INTR_ID(addr), 0x00); /* Disable fifos */
        }
    }
    SHOW_INFO( 0, "com%d: %s chip", unit, type);
    return 1;

}

static int com_start(phantom_device_t *dev); // Start device (begin using)
static int com_stop(phantom_device_t *dev);  // Stop device

// Access from kernel - can block!
static int com_read(struct phantom_device *dev, void *buf, int len);
static int com_write(struct phantom_device *dev, const void *buf, int len);


static void com_interrupt( void *_dev );

// TODO this is to be moved to driver_map.c and be kept in driver tables
static int seq_number = 0;

phantom_device_t * driver_isa_com_probe( int port, int irq, int stage )
{
    (void) stage;
    if( !com_detect( seq_number, port) )
        return 0;


    phantom_device_t * dev = calloc(sizeof(phantom_device_t), 1);

    dev->iobase = port;
    dev->irq = irq;

    dev->name = "COM";
    dev->seq_number = seq_number++;


    dev->dops.start = com_start;
    dev->dops.stop = com_stop;

    dev->dops.read = com_read;
    dev->dops.write = com_write;

    if( hal_irq_alloc( irq, &com_interrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", irq );
        free(dev);
        return 0;
    }


    com_port_t *cp = calloc( sizeof(com_port_t), 1 );
    dev->drv_private = cp;

    cp->baudRate = 9600;
    cp->stopBits = 1;
    cp->dataBits = 8;
    cp->parity = 0;

    com_setbaud(dev, cp->baudRate);

    return dev;
}




static void com_interrupt( void *_dev )
{
    phantom_device_t * dev = _dev;

    int unit = dev->seq_number;
    int addr = dev->iobase;

    SHOW_FLOW( 9, "com port %d interrupt", unit );

    //register struct tty	*tp = &com_tty[unit];
    //static char 		comoverrun = 0;
    char			c, line, intr_id;
    //int			modem_stat;
    int				line_stat;

    while (! ((intr_id=(inb(INTR_ID(addr))&MASKi)) & 1))
    {
        switch (intr_id) {
        case MODi:
            /* modem change */
            //int ms =
            inb(MODEM_STAT(addr));
            //commodem_intr(unit, ms));
            break;

        case TRAi:
            //comtimer_state[unit] = 0;
            //tp->t_state &= ~(TS_BUSY|TS_FLUSH);
            //tt_write_wakeup(tp);
            //(void) comstart(tp);
            break;

        case RECi:
        case CTIi:         /* Character timeout indication */
            //if (tp->t_state&TS_ISOPEN)
            {
                while ((line = inb(LINE_STAT(addr))) & iDR)
                {
                    c = inb(TXRX(addr));
                    //ttyinput(c, tp);
                }
            } //else
            {
                //tt_open_wakeup(tp);
            }
            break;

        case LINi:
            line_stat = inb(LINE_STAT(addr));
#if 0
            if ((line_stat & iPE) &&
                ((tp->t_flags&(EVENP|ODDP)) == EVENP ||
                 (tp->t_flags&(EVENP|ODDP)) == ODDP)) {
                /* parity error */;
            } else 	if (line&iOR && !comoverrun) {
                printf("com%d: overrun\n", unit);
                comoverrun = 1;
            } else if (line_stat & (iFE | iBRKINTR)) {
                /* framing error or break */
                ttyinput(tp->t_breakc, tp);
            }
#endif
            break;
        }
    }
}





// Start device (begin using)
static int com_start(phantom_device_t *dev)
{
    int addr = dev->iobase;
    int unit = dev->seq_number;

    //take_dev_irq(dev);
    SHOW_INFO( 1, "start com port = %x, unit %d", addr, unit);

    //cominfo[unit] = dev;
    /*	comcarrier[unit] = addr->flags;*/
    //commodem[unit] = 0;

    outb(INTR_ENAB(addr), 0);
    outb(MODEM_CTL(addr), 0);
    while( !(inb(INTR_ID(addr))&1) )
    {
        (void) inb(LINE_STAT (addr)); 	/* reset overrun error etc */
        (void) inb(TXRX      (addr)); 	/* reset data-ready */
        (void) inb(MODEM_STAT(addr)); 	/* reset modem status reg */
    }

    return 0;
}

// Stop device
static int com_stop(phantom_device_t *dev)
{
    int addr = dev->iobase;

    outb(INTR_ENAB(addr), 0);
    outb(MODEM_CTL(addr), 0);
    return 0;
}




static int com_read(struct phantom_device *dev, void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;
    return -EIO;
}

static int com_write(struct phantom_device *dev, const void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;
    return -EIO;
}




static void com_setbaud(struct phantom_device *dev, int speed)
{
#if 0
    int addr = dev->iobase;
    //int unit = dev->seq_number;

	// TODO: com speed setup
    outb(LINE_CTL(addr), iDLAB);
    outb(BAUD_LSB(addr), divisorreg[speed] & 0xff);
    outb(BAUD_MSB(addr), divisorreg[speed] >> 8);
#endif
}





#if 0


/*
 *
 * Device Attach's are called during kernel boot, but only if the matching
 * device Probe returned a 1.
 * No return value, despite the "int" signature.
 *
 */
void comattach(dev)
struct bus_device *dev;
{
	u_char	unit = dev->unit;
	u_short	addr = dev->address;

	take_dev_irq(dev);
	printf(", port = %x, spl = %d, pic = %d. (DOS COM%d)",
		dev->address, dev->sysdep, dev->sysdep1, unit+1);

	cominfo[unit] = dev;
/*	comcarrier[unit] = addr->flags;*/
	commodem[unit] = 0;

	outb(INTR_ENAB(addr), 0);
	outb(MODEM_CTL(addr), 0);
	while (!(inb(INTR_ID(addr))&1)) {
		(void) inb(LINE_STAT (addr)); 	/* reset overrun error etc */
		(void) inb(TXRX      (addr)); 	/* reset data-ready */
		(void) inb(MODEM_STAT(addr)); 	/* reset modem status reg */
	}
}

io_return_t comopen(
	int dev,
	int flag,
	io_req_t ior)
{
	int		unit = minor(dev);
	u_short		addr;
	struct bus_device	*isai;
	struct tty	*tp;
	spl_t		s;
	io_return_t	result;

	if (unit >= NCOM || (isai = cominfo[unit]) == 0 || isai->alive == 0)
		return(ENXIO);
	tp = &com_tty[unit];

	if ((tp->t_state & (TS_ISOPEN|TS_WOPEN)) == 0) {
		ttychars(tp);
		tp->t_addr = (char *)isai->address;
		tp->t_dev = dev;
		tp->t_oproc = comstart;
		tp->t_stop = comstop;
		tp->t_mctl = commctl;
		tp->t_getstat = comgetstat;
		tp->t_setstat = comsetstat;
#ifndef	PORTSELECTOR
		if (tp->t_ispeed == 0) {
#else
			tp->t_state |= TS_HUPCLS;
#endif	PORTSELECTOR
			tp->t_ispeed = ISPEED;
			tp->t_ospeed = ISPEED;
			tp->t_flags = IFLAGS;
			tp->t_state &= ~TS_BUSY;
#ifndef	PORTSELECTOR
		}
#endif	PORTSELECTOR
	}
/*rvb	tp->t_state |= TS_WOPEN; */
	if ((tp->t_state & TS_ISOPEN) == 0)
		comparam(unit);
	addr = (int)tp->t_addr;

	s = spltty();
	if (!comcarrier[unit])	/* not originating */
		tp->t_state |= TS_CARR_ON;
	else {
		int modem_stat = inb(MODEM_STAT(addr));
		if (modem_stat & iRLSD)
			tp->t_state |= TS_CARR_ON;
		else
			tp->t_state &= ~TS_CARR_ON;
		fix_modem_state(unit, modem_stat);
	}
	splx(s);

	result = char_open(dev, tp, flag, ior);

	if (!comtimer_active) {
		comtimer_active = TRUE;
		comtimer();
	}

	s = spltty();
	while(!(inb(INTR_ID(addr))&1)) { /* while pending interrupts */
		(void) inb(LINE_STAT (addr)); /* reset overrun error  */
		(void) inb(TXRX      (addr)); /* reset data-ready	    */
		(void) inb(MODEM_STAT(addr)); /* reset modem status   */
	}
	splx(s);
	return result;
}

io_return_t comclose(dev, flag)
int dev;
int flag;
{
	struct tty	*tp = &com_tty[minor(dev)];
	u_short		addr = (int)tp->t_addr;

	ttyclose(tp);
	if (tp->t_state&TS_HUPCLS || (tp->t_state&TS_ISOPEN)==0) {
		outb(INTR_ENAB(addr), 0);
		outb(MODEM_CTL(addr), 0);
		tp->t_state &= ~TS_BUSY;
		commodem[minor(dev)] = 0;
	}
	return 0;
}

io_return_t comread(dev, ior)
int	dev;
io_req_t ior;
{
	return char_read(&com_tty[minor(dev)], ior);
}

io_return_t comwrite(dev, ior)
int	dev;
io_req_t ior;
{
	return char_write(&com_tty[minor(dev)], ior);
}

io_return_t comportdeath(dev, port)
dev_t		dev;
mach_port_t	port;
{
	return (tty_portdeath(&com_tty[minor(dev)], port));
}

io_return_t
comgetstat(dev, flavor, data, count)
dev_t		dev;
int		flavor;
int		*data;		/* pointer to OUT array */
unsigned int	*count;		/* out */
{
	io_return_t	result = D_SUCCESS;
	int		unit = minor(dev);

	switch (flavor) {
	case TTY_MODEM:
		fix_modem_state(unit, inb(MODEM_STAT(cominfo[unit]->address)));
		*data = commodem[unit];
		*count = 1;
		break;
	default:
		result = tty_get_status(&com_tty[unit], flavor, data, count);
		break;
	}
	return (result);
}

io_return_t
comsetstat(dev, flavor, data, count)
dev_t		dev;
int		flavor;
int *		data;
unsigned int	count;
{
	io_return_t	result = D_SUCCESS;
	int 		unit = minor(dev);
	struct tty	*tp = &com_tty[unit];

	switch (flavor) {
	case TTY_SET_BREAK:
		commctl(tp, TM_BRK, DMBIS);
		break;
	case TTY_CLEAR_BREAK:
		commctl(tp, TM_BRK, DMBIC);
		break;
	case TTY_MODEM:
		commctl(tp, *data, DMSET);
		break;
	default:
		result = tty_set_status(&com_tty[unit], flavor, data, count);
		if (result == D_SUCCESS && flavor == TTY_STATUS)
			comparam(unit);
		return (result);
	}
	return (D_SUCCESS);
}

comintr(unit)
int unit;
{
	register struct tty	*tp = &com_tty[unit];
	u_short			addr = cominfo[unit]->address;
	static char 		comoverrun = 0;
	char			c, line, intr_id;
	int			modem_stat, line_stat;

	while (! ((intr_id=(inb(INTR_ID(addr))&MASKi)) & 1))
	    switch (intr_id) {
		case MODi:
		    /* modem change */
			commodem_intr(unit, inb(MODEM_STAT(addr)));
			break;

		case TRAi:
			comtimer_state[unit] = 0;
			tp->t_state &= ~(TS_BUSY|TS_FLUSH);
			tt_write_wakeup(tp);
			(void) comstart(tp);
			break;
		case RECi:
		case CTIi:         /* Character timeout indication */
			if (tp->t_state&TS_ISOPEN) {
				while ((line = inb(LINE_STAT(addr))) & iDR) {
					c = inb(TXRX(addr));
					ttyinput(c, tp);
				}
			} else
				tt_open_wakeup(tp);
			break;
		case LINi:
			line_stat = inb(LINE_STAT(addr));

			if ((line_stat & iPE) &&
			    ((tp->t_flags&(EVENP|ODDP)) == EVENP ||
			     (tp->t_flags&(EVENP|ODDP)) == ODDP)) {
				/* parity error */;
			} else 	if (line&iOR && !comoverrun) {
				printf("com%d: overrun\n", unit);
				comoverrun = 1;
			} else if (line_stat & (iFE | iBRKINTR)) {
				/* framing error or break */
				ttyinput(tp->t_breakc, tp);
			}
			break;
		}
}

static void
comparam(unit)
register int unit;
{
	struct tty	*tp = &com_tty[unit];
	u_short		addr = (int)tp->t_addr;
	spl_t		s = spltty();
	int		mode;

        if (tp->t_ispeed == B0) {
		tp->t_state |= TS_HUPCLS;
		outb(MODEM_CTL(addr), iOUT2);
		commodem[unit] = 0;
		splx(s);
		return;
	}

	/* Do input buffering */
	if (tp->t_ispeed >= B300)
		tp->t_state |= TS_MIN;

	outb(LINE_CTL(addr), iDLAB);
	outb(BAUD_LSB(addr), divisorreg[tp->t_ispeed] & 0xff);
	outb(BAUD_MSB(addr), divisorreg[tp->t_ispeed] >> 8);

	if (tp->t_flags & (RAW|LITOUT|PASS8))
		mode = i8BITS;
	else
		mode = i7BITS | iPEN;
	if (tp->t_flags & EVENP)
		mode |= iEPS;
	if (tp->t_ispeed == B110)
		/*
		 * 110 baud uses two stop bits -
		 * all other speeds use one
		 */
		mode |= iSTB;

	outb(LINE_CTL(addr), mode);

	outb(INTR_ENAB(addr), iTX_ENAB|iRX_ENAB|iMODEM_ENAB|iERROR_ENAB);
	if (comfifo[unit])
		outb(FIFO_CTL(addr), iFIFOENA|iFIFO14CH);
	outb(MODEM_CTL(addr), iDTR|iRTS|iOUT2);
	commodem[unit] |= (TM_DTR|TM_RTS);
        splx(s);
}

computc(char ch, int unit)
{
	u_short addr = (u_short)(cominfo[unit]->address);

	/* send the char */
	outb(addr, ch);

	/* wait for transmitter to empty */
	while((inb(LINE_STAT(addr)) & iTHRE) == 0);
}

comgetc(int unit)
{
	u_short		addr = (u_short)(cominfo[unit]->address);
	spl_t		s = spltty();
	natural_t	c;

	while((inb(LINE_STAT(addr)) & iDR) == 0) ;

	c = inb(TXRX(addr));
	splx(s);
	return c;
}

comparm(int unit, int baud, int intr, int mode, int modem)
{
	u_short addr = (u_short)(cominfo[unit]->address);
	spl_t	s = spltty();

	if (unit != 0 && unit != 1) {
		printf("comparm(unit, baud, mode, intr, modem)\n");
		splx(s);
		return;
	}
	outb(LINE_CTL(addr), iDLAB);
	outb(BAUD_LSB(addr), divisorreg[baud] & 0xff);
	outb(BAUD_MSB(addr), divisorreg[baud] >> 8);
	outb(LINE_CTL(addr), mode);
	outb(INTR_ENAB(addr), intr);
	outb(MODEM_CTL(addr), modem);
	splx(s);
}

int comst_1, comst_2, comst_3, comst_4, comst_5 = 14;

int
comstart(tp)
struct tty *tp;
{
	char nch;
	int i;

	if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP|TS_BUSY)) {
comst_1++;
		return(0);
	}
	if ((!queue_empty(&tp->t_delayed_write)) &&
	    (tp->t_outq.c_cc <= TTLOWAT(tp))) {
comst_2++;
		tt_write_wakeup(tp);
	}
	if (!tp->t_outq.c_cc) {
comst_3++;
		return(0);
	}

#if 0
	i = (comfifo[minor(tp->t_dev)]) ? /*14*/comst_5 : 1;

	tp->t_state |= TS_BUSY;
	while (i-- > 0) {
		nch = getc(&tp->t_outq);
		if (nch == -1) break;
		if ((nch & 0200) && ((tp->t_flags & LITOUT) == 0)) {
		    timeout(ttrstrt, (char *)tp, (nch & 0x7f) + 6);
		    tp->t_state |= TS_TIMEOUT;
comst_4++;
		    return(0);
		}
		outb(TXRX((int)tp->t_addr), nch);
	}
#else
	nch = getc(&tp->t_outq);
	if ((nch & 0200) && ((tp->t_flags & LITOUT) == 0)) {
	    timeout(ttrstrt, (char *)tp, (nch & 0x7f) + 6);
	    tp->t_state |= TS_TIMEOUT;
comst_4++;
	    return(0);
	}
	outb(TXRX((int)tp->t_addr), nch);
	tp->t_state |= TS_BUSY;
#endif
	return(0);
}

/* Check for stuck xmitters */
int comtimer_interval = 5;

comtimer()
{
	spl_t	s = spltty();
	struct tty *tp = com_tty;
	int i, nch;

	for (i = 0; i < NCOM; i++, tp++) {
		if ((tp->t_state & TS_ISOPEN) == 0)
			continue;
		if (!tp->t_outq.c_cc)
			continue;
		if (++comtimer_state[i] < 2)
			continue;
		/* Its stuck */
db_printf("Tty %x was stuck\n", tp);
		nch = getc(&tp->t_outq);
		outb(TXRX((int)tp->t_addr), nch);
	}

	splx(s);
	timeout(comtimer, 0, comtimer_interval*hz);
}

/*
 * Set receive modem state from modem status register.
 */
fix_modem_state(unit, modem_stat)
int	unit, modem_stat;
{
	int	stat = 0;

	if (modem_stat & iCTS)
	    stat |= TM_CTS;	/* clear to send */
	if (modem_stat & iDSR)
	    stat |= TM_DSR;	/* data set ready */
	if (modem_stat & iRI)
	    stat |= TM_RNG;	/* ring indicator */
	if (modem_stat & iRLSD)
	    stat |= TM_CAR;	/* carrier? */

	commodem[unit] = (commodem[unit] & ~(TM_CTS|TM_DSR|TM_RNG|TM_CAR))
				| stat;
}

/*
 * Modem change (input signals)
 */
commodem_intr(
	int	unit,
	int	stat)
{
	int	changed;

	changed = commodem[unit];
	fix_modem_state(unit, stat);
	stat = commodem[unit];

	/* Assumption: if the other party can handle
	   modem signals then it should handle all
	   the necessary ones. Else fix the cable. */

	changed ^= stat;	/* what changed ? */

	if (changed & TM_CTS)
		tty_cts( &com_tty[unit], stat & TM_CTS );

	if (changed & TM_CAR)
		ttymodem( &com_tty[unit], stat & TM_CAR );

}

/*
 * Set/get modem bits
 */
commctl(
	register struct tty	*tp,
	int	bits,
	int	how)
{
	spl_t		s;
	int		unit;
	vm_offset_t	dev_addr;
	register int	b;

	unit = minor(tp->t_dev);

	if (bits == TM_HUP) { /* close line (internal) */
		bits = TM_DTR | TM_RTS;
		how = DMBIC;
	}

	if (how == DMGET) return commodem[unit];

	dev_addr = cominfo[unit]->address;

	s = spltty();

	switch (how) {
	case DMSET:
		b = bits; break;
	case DMBIS:
		b = commodem[unit] | bits; break;
	case DMBIC:
		b = commodem[unit] & ~bits; break;
	}
	commodem[unit] = b;

	if (bits & TM_BRK) {
		if (b & TM_BRK) {
			outb(LINE_CTL(dev_addr), inb(LINE_CTL(dev_addr)) | iSETBREAK);
		} else {
			outb(LINE_CTL(dev_addr), inb(LINE_CTL(dev_addr)) & ~iSETBREAK);
		}
	}

#if 0
	/* do I need to do something on this ? */
	if (bits & TM_LE) {	/* line enable */
	}
#endif
#if 0
	/* Unsupported */
	if (bits & TM_ST) {	/* secondary transmit */
	}
	if (bits & TM_SR) {	/* secondary receive */
	}
#endif
	if (bits & (TM_DTR|TM_RTS)) {	/* data terminal ready, request to send */
		how = iOUT2;
		if (b & TM_DTR) how |= iDTR;
		if (b & TM_RTS) how |= iRTS;
		outb(MODEM_CTL(dev_addr), how);
	}

	splx(s);

	/* the rest are inputs */
	return commodem[unit];
}

comstop(tp, flags)
register struct tty *tp;
int	flags;
{
	if ((tp->t_state & TS_BUSY) && (tp->t_state & TS_TTSTOP) == 0)
	    tp->t_state |= TS_FLUSH;
}

/*
 *
 * Code to be called from debugger.
 *
 */
void compr_addr(addr)
{
	/* The two line_stat prints may show different values, since
	*  touching some of the registers constitutes changing them.
	*/
	printf("LINE_STAT(%x) %x\n",
		LINE_STAT(addr), inb(LINE_STAT(addr)));

	printf("TXRX(%x) %x, INTR_ENAB(%x) %x, INTR_ID(%x) %x, LINE_CTL(%x) %x,\n\
MODEM_CTL(%x) %x, LINE_STAT(%x) %x, MODEM_STAT(%x) %x\n",
	TXRX(addr), 	 inb(TXRX(addr)),
	INTR_ENAB(addr), inb(INTR_ENAB(addr)),
	INTR_ID(addr), 	 inb(INTR_ID(addr)),
	LINE_CTL(addr),  inb(LINE_CTL(addr)),
	MODEM_CTL(addr), inb(MODEM_CTL(addr)),
	LINE_STAT(addr), inb(LINE_STAT(addr)),
	MODEM_STAT(addr),inb(MODEM_STAT(addr)));
}

int compr(unit)
{
	compr_addr(cominfo[unit]->address);
	return(0);
}

int rcline = -1;
int rcbaud = B9600;
comrc_put(ch)
char	ch;
{
    static int		opened = 0;
    u_short		addr;

    if (!(int)cominfo[rcline]) return;

    addr = (u_short)(cominfo[rcline]->address);

    if (!opened) {	/* whap down chip config for rconsole */
	int	mode = i7BITS | iPEN;

	opened = 1;
	outb(LINE_CTL(addr), iDLAB);
	outb(BAUD_LSB(addr), divisorreg[rcbaud] & 0xff);
	outb(BAUD_MSB(addr), divisorreg[rcbaud] >>8);
	outb(LINE_CTL(addr), mode);
	outb(INTR_ENAB(addr), 0);
	outb(MODEM_CTL(addr), iDTR|iRTS|iOUT2);
    }

    /* wait for transmitter to empty */
    while((inb(LINE_STAT(addr)) & iTHRE) == 0);
    /* send the char */
    outb(addr, ch);
}

comrc_may(wait)
boolean_t	wait;
{
    unsigned char	c;
    u_short		addr;

    if (!(int)cominfo[rcline]) return;

    addr = (u_short)(cominfo[rcline]->address);

    if ((inb(LINE_STAT(addr)) & iDR) == 0) {
	    return (-1);
    }

    c = inb(TXRX(addr));
    return (c & 0x7f);
}

comrc_get(wait)
boolean_t	wait;
{
    unsigned char	c;
    u_short		addr;


    addr = (u_short)(cominfo[rcline]->address);

    while((inb(LINE_STAT(addr)) & iDR) == 0) {
	if (!wait)
	    return (-1);
    }

    c = inb(TXRX(addr));
    return (c & 0x7f);
}


#endif
