#define DEBUG_MSG_PREFIX "mouse"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <i386/pio.h>
#include <phantom_libc.h>

#include "driver_map.h"
#include "device.h"
#include "hal.h"

#include "drv_video_screen.h"

#include <event.h>


#define PS2_DATA_ADDR 	0x60
#define PS2_CTRL_ADDR 	0x64



// control words
#define PS2_CTRL_WRITE_CMD       0x60
#define PS2_CTRL_WRITE_AUX       0xD4

// data words
#define PS2_CMD_DEV_INIT         0x43
#define PS2_CMD_ENABLE_MOUSE     0xF4
#define PS2_CMD_DISABLE_MOUSE    0xF5
#define PS2_CMD_RESET_MOUSE      0xFF

#define PS2_RES_ACK              0xFA
#define PS2_RES_RESEND           0xFE



void phantom_dev_ps2_int_handler( void * arg );
static int phantom_dev_ps2_do_init( void );

// PS2 simplistic driver
// Returns not null if failed
/*
 int phantom_dev_ps2_init()
 {
 if( phantom_dev_ps2_do_init() )
 return 1;
 #define PS2_INTERRUPT   12

 hal_irq_alloc( PS2_INTERRUPT, phantom_dev_ps2_int_handler, 0, HAL_IRQ_SHAREABLE );

 printf("PS/2 mouse microdriver installed\n");

 return 0;
 }
 */

int phantom_dev_ps2_stat_interrupts = 0;
unsigned char phantom_dev_ps2_state_buttons = 0;
int phantom_dev_ps2_state_xpos = 0;
int phantom_dev_ps2_state_ypos = 0;
static int xsign, ysign, xval, yval;


// TODO 9th bit

void phantom_dev_ps2_int_handler( void *arg )
{
    static int inbytepos = 0;

    signed char mousedata = inb( PS2_DATA_ADDR );

    SHOW_FLOW( 10 ,"%2X ", mousedata & 0xFFu );

    switch(inbytepos)
    {
    case 0:

        // first byte has one in this pos
        if(1 && ! (0x8 & mousedata) )
        {
            inbytepos = -1;
            break;
        }

        phantom_dev_ps2_state_buttons = 0x7 & mousedata;
        xsign = 0x10 & mousedata;
        ysign = 0x20 & mousedata;
        break;

    case 1:     xval = mousedata; break;
    case 2:     yval = mousedata; break;
    case 3:     break;
    case 4:     break;
    }

    inbytepos++;
    inbytepos %= 3;
    //inbytepos %= 4;

    if(inbytepos == 0)
    {
        // TODO 32bit int dependent
        //if(xsign) xval = 0xFFFFFF00 || xval;
        //if(ysign) yval = 0xFFFFFF00 || yval;
        phantom_dev_ps2_state_xpos += xval;
        phantom_dev_ps2_state_ypos += yval;

        if( phantom_dev_ps2_state_xpos < 0 ) phantom_dev_ps2_state_xpos = 0;
        if( phantom_dev_ps2_state_ypos < 0 ) phantom_dev_ps2_state_ypos = 0;

        if( phantom_dev_ps2_state_xpos > video_drv->xsize ) phantom_dev_ps2_state_xpos = video_drv->xsize;
        if( phantom_dev_ps2_state_ypos > video_drv->ysize ) phantom_dev_ps2_state_ypos = video_drv->ysize;


        //printf("ms %d %d %x\n", phantom_dev_ps2_state_xpos, phantom_dev_ps2_state_ypos, phantom_dev_ps2_state_buttons );

        if(NULL != video_drv)
        {
            video_drv->mouse_x = phantom_dev_ps2_state_xpos;
            video_drv->mouse_y = phantom_dev_ps2_state_ypos;
            if(video_drv->redraw_mouse_cursor != NULL)
                video_drv->redraw_mouse_cursor();

            event_q_put_mouse(
                              phantom_dev_ps2_state_xpos,
                              phantom_dev_ps2_state_ypos,
                              phantom_dev_ps2_state_buttons
                             );

        }
    }

    // wakeup everyone who waits fpr data
}


