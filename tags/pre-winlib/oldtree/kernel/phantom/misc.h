#ifndef MISC_H
#define MISC_H

// TODO move all init to kernel/init.h

#include <multiboot.h>
#include <errno.h>


// Phantom junkyard of func prototypes

// Boot state

//extern struct multiboot_info bootParameters;




int main();
//void init_stray_checker(void);


void phantom_map_mem_equally(void);



//void port_init(void);


//void phantom_start_video_driver(void);






// TODO move to time.h
/* wait by spinning: number of milliseconds to delay */
//void phantom_spinwait(int millis);
// Uptime in seconds
//time_t uptime(void);




//void hal_cpu_reset_real(void);


void net_test(void);

void resolver_init(void);


#endif // MISC_H

// To init.h

//void phantom_parse_cmd_line(struct multiboot_info *bootParameters);
//void phantom_process_boot_options(void);
//void phantom_start_boot_modules(void);



#include <multiboot.h>




//errno_t load_elf( void *_elf, size_t elf_size, const char *name );
/*
errno_t get_uldt_cs_ds(
                       linaddr_t cs_base, u_int16_t *ocs, size_t cs_limit,
                       linaddr_t ds_base, u_int16_t *ods, size_t ds_limit
                      );
*/

void sound(u_int32_t frequency);
void nosound(void);
void beep(void);


// -----------------------------------------------------------------------
// Unix emulation
// -----------------------------------------------------------------------

extern int syscall(void);


void phantom_unix_fs_init(void);



// -----------------------------------------------------------------------
// Misc debug
// -----------------------------------------------------------------------

void check_global_lock_entry_count(void);
void pressEnter(char *text);

void DumpVgaMode(void);


// -----------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------



void trfs_testrq(void);
//void init_tetris(void);
void connect_ide_io(void);

// -----------------------------------------------------------------------
// Scheduler
// -----------------------------------------------------------------------


//void phantom_scheduler_time_interrupt(void);
