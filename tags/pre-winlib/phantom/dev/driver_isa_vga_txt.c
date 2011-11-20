#ifdef ARCH_ia32

#include <kernel/vm.h>

#include <kernel/drivers.h>

#include <ia32/pio.h>
#include <phantom_libc.h>

#include <hal.h>

#define ROWS 25
#define COLS 80


#define MDA_PORT 0x3B4
#define CGA_PORT 0x3D4

#define MDA_MEM 0xB0000
#define CGA_MEM 0xB8000


#define vga_ctl vga_port
#define vga_data (vga_port+1)

static int vga_port = 0;
static u_int16_t *mem = 0;

static int screen_pos = 0;

static int inited = 0;

static void doputc(int c);
//static void doputs(char *s);
static void init();



static int mem_test(int paddr, u_char data, u_char mask)
{
    char *addr = (char *)phystokv(paddr);
    int	temp;

    data = data & mask;

    *addr = data;

    temp = (*addr) & mask;

    return (temp == data);
}


static int
vga_detect(int addr)
{
    static u_char	testbyte[18] = {
        0x55,			/* alternating zeros */
        0xaa,			/* alternating ones */
        0xfe, 0xfd, 0xfb, 0xf7,
        0xef, 0xdf, 0xbf, 0x7f,	/* walking zero */
        0x01, 0x02, 0x04, 0x08,
        0x10, 0x20, 0x40, 0x80	/* walking one */
    };
    int		i, status;

    status = 1;				/* assume success */

    for (i = 0; i < 18 && status; i++)
        if (!mem_test(addr, testbyte[i], 0xff))
        {
            status = 0;
            break;
        }

    return status;
}


static int try_port( int port )
{
    if( port == MDA_PORT )
    {
        if( !vga_detect(MDA_MEM) )
            return 0;
        mem = (void *)phystokv(MDA_MEM);
    }

    if( port == CGA_PORT )
    {
        if( !vga_detect(CGA_MEM) )
            return 0;
        mem = (void *)phystokv(CGA_MEM);
    }

    vga_port = port;

    init();
    return 1;
}


// TODO this is to be moved to driver_map.c and be kept in driver tables
static int seq_number = 0;

phantom_device_t * driver_isa_vga_probe( int port, int irq, int stage )
{
    (void) irq;
    (void) stage;

    if(seq_number)
    {
        return 0; // just one instance!
    }

    if(inited && (vga_port != port) )
        return 0; // We inited and found other port

    if(!inited)
    {
        if( try_port(port) == 0 )
            return 0;
    }

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = "vga-console";
    dev->seq_number = seq_number++;

    //doputs("Test VGA console output\n");

    return dev;
}


int driver_isa_vga_putc(int c)
{
    if(vga_port == 0)
    {
        // Somebody called us too early - init from here
        //driver_isa_vga_probe( CGA_PORT, 0, 0 );
        try_port( CGA_PORT );
    }

    if(vga_port == 0)
    {
        // Still no success? Try monochrome
        try_port(  MDA_PORT );
    }

    // Sorry :(
    if(vga_port == 0)
        return c;

    doputc(c);
    return c;
}

#define LASTLINE (((ROWS-1)*COLS))

static void clr(u_int16_t start, u_int16_t cnt)
{
    volatile u_int16_t *p = mem+start;
    while (cnt--)
        *p++ = (u_int16_t) 0x0700;
}


static void scroll()
{
	memcpy( mem, mem + COLS, COLS*ROWS*2);
	clr((ROWS - 1) * COLS, COLS);
}


static void tab()
{
    do { doputc(' '); } while( screen_pos & 0x7 );
}

static void bs()
{
    if(screen_pos > 0) screen_pos--;
}

static void cr()
{
    screen_pos -= (screen_pos % COLS);
}

static void lf()
{
    if(screen_pos < LASTLINE)
        screen_pos += COLS;
    else
        scroll();
    cr();
}







static void init()
{
    if(inited) return;

    int pos = 0;
    // get curs pos

    outb_p( vga_ctl, 0xE ); // get hi
    pos = (inb_p( vga_data ) & 0xFFu) << 8;
    outb_p( vga_ctl, 0xF ); // get lo
    pos |= inb_p( vga_data ) & 0xFFu;

    if( pos < LASTLINE )
        screen_pos = pos;
    else
        screen_pos = LASTLINE;
    //cr();
    lf();

    inited = 1;
}

// TODO mutex
static void doputc(int c)
{
    switch(c)
    {
    case '\n': lf(); break;
    case '\r': cr(); break;
    case '\t': tab(); break;
    case '\b': bs(); break;

    default:
        if( screen_pos >= ROWS*COLS )
        {
            scroll();
            screen_pos = LASTLINE;
        }

        mem[screen_pos++] = 0x0700 | (unsigned char)c;

    }
}
/*
static void doputs(char *s)
{
    while( *s )
        doputc(*s++);
}
*/
// ---------------------------------
// Connect old stuff here to our driver

#if 0
int
direct_cons_putchar(int c)
{
    driver_isa_vga_putc(c);
    return 0;
}
#endif

#endif // ARCH_ia32
