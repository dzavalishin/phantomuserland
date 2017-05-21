#include <kernel/config.h>
#include <kernel/page.h>


char phantom_start_heap_start[PHANTOM_START_HEAP_SIZE];
//char phantom_start_heap_end[1];

long phantom_start_heap_size = PHANTOM_START_HEAP_SIZE;