unsigned char phantom_dev_ps2_get_data()
{
    while((inb(PS2_CTRL_ADDR) & 0x1) == 0)
        ;
    return inb( PS2_DATA_ADDR );
}

static void wait_write_data()
{
    while(inb(PS2_CTRL_ADDR) & 0x2);
}

static void wait_write_ctrl()
{
    while(inb(PS2_CTRL_ADDR) & 0x3);
}


void phantom_dev_ps2_send_cmd(unsigned char cmd)
{
    wait_write_ctrl();    	outb(PS2_CTRL_ADDR, PS2_CTRL_WRITE_CMD);
    wait_write_data();    	outb(PS2_DATA_ADDR, cmd);
}


void phantom_dev_ps2_send_aux(unsigned char aux)
{
    wait_write_ctrl();   	outb(PS2_CTRL_ADDR, PS2_CTRL_WRITE_AUX);
    wait_write_data();   	outb(PS2_DATA_ADDR, aux);

}

static int phantom_dev_ps2_do_init( void )
{
    // TODO check if we have ISA at all :)

    int tries = 10000;
    // Purge buffer
    while( tries-- > 0 && inb( PS2_CTRL_ADDR ) & 0x01 )
        inb( PS2_DATA_ADDR );

    if( tries <= 0 ) goto notfound;

    phantom_dev_ps2_send_cmd(PS2_CMD_DEV_INIT);
    phantom_dev_ps2_send_aux(PS2_CMD_ENABLE_MOUSE);

    // Now check for ACK
    if(phantom_dev_ps2_get_data() != PS2_RES_ACK)
    {
    notfound:
        SHOW_ERROR0( 1, "PS/2 mouse not found\n" );
        return 1;
    }

    return 0;
}

static int seq_number = 0;

phantom_device_t * driver_isa_ps2m_probe( int port, int irq, int stage )
{
    if( seq_number || phantom_dev_ps2_do_init())
        return 0;

    hal_irq_alloc( irq, phantom_dev_ps2_int_handler, 0, HAL_IRQ_SHAREABLE );

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = "PS/2 mouse";
    dev->seq_number = seq_number++;

    return dev;

}



















#if 0
//Mouse.inc by SANiK
//License: Use as you wish, except to cause damage
byte mouse_cycle=0;     //unsigned char
sbyte mouse_byte[3];    //signed char
sbyte mouse_x=0;         //signed char
sbyte mouse_y=0;         //signed char

//Mouse functions
void mouse_handler(struct regs *a_r) //struct regs *a_r (not used but just there)
{
    switch(mouse_cycle)
    {
    case 0:
        mouse_byte[0]=inportb(0x60);
        mouse_cycle++;
        break;
    case 1:
        mouse_byte[1]=inportb(0x60);
        mouse_cycle++;
        break;
    case 2:
        mouse_byte[2]=inportb(0x60);
        mouse_x=mouse_byte[1];
        mouse_y=mouse_byte[2];
        mouse_cycle=0;
        break;
    }
}

inline void mouse_wait(byte a_type) //unsigned char
{
    dword _time_out=100000; //unsigned int
    if(a_type==0)
    {
        while(_time_out--) //Data
        {
            if((inportb(0x64) & 1)==1)
            {
                return;
            }
        }
        return;
    }
    else
    {
        while(_time_out--) //Signal
        {
            if((inportb(0x64) & 2)==0)
            {
                return;
            }
        }
        return;
    }
}

inline void mouse_write(byte a_write) //unsigned char
{
    //Wait to be able to send a command
    mouse_wait(1);
    //Tell the mouse we are sending a command
    outportb(0x64, 0xD4);
    //Wait for the final part
    mouse_wait(1);
    //Finally write
    outportb(0x60, a_write);
}

