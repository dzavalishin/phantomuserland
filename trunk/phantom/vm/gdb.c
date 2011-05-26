/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * This is a debugger interface. It is modelled ater (and made of) a popular GDB stub,
 * and tries to be compatible where possible. This stub serves object space debugging,
 * not C/asm level.
 *
 *
 **/


/*
 *  arch/s390/kernel/gdb-stub.c
 *
 *  S390 version
 *    Copyright (C) 1999 IBM Deutschland Entwicklung GmbH, IBM Corporation
 *    Author(s): Denis Joseph Barrow (djbarrow@de.ibm.com,barrow_dj@yahoo.com),
 *
 *  Originally written by Glenn Engel, Lake Stevens Instrument Division
 *
 *  Contributed by HP Systems
 *
 *  Modified for SPARC by Stu Grossman, Cygnus Support.
 *
 *  Modified for Linux/MIPS (and MIPS in general) by Andreas Busse
 *  Send complaints, suggestions etc. to <andy@waldorf-gmbh.de>
 *
 *  Copyright (C) 1995 Andreas Busse
 */

/*
 *  To enable debugger support, two things need to happen.  One, a
 *  call to set_debug_traps() is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to breakpoint().  Breakpoint()
 *  simulates a breakpoint by executing a BREAK instruction.
 *
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 */

#include <string.h>
#include <assert.h>
#include "vm/internal_da.h"
#include "vm/exec.h"



struct gdb_pt_regs
{
    void *		r_thread;

    void *              r_ostack;
    void *              r_istack;
    void *              r_estack;

    int         	r_ip;
    struct pvm_object 	r_this;
    struct pvm_object 	r_frame;
};


typedef struct gdb_pt_regs gdb_pt_regs;



static void get_regs(gdb_pt_regs *r, struct data_area_4_thread *da)
{
    r->r_thread = da;
    r->r_ostack = da->_ostack;
    r->r_istack = da->_istack;
    r->r_estack = da->_estack;

    r->r_ip	= da->code.IP;
    r->r_this	= da->_this_object;
    r->r_frame	= da->call_frame;
}

static void set_regs(gdb_pt_regs *r, struct data_area_4_thread *da)
{
     //da                   = r->r_thread;
     da->_ostack          = r->r_ostack;
     da->_istack          = r->r_istack;
     da->_estack          = r->r_estack;

     da->code.IP          = r->r_ip;
     da->_this_object     = r->r_this;
     da->call_frame       = r->r_frame;

    pvm_exec_load_fast_acc(da);
}



/*
#include <asm/gdb-stub.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/pgtable.h>
#include <asm/system.h>
*/

/*
 * external low-level support routines
 */

extern int putDebugChar(char c);    /* write a single character      */
extern char getDebugChar(void);     /* read and return a single char */
//extern void fltr_set_mem_err(void);
//extern void trap_low(void);

/*
 * breakpoint and test functions
 */
//extern void breakpoint(void);
//extern void breakinst(void);

/*
 * local prototypes
 */

static void getpacket(char *buffer);
static void putpacket(char *buffer);
static int hex(unsigned char ch);
static int hexToInt(char **ptr, addr_t *intValue);
static unsigned char *mem2hex(char *mem, char *buf, int count, int may_fault);


/*
 * BUFMAX defines the maximum number of characters in inbound/outbound buffers
 * at least NUMREGBYTES*2 are needed for register packets
 */
#define BUFMAX 2048

static char input_buffer[BUFMAX];
static char output_buffer[BUFMAX];
int gdb_stub_initialised = 0;
static const char hexchars[]="0123456789abcdef";


/*
 * Convert ch from a hex digit to an int
 */
static int hex(unsigned char ch)
{
    if (ch >= 'a' && ch <= 'f')
        return ch-'a'+10;
    if (ch >= '0' && ch <= '9')
        return ch-'0';
    if (ch >= 'A' && ch <= 'F')
        return ch-'A'+10;
    return -1;
}

/*
 * scan for the sequence $<data>#<checksum>
 */
