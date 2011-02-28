//#define PHANTOM_START_STACK_SIZE (128*1024)
#define PHANTOM_START_STACK_SIZE (2*1024*1024)

#ifndef ASSEMBLER

extern char phantom_start_stack_start[], phantom_start_stack_end[];
extern int  phantom_start_stack_size;

#endif

#include <kernel/page.h>
//#include <x86/phantom_page.h>

