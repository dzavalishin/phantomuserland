//#define PHANTOM_START_STACK_SIZE (128*1024)
#define PHANTOM_START_STACK_SIZE (2*1024*1024)

#ifndef ASSEMBLER

extern char phantom_start_stack_start[], phantom_start_stack_end[];
extern int  phantom_start_stack_size;

#endif

// TODO must be arch specific
#define PAGE_SHIFT	12