static void getpacket(char *buffer)
{
    unsigned char checksum;
    unsigned char xmitcsum;
    int i;
    int count;
    unsigned char ch;

    while(1)
    {
        printf("GDB read pkt " );
        /*
         * wait around for the start character,
         * ignore all other characters
         */
        while ((ch = (getDebugChar() & 0x7f)) != '$') ;

        //printf("GDB pkt start " );

        checksum = 0;
        xmitcsum = -1;
        count = 0;

        /*
         * now, read until a # or end of buffer is found
         */
        while (count < BUFMAX) {
            ch = getDebugChar() & 0x7f;
            if (ch == '#')
                break;
            checksum = checksum + ch;
            buffer[count] = ch;
            count = count + 1;
        }

        printf("GDB pkt # " );

        if (count >= BUFMAX)
            continue;

        buffer[count] = 0;

        if (ch == '#') {

            xmitcsum = hex(getDebugChar() & 0x7f) << 4;
            xmitcsum |= hex(getDebugChar() & 0x7f);

            //printf("GDB got csum " );

            if (checksum != xmitcsum)
                putDebugChar('-');	/* failed checksum */
            else {
                putDebugChar('+'); /* successful transfer */

                /*
                 * if a sequence char is present,
                 * reply the sequence ID
                 */
                if (buffer[2] == ':') {
                    putDebugChar(buffer[0]);
                    putDebugChar(buffer[1]);

                    /*
                     * remove sequence chars from buffer
                     */
                    count = strlen(buffer);
                    for (i=3; i <= count; i++)
                        buffer[i-3] = buffer[i];
                }
            }
        }

        if(checksum != xmitcsum)
        {
            printf("GDB my check = %x, his = %x", checksum, xmitcsum );
            continue;
        }
        break;
    }
}

/*
 * send the packet in buffer.
 */
static void putpacket(char *buffer)
{
    unsigned char checksum;
    int count;
    unsigned char ch;

    /*
     * $<packet info>#<checksum>.
     */

    do {
        putDebugChar('$');
        checksum = 0;
        count = 0;

        while ((ch = buffer[count]) != 0) {
            if (!(putDebugChar(ch)))
                return;
            checksum += ch;
            count += 1;
        }

        putDebugChar('#');
        putDebugChar(hexchars[checksum >> 4]);
        putDebugChar(hexchars[checksum & 0xf]);

    }
    while ((getDebugChar() & 0x7f) != '+');
}



/*
 * Convert the memory pointed to by mem into hex, placing result in buf.
 * Return a pointer to the last char put in buf (null), in case of mem fault,
 * return 0.
 * If MAY_FAULT is non-zero, then we will handle memory faults by returning
 * a 0, else treat a fault like any other fault in the stub.
 */
static unsigned char *mem2hex(char *mem, char *buf, int count, int may_fault)
{
    unsigned char ch;

    //assert(!may_fault);

    /*	set_mem_fault_trap(may_fault); */

    while (count-- > 0) {
        ch = *(mem++);
//#warning implement mem_err
        //if (mem_err)            return 0;
        *buf++ = hexchars[ch >> 4];
        *buf++ = hexchars[ch & 0xf];
    }

    *buf = 0;

    /*	set_mem_fault_trap(0); */

    return (unsigned char *)buf;
}

/*
 * convert the hex array pointed to by buf into binary to be placed in mem
 * return a pointer to the character AFTER the last byte written
 */
static char *hex2mem(char *buf, char *mem, int count, int may_fault)
{
    int i;
    unsigned char ch;

    assert(!may_fault);
    /*	set_mem_fault_trap(may_fault); */

    for (i=0; i<count; i++)
    {
        ch = hex(*buf++) << 4;
        ch |= hex(*buf++);
        *(mem++) = ch;
//#warning implement mem_err
        //if (mem_err)            return 0;
    }

    /*	set_mem_fault_trap(0); */

    return mem;
}



/*
 * Set up exception handlers for tracing and breakpoints
 */
void set_debug_traps(void)
{
    //	unsigned long flags;
    unsigned char c;

    //	save_and_cli(flags);
    /*
     * In case GDB is started before us, ack any packets
     * (presumably "$?#xx") sitting there.
     */
    while((c = getDebugChar()) != '$');
    while((c = getDebugChar()) != '#');
    c = getDebugChar(); /* eat first csum byte */
    c = getDebugChar(); /* eat second csum byte */
    putDebugChar('+'); /* ack it */

    gdb_stub_initialised = 1;
    //	restore_flags(flags);
}


/*
 * Trap handler for memory errors.  This just sets mem_err to be non-zero.  It
 * assumes that %l1 is non-zero.  This should be safe, as it is doubtful that
 * 0 would ever contain code that could mem fault.  This routine will skip
 * past the faulting instruction after setting mem_err.
 */
extern void fltr_set_mem_err(void)
{
    /* TODO: Needs to be written... */
}


/*
 * While we find nice hex chars, build an int.
 * Return number of chars processed.
 */
static int hexToInt(char **ptr, addr_t *intValue)
{
    int numChars = 0;
    int hexValue;

    *intValue = 0;

    while (**ptr)
    {
        hexValue = hex(**ptr);
        if (hexValue < 0)
            break;

        *intValue = (*intValue << 4) | hexValue;
        numChars ++;

        (*ptr)++;
    }

    return (numChars);
}