byte mouse_read()
{
    //Get's response from mouse
    mouse_wait(0);
    return inportb(0x60);
}

void mouse_install()
{
    byte _status;  //unsigned char

    //Enable the auxiliary mouse device
    mouse_wait(1);
    outportb(0x64, 0xA8);

    //Enable the interrupts
    mouse_wait(1);
    outportb(0x64, 0x20);
    mouse_wait(0);
    _status=(inportb(0x60) | 2);
    mouse_wait(1);
    outportb(0x64, 0x60);
    mouse_wait(1);
    outportb(0x60, _status);

    //Tell the mouse to use default settings
    mouse_write(0xF6);
    mouse_read();  //Acknowledge

    //Enable the mouse
    mouse_write(0xF4);
    mouse_read();  //Acknowledge

    //Setup the mouse handler
    irq_install_handler(12, mouse_handler);
}
#endif


/**

From: Tomi Engdahl <then@snakemail.hut.fi>
Date: Fri, 29 Apr 1994 13:28:37 +0200 (EET DST)


T{ss{ hiirien speksit englanniksi. Ei nyt ihan t{ydelliset, mutta kyll{
aika kattavat. 

 
        Serial mouse
 
Voltage levels:
Mouse takes standard RS-232C output signals (+-12V) as its input signals.
Those outputs are in +12V when mouse is operated. Mouse takes some current
>from each of the RS-232C port output lines it is connected (about 10mA).
Mouse send data to computer in levels that RS-232C receiver chip in the 
computer can uderstand as RS-232C input levels. Mouse outputs are normally
something like +-5V, 0..5V or sometimes +-12V. Mouse electronics
normally use +5V voltage.
 
 
        Microsoft serial mouse
 
Pins used:
TD, RTS and DTR are used only as power source for the mouse.
RD is used to receive data from mouse.
 
Serial data parameters: 1200bps, 7 databits, 1 stop-bit
 
Data packet format: 
Data packet is 3 byte packet. It is send to the computer every time
mouse state changes (mouse moves or keys are pressed/released).
 
        D7      D6      D5      D4      D3      D2      D1      D0
 
1.      X       1       LB      RB      Y7      Y6      X7      X6
2.      X       0       X5      X4      X3      X2      X1      X0      
3.      X       0       Y5      Y4      Y3      Y2      Y1      Y0
 
The byte marked with 1. is send first, then the others. The bit D6
in the first byte is used for syncronizing the software to mouse
packets if it goes out of sync.
 
LB is the state of the left button (0 means pressed down)
RB is the state of the right button (0 means pressed down)
X7-X0 movement in X direction since last packet (signed byte)
Y7-Y0 movement in Y direction since last packet (signed byte)
 
 
        Mouse systems mouse            
 
Serial data parameters: 1200bps, 8 databits, 1 stop-bit
 
The data is sent in 5 byte packets in following format:
 
        D7      D6      D5      D4      D3      D2      D1      D0
 
1.      1       0       0       0       0       LB      CB      RB
2.      X7      X6      X5      X4      X3      X2      X1      X0
3.      Y7      Y6      Y5      Y4      Y3      Y4      Y1      Y0
4.      
5.
 
LB is left button state (0=pressed, 1=released)
CB is center button state (0=pressed, 1=released)
RB is right button state (0=pressed, 1=released)
X7-X0 movement in X direction since last packet in signed byte 
      format (-128..+127), positive direction right
Y7-Y0 movement in Y direction since last packet in signed byte 
      format (-128..+127), positive direction up
 
The last two bytes in the packet (bytes 4 and 5) contains
information about movement data send in last packet. I have not
found exact information about those bytes. I have not also found
any use for such a information (maybe it is for syncronization 
or something like that).
 
 
 


-- 
Tomi.Engdahl@hut.fi                        Helsinki University of Technology
G=Tomi S=Engdahl O=hut ADMD=fumail C=fi    Department of Computer Science
# This text is provided "as is" without any express or implied warranty #




**/

