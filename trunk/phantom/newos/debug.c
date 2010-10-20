/*
 ** Copyright 2001-2004, Mark-Jan Bastian. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */

#include <newos/compat.h>
#include <newos/port.h>
#include <newos/cbuf.h>

#include <string.h>
#include <stdlib.h>

#include <threads.h>
#include <spinlock.h>
#include <hal.h>

#include <kernel/init.h>
#include <kernel/debug.h>


#define dprintf printf

// ------------------------------------------------------------------
// Debugger globals
// ------------------------------------------------------------------

void dbg_puts(const char *s);
char dbg_putch(char c);


static char arch_dbg_con_read(void) { return getchar(); }


struct debugger_command
{
    struct debugger_command *next;
    void (*func)(int, char **);
    const char *cmd;
    const char *description;
};

static struct debugger_command *commands;
static int debugger_on_cpu = -1;
static bool serial_debug_on = false;

//static spinlock_t dbg_spinlock;
static hal_spinlock_t dbg_spinlock;




#define LINE_BUF_SIZE 1024
#define MAX_ARGS 16
#define HISTORY_SIZE 16

static char line_buf[HISTORY_SIZE][LINE_BUF_SIZE] = { "", };
static char parse_line[LINE_BUF_SIZE] = "";
static int cur_line = 0;
static char *args[MAX_ARGS] = { NULL, };

#define distance(a, b) ((a) < (b) ? (b) - (a) : (a) - (b))





static void cmd_reboot(int argc, char **argv);
static void cmd_help(int argc, char **argv);
static void cmd_test(int argc, char **argv);













// ------------------------------------------------------------------
// Debugger utils
// ------------------------------------------------------------------


static int debug_read_line(char *buf, int max_len)
{
    char c;
    int ptr = 0;
    bool done = false;
    int cur_history_spot = cur_line;

    while(!done) {
        c = arch_dbg_con_read();
        switch(c) {
        case '\n':
        case '\r':
            buf[ptr++] = '\0';
            dbg_puts("\n");
            done = true;
            break;
        case 8: // backspace
            if(ptr > 0) {
                dbg_puts("\b"); // move to the left one
                dbg_putch(' ');
                dbg_puts("\b"); // move to the left one
                ptr--;
            }
            break;
        case 27: // escape sequence
            c = arch_dbg_con_read(); // should be '['
            c = arch_dbg_con_read();
            switch(c) {
            case 67: // right arrow acts like space
                buf[ptr++] = ' ';
                dbg_putch(' ');
                break;
            case 68: // left arrow acts like backspace
                if(ptr > 0) {
                    dbg_puts("\x1b[1D"); // move to the left one
                    dbg_putch(' ');
                    dbg_puts("\x1b[1D"); // move to the left one
                    ptr--;
                }
                break;
            case 65: // up arrow
            case 66: // down arrow
                {
                    int history_line = 0;

                    //						dprintf("1c %d h %d ch %d\n", cur_line, history_line, cur_history_spot);

                    if(c == 65) {
                        // up arrow
                        history_line = cur_history_spot - 1;
                        if(history_line < 0)
                            history_line = HISTORY_SIZE - 1;
                    } else {
                        // down arrow
                        if(cur_history_spot != cur_line) {
                            history_line = cur_history_spot + 1;
                            if(history_line >= HISTORY_SIZE)
                                history_line = 0;
                        } else {
                            break; // nothing to do here
                        }
                    }

                    //						dprintf("2c %d h %d ch %d\n", cur_line, history_line, cur_history_spot);

                    // swap the current line with something from the history
                    if(ptr > 0)
                        dprintf("\x1b[%dD", ptr); // move to beginning of line

                    strcpy(buf, line_buf[history_line]);
                    ptr = strlen(buf);
                    dprintf("%s\x1b[K", buf); // print the line and clear the rest
                    cur_history_spot = history_line;
                    break;
                }
            default:
                break;
            }
            break;
        case '$':
        case '+':
            /* HACK ALERT!!!
             *
             * If we get a $ at the beginning of the line
             * we assume we are talking with GDB
             */
            if(ptr == 0) {
                strcpy(buf, "gdb");
                ptr= 4;
                done= true;
                break;
            } else {
                /* fall thru */
            }
        default:
            buf[ptr++] = c;
            dbg_putch(c);
        }
        if(ptr >= max_len - 2) {
            buf[ptr++] = '\0';
            dbg_puts("\n");
            done = true;
            break;
        }
    }
    return ptr;
}

static int debug_parse_line(char *buf, char **argv, int *argc, int max_args)
{
    int pos = 0;

    strcpy(parse_line, buf);

    //hexdump(buf, strlen(buf), "", 0 );

    // scan all of the whitespace out of this
    while(isspace(parse_line[pos]))
        pos++;

    argv[0] = parse_line+pos;
    *argc = 1;

    while(parse_line[pos] != '\0')
    {
        if(isspace(parse_line[pos]))
        {
        	//printf("parse - end arg '%s'\n", parse_line+pos);
            parse_line[pos] = '\0';

            // scan all of the whitespace out of this
            while(1)
            {
            	char ch = parse_line[++pos];
            	if(!isspace(ch))
            		break;
            }

            //printf("parse - next arg '%s'\n", parse_line+pos);

            if(parse_line[pos] == '\0')
                break;

            argv[*argc] = parse_line+pos;
            (*argc)++;

            if(*argc >= max_args - 1)
                break;
        }
        pos++;
    }

    return *argc;
}