#if 0
void gdb_stub_get_non_pt_regs(gdb_pt_regs *regs)
{
#warning implement
    /*
    s390_fp_regs *fpregs=&regs->fp_regs;
    int has_ieee=save_fp_regs1(fpregs);

    if(!has_ieee)
    {
        fpregs->fpc=0;
        fpregs->fprs[1].d=
            fpregs->fprs[3].d=
            fpregs->fprs[5].d=
            fpregs->fprs[7].d=0;
        memset(&fpregs->fprs[8].d,0,sizeof(freg_t)*8);
        }
    */
}

void gdb_stub_set_non_pt_regs(gdb_pt_regs *regs)
{
#warning implement
    //restore_fp_regs1(&regs->fp_regs);
}
#endif

void gdb_stub_send_signal(int sigval)
{
    char *ptr;
    ptr = output_buffer;

    /*
     * Send trap type (converted to signal)
     */
    *ptr++ = 'S';
    *ptr++ = hexchars[sigval >> 4];
    *ptr++ = hexchars[sigval & 0xf];
    *ptr++ = 0;
    putpacket(output_buffer);	/* send it off... */
}

void gdb_stub_handle_cmds(struct data_area_4_thread *da, int signal)
{
    addr_t addr;
    addr_t length; // need size_t, but hex2int decodes addr_t
    //size_t length;
    char *ptr;

    /*
     * Wait for input from remote GDB
     */
    while (1) {
        //printf("GDB wait for cmd\n" );

        output_buffer[0] = 0;
        getpacket(input_buffer);

        printf("GDB cmd '%s'\n", input_buffer );

        switch (input_buffer[0])
        {
        case '?':
            gdb_stub_send_signal(signal);
            continue;

        case 'd':
            /* toggle debug flag */
            break;

            /*
             * Return the value of the CPU registers
             */
        case 'g':
            {
                gdb_pt_regs r;
                get_regs(&r, da);
                //gdb_stub_get_non_pt_regs(regs);
                ptr = output_buffer;
                ptr = (char *)mem2hex((char *)&r,ptr,sizeof(r),0);
                //ptr=  mem2hex((char *)regs,ptr,sizeof(s390_regs_common),FALSE);
                //ptr=  mem2hex((char *)&regs->crs[0],ptr,NUM_CRS*CR_SIZE,FALSE);
                //ptr = mem2hex((char *)&regs->fp_regs, ptr,sizeof(s390_fp_regs));
            }
            break;

            /*
             * set the value of the CPU registers - return OK
             * TODO: Needs to be written
             */
        case 'G':
            {
                gdb_pt_regs r;
                ptr=input_buffer;
                hex2mem (ptr, (char *)&r,sizeof(r), 0);
                set_regs(&r, da);
                /*
                 hex2mem (ptr, (char *)regs,sizeof(s390_regs_common), FALSE);
                 ptr+=sizeof(s390_regs_common)*2;
                 hex2mem (ptr, (char *)regs->crs[0],NUM_CRS*CR_SIZE, FALSE);
                 ptr+=NUM_CRS*CR_SIZE*2;
                 hex2mem (ptr, (char *)regs->fp_regs,sizeof(s390_fp_regs), FALSE);
                 gdb_stub_set_non_pt_regs(regs);
                 */
                strcpy(output_buffer,"OK");
            }
            break;

            /*
             * mAA..AA,LLLL  Read LLLL bytes at address AA..AA
             */
        case 'm':
            ptr = &input_buffer[1];

            if (hexToInt(&ptr, &addr)
                && *ptr++ == ','
                && hexToInt(&ptr, &length))
            {
                printf("GDB read mem %d @ 0x%p\n", (int)length, (void *)addr );
                if (mem2hex((char *)addr, output_buffer, length, 1))
                    break;
                strcpy (output_buffer, "E03");
            }
            else
                strcpy(output_buffer,"E01");
            break;

            /*
             * MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK
             */
        case 'M':
            ptr = &input_buffer[1];

            if (hexToInt(&ptr, &addr)
                && *ptr++ == ','
                && hexToInt(&ptr, &length)
                && *ptr++ == ':')
            {
                if (hex2mem(ptr, (char *)addr, length, 1))
                    strcpy(output_buffer, "OK");
                else
                    strcpy(output_buffer, "E03");
            }
            else
                strcpy(output_buffer, "E02");
            break;

            /*
             * cAA..AA    Continue at address AA..AA(optional)
             */
        case 'c':
            /* try to read optional parameter, pc unchanged if no parm */
            {
                addr_t addr;

                ptr = &input_buffer[1];

                if (hexToInt(&ptr, &addr))
                    da->code.IP = addr;

                pvm_exec_save_fast_acc(da);
            }
            return;
            /* NOTREACHED */
            break;


            /*
             * kill the program
             */
        case 'k' :
            break;		/* do nothing */


            /*
             * Reset the whole machine (TODO: system dependent)
             */
        case 'r':
            break;


            /*
             * Step to next instruction
             */
        case 's':
            //#warning implement
            //single_step(regs);
            //flush_cache_all();
            return;
            /* NOTREACHED */

            //}
            break;


            /*
             * Query
             */
        case 'q':
            ptr = &input_buffer[2];
            {
                static int startTid = 0;
                if( 0 == strcmp( ptr, "fThreadInfo" ))
                {
                    startTid = 0;
                    goto sendThreadList;
                }
                if( 0 == strcmp( ptr, "sThreadInfo" ))
                {
                sendThreadList:
                    ;
                    ptr = output_buffer;
#if 0
                    int maxcnt = BUFMAX/(sizeof(int)*3);
                    while(maxcnt-- > 0)
                    {
                        // put hex list of tids sep by comma
                    }
#else
                    if(startTid == 0)
                    {
                        // just one thread in pvm_test yet
                        snprintf( output_buffer, sizeof(output_buffer), "%lx", (unsigned long) 1 );
                        startTid++;
                    }
                    else
                        snprintf( output_buffer, sizeof(output_buffer), "l" );
#endif
                    break;
                }
            }
            break;


            /*
             * Phantom specific
             */
        case ':':
            ptr = &input_buffer[2];

            switch(input_buffer[1])
            {

                // return persistent address space start addr
            case 'p':
                {
                    //void *a = hal_object_space_address();
                    //mem2hex( (char *)&a, output_buffer, sizeof(a), 1);
                    //printf(":p answer '%s'\n", output_buffer );
                    snprintf( output_buffer, sizeof(output_buffer), "%lx", (unsigned long) hal_object_space_address() );
                    break;
                }

                // Run class method (create object and run in new thread)
            case 'r':
                {
                    if( (!hexToInt(&ptr, &addr)) || *ptr++ != ',' || *ptr++ == 0 )
                    {
                        strcpy(output_buffer, "E03"); // TODO check 03
                        break;
                    }

                    const char *class_name = ptr;
                    int method = (int)addr;

                    create_and_run_object(class_name, method );
                    strcpy(output_buffer, "OK");
                    break;
                }

                // Create object
            case 'c':
                {
                    pvm_object_t cns = pvm_create_string_object(ptr);
                    if( pvm_is_null(cns) )
                    {
                        strcpy(output_buffer, "E03"); // TODO check 03
                        break;
                    }

                    pvm_object_t c = pvm_exec_lookup_class_by_name(cns);
                    if( pvm_is_null(c) )
                    {
                        strcpy(output_buffer, "E02"); // TODO check 02
                        break;
                    }

                    pvm_object_t o = pvm_create_object(c);
                    if( pvm_is_null(o) )
                    {
                        strcpy(output_buffer, "E01"); // TODO check 01
                        break;
                    }

                    // TODO add to kernel objects list (object must be available from root)

                    snprintf( output_buffer, sizeof(output_buffer), "%lx, %lx", (long)o.data, (long)o.interface  );
                    break;
                }

            case 's':
                {
                    addr_t mode;
                    if(hexToInt(&ptr, &mode))
                    {
                        printf("GDB snapshots %s\n", mode ? "on" : "off" );
                        // TODO implement
                        strcpy(output_buffer,"E01");
                        //snprintf( output_buffer, sizeof(output_buffer), "OK" );
                    }
                    else
                        strcpy(output_buffer,"E01");

                    break;
                }

            default:
                strcpy(output_buffer,"E00"); // TODO check

            }

            break;


        }			/* switch */

        /*
         * reply to the request
         */

        printf("GDB answer '%s'\n", output_buffer );

        putpacket(output_buffer);

    } /* while */
}


/*
 * This function does all command processing for interfacing to gdb.  It
 * returns 1 if you should skip the instruction at the trap address, 0
 * otherwise.
 */
void gdb_stub_handle_exception(struct data_area_4_thread *da, int signal)
                               //gdb_pt_regs *regs,int sigval)
{
    //int trap;			/* Trap type */
    //unsigned long *stack;


    /*
     * reply to host that an exception has occurred
     */
    gdb_stub_send_signal(signal);
    gdb_stub_handle_cmds(da, signal);

}



#if 0

int putDebugChar(char c)    /* write a single character      */
{
//#warning implement
    return 0;
}

extern char getDebugChar(void)     /* read and return a single char */
{
//#warning implement
    return 0;
}

#endif