static void kernel_debugger_loop()
{
    int argc;
    struct debugger_command *cmd;

    //int_disable_interrupts();

    dprintf("kernel debugger on cpu %d\n", GET_CPU_ID());
    debugger_on_cpu = GET_CPU_ID();

    for(;;) {
        dprintf("> ");
        debug_read_line(line_buf[cur_line], LINE_BUF_SIZE);
        debug_parse_line(line_buf[cur_line], args, &argc, MAX_ARGS);

        /*
        printf("argc = %d\n", argc);
        {
            char **av = args;
            int ac = argc;
            while(ac-- > 0)
            {
                printf("arg = '%s'\n", *av++);
            }
        }
		*/

        if(argc <= 0)
            continue;

        debugger_on_cpu = GET_CPU_ID();

        cmd = commands;
        while(cmd != NULL) {
            if(strcmp(args[0], cmd->cmd) == 0) {
                cmd->func(argc, args);
            }
            cmd = cmd->next;
        }
        cur_line++;
        if(cur_line >= HISTORY_SIZE)
            cur_line = 0;
    }

    //int_restore_interrupts();
}

// ------------------------------------------------------------------
// Debugger entries
// ------------------------------------------------------------------


void kernel_debugger()
{
    /* we're toast, so let all the sem calls whatnot through */
    //kernel_startup = true;

    //dbg_save_registers(dbg_register_file);

    kernel_debugger_loop();
}


// Newos inits in 2 stages, and we do in 1
static int dbg_init2()
{
    dbg_add_command(&cmd_help, "help", "List all debugger commands");
    dbg_add_command(&cmd_reboot, "reboot", "Reboot");
    //dbg_add_command(&cmd_gdb, "gdb", "Connect to remote gdb. May supply an optional iframe.");


    dbg_add_command(&cmd_test, "test", "Run named test (possibly with parameter)");

    /*
    int ret = arch_dbg_con_init2();
    if(ret < 0)
        return ret;

    return arch_dbg_init2();
    */
    return 0;
}



int dbg_init()
{
    commands = NULL;

    hal_spin_init(&dbg_spinlock);

    /*
    int ret = arch_dbg_con_init();
    if(ret < 0)
        return ret;
        return arch_dbg_init();
        */
    //return 0;
    return dbg_init2();
}




// ------------------------------------------------------------------
// Debugger IO
// ------------------------------------------------------------------


char dbg_putch(char c)
{
    char ret;

    if(serial_debug_on) {
        int_disable_interrupts();
        acquire_spinlock(&dbg_spinlock);

        //ret = arch_dbg_con_putch(c);

        release_spinlock(&dbg_spinlock);
        int_restore_interrupts();
    } else {
        ret = putchar(c);
    }

    return ret;
}

void dbg_puts(const char *s)
{
    if(serial_debug_on) {
        int_disable_interrupts();
        acquire_spinlock(&dbg_spinlock);

        panic("why here?");
        //arch_dbg_con_puts(s);

        release_spinlock(&dbg_spinlock);
        int_restore_interrupts();
    }
    else
    {
        //puts(s);

        char c;
        while( (c = *s++) )
            putchar( c );

    }
}

ssize_t dbg_write(const void *buf, ssize_t len)
{
    ssize_t ret;

    if(serial_debug_on) {
        int_disable_interrupts();
        acquire_spinlock(&dbg_spinlock);

        panic("why here?");
        //ret = arch_dbg_con_write(buf, len);

        release_spinlock(&dbg_spinlock);
        int_restore_interrupts();
    } else {
        ret = len;
        while(len--)
        {
            putchar( *(char *)buf++ );
        }
    }
    return ret;
}





// ------------------------------------------------------------------
// Debugger commands
// ------------------------------------------------------------------


int dbg_add_command(void (*func)(int, char **), const char *name, const char *desc)
{
    struct debugger_command *cmd;

    cmd = (struct debugger_command *)malloc(sizeof(struct debugger_command));
    if(cmd == NULL)
        return ERR_NO_MEMORY;

    cmd->func = func;
    cmd->cmd = name;
    cmd->description = desc;

    int_disable_interrupts();
    acquire_spinlock(&dbg_spinlock);

    cmd->next = commands;
    commands = cmd;

    release_spinlock(&dbg_spinlock);
    int_restore_interrupts();

    return NO_ERROR;
}


static void cmd_reboot(int argc, char **argv)
{
    hal_cpu_reset_real();
}

static void cmd_help(int argc, char **argv)
{
    struct debugger_command *cmd;

    dprintf("debugger commands:\n\n");
    cmd = commands;
    while(cmd != NULL) {
        dprintf("%-32s\t\t%s\n", cmd->cmd, cmd->description);
        cmd = cmd->next;
    }
}

static void cmd_test(int argc, char **argv)
{
    const char *parm = argc > 2 ? argv[2] : 0;

    dprintf("running test %s (%s):\n\n", argv[1], parm ? parm : "no parameter");
    run_test( argv[2], parm );
}



