#include <phantom_types.h>
#include <phantom_libc.h>
#include <malloc.h>

#include <hal.h>

#include "misc.h"

#include <kernel/debug.h>
#include <kernel/page.h>
#include <kernel/init.h>

#define addr_t u_int32_t
#define NULL 0


/*
 ** Copyright 2001, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */



#if DEBUG > 1
#define PARANOID_KFREE 1
#endif
#if DEBUG > 2
#define WIPE_KFREE 1
#endif
#define MAKE_NOIZE 0

// heap stuff
// ripped mostly from nujeffos

struct heap_page {
    unsigned short bin_index : 5;
    unsigned short free_count : 9;
    unsigned short cleaning : 1;
    unsigned short in_use : 1;
};



static struct heap_page *heap_alloc_table;
static addr_t heap_base_ptr;
static addr_t heap_base;
static addr_t heap_size;

struct heap_bin {
    unsigned int element_size;
    unsigned int grow_size;
    unsigned int alloc_count;
    void *free_list;
    unsigned int free_count;
    char *raw_list;
    unsigned int raw_count;
};
static struct heap_bin bins[] = {
    {16, PAGE_SIZE, 0, 0, 0, 0, 0},
    {32, PAGE_SIZE, 0, 0, 0, 0, 0},
    {64, PAGE_SIZE, 0, 0, 0, 0, 0},
    {128, PAGE_SIZE, 0, 0, 0, 0, 0},
    {256, PAGE_SIZE, 0, 0, 0, 0, 0},
    {512, PAGE_SIZE, 0, 0, 0, 0, 0},
    {1024, PAGE_SIZE, 0, 0, 0, 0, 0},
    {2048, PAGE_SIZE, 0, 0, 0, 0, 0},
    {0x1000, 0x1000, 0, 0, 0, 0, 0},
    {0x2000, 0x2000, 0, 0, 0, 0, 0},
    {0x3000, 0x3000, 0, 0, 0, 0, 0},
    {0x4000, 0x4000, 0, 0, 0, 0, 0},
    {0x5000, 0x5000, 0, 0, 0, 0, 0},
    {0x6000, 0x6000, 0, 0, 0, 0, 0},
    {0x7000, 0x7000, 0, 0, 0, 0, 0},
    {0x8000, 0x8000, 0, 0, 0, 0, 0},
    {0x9000, 0x9000, 0, 0, 0, 0, 0},
    {0xa000, 0xa000, 0, 0, 0, 0, 0},
    {0xb000, 0xb000, 0, 0, 0, 0, 0},
    {0xc000, 0xc000, 0, 0, 0, 0, 0},
    {0xd000, 0xd000, 0, 0, 0, 0, 0},
    {0xe000, 0xe000, 0, 0, 0, 0, 0},
    {0xf000, 0xf000, 0, 0, 0, 0, 0},
    {0x10000, 0x10000, 0, 0, 0, 0, 0}, // 64k
    {0x20000, 0x20000, 0, 0, 0, 0, 0}, // 128k
    {0x40000, 0x40000, 0, 0, 0, 0, 0}, // 256k
    {0x80000, 0x80000, 0, 0, 0, 0, 0}, // 512k
    {0x100000, 0x100000, 0, 0, 0, 0, 0}, // 1M
    {0x200000, 0x200000, 0, 0, 0, 0, 0}, // 2M
    {0x400000, 0x400000, 0, 0, 0, 0, 0}, // 4M
    {0x800000, 0x800000, 0, 0, 0, 0, 0}, // 8M
    {0x1000000, 0x1000000, 0, 0, 0, 0, 0}, // 16M
};

static const int bin_count = sizeof(bins) / sizeof(struct heap_bin);
static hal_mutex_t *heap_lock = 0; // won't be used until assigned

static void dump_bin(int bin_index)
{
    struct heap_bin *bin = &bins[bin_index];
    unsigned int *temp;

    printf("%d:\tesize %d\tgrow_size %d\talloc_count %d\tfree_count %d\traw_count %d\traw_list %p\n",
            bin_index, bin->element_size, bin->grow_size, bin->alloc_count, bin->free_count, bin->raw_count, bin->raw_list);
    printf("free_list: ");
    for(temp = bin->free_list; temp != NULL; temp = (unsigned int *)*temp) {
        printf("%p ", temp);
    }
    printf("NULL\n");
}

#pragma GCC diagnostic ignored "-Wunused-function"
static void dump_bin_list(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    int i;

    printf("%d heap bins at %p:\n", bin_count, bins);

    for(i=0; i<bin_count; i++) {
        dump_bin(i);
    }
}

// called from vm_init. The heap should already be mapped in at this point, we just
// do a little housekeeping to set up the data structure.
static void do_phantom_heap_init(addr_t new_heap_base, unsigned int new_heap_size)
{
    const unsigned int page_entries = PAGE_SIZE / sizeof(struct heap_page);
    // set some global pointers
    heap_alloc_table = (struct heap_page *)new_heap_base;
    //heap_size = ((uint64)new_heap_size * page_entries / (page_entries + 1)) & ~(PAGE_SIZE-1);
    heap_size = new_heap_size - PAGE_SIZE;  // use above line instead if new_heap_size > sqr(PAGE_SIZE)/2
    heap_base = (unsigned int)heap_alloc_table + PAGE_ALIGN(heap_size / page_entries);
    heap_base_ptr = heap_base;
    //printf("heap_alloc_table = %p, heap_base = 0x%lx, heap_size = 0x%lx\n", heap_alloc_table, heap_base, heap_size);

    // zero out the heap alloc table at the base of the heap
    memset((void *)heap_alloc_table, 0, (heap_size / PAGE_SIZE) * sizeof(struct heap_page));


    // set up some debug commands
    dbg_add_command(&dump_bin_list, "heap-bindump", "dump stats about bin usage");

}



void phantom_heap_init(void)
{
    //printf("Init heap of %d Mb\n", phantom_start_heap_size/(1024*1024L) );
    //printf("Init heap of %d bytes\n", phantom_start_heap_size );
#if 0 // def ARCH_mips
    extern char __end__[];
    do_phantom_heap_init((addr_t)&__end__, PHANTOM_START_HEAP_SIZE);
#else
    extern char phantom_start_heap_start[];
    extern int phantom_start_heap_size;
    do_phantom_heap_init((addr_t)&phantom_start_heap_start, phantom_start_heap_size);
#endif
}


// Called after initing threads (and mutexes)
void heap_init_mutex(void)
{
    // Mutex init uses malloc! First allocate it (malloc will run unprotected),
    // then assign, turning on protection

    static hal_mutex_t mutex;
    if(hal_mutex_init(&mutex,"Heap") < 0) {
        panic("error creating heap mutex\n");
    }

    heap_lock = &mutex;
}


static char *raw_alloc(unsigned int size, int bin_index)
{
    unsigned int new_heap_ptr;
    struct heap_page *page;
    unsigned int addr;

    new_heap_ptr = heap_base_ptr + PAGE_ALIGN(size);
    if(new_heap_ptr > heap_base + heap_size) {
        panic("heap overgrew itself!\n");
    }

    for(addr = heap_base_ptr; addr < new_heap_ptr; addr += PAGE_SIZE) {
        page = &heap_alloc_table[(addr - heap_base) / PAGE_SIZE];
        page->in_use = 1;
        page->cleaning = 0;
        page->bin_index = bin_index;
        if (bin_index < bin_count && bins[bin_index].element_size < PAGE_SIZE)
            page->free_count = PAGE_SIZE / bins[bin_index].element_size;
        else
            page->free_count = 1;
    }

    char *retval = (char *)heap_base_ptr;
    heap_base_ptr = new_heap_ptr;
    return retval;
}

void *malloc(size_t size)
{
    void *address = NULL;
    int bin_index;
    unsigned int i;
    struct heap_page *page;

#if MAKE_NOIZE
    printf("kmalloc: asked to allocate size %d\n", size);
#endif

    if(heap_lock) hal_mutex_lock(heap_lock);

    for (bin_index = 0; bin_index < bin_count; bin_index++)
        if (size <= bins[bin_index].element_size)
            break;

    if (bin_index == bin_count) {
        // XXX fix the raw alloc later.
        //address = raw_alloc(size, bin_index);
        panic("kmalloc: asked to allocate too much for now!\n");
        goto out;
    } else {
        if (bins[bin_index].free_list != NULL) {
            address = bins[bin_index].free_list;
            bins[bin_index].free_list = (void *)(*(unsigned int *)bins[bin_index].free_list);
            bins[bin_index].free_count--;
        } else {
            if (bins[bin_index].raw_count == 0) {
                bins[bin_index].raw_list = raw_alloc(bins[bin_index].grow_size, bin_index);
                bins[bin_index].raw_count = bins[bin_index].grow_size / bins[bin_index].element_size;
            }

            bins[bin_index].raw_count--;
            address = bins[bin_index].raw_list;
            bins[bin_index].raw_list += bins[bin_index].element_size;
        }

        bins[bin_index].alloc_count++;
        page = &heap_alloc_table[((unsigned int)address - heap_base) / PAGE_SIZE];
        page[0].free_count--;
#if MAKE_NOIZE
        printf("kmalloc0: page 0x%x: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count);
#endif
        for(i = 1; i < bins[bin_index].element_size / PAGE_SIZE; i++) {
            page[i].free_count--;
#if MAKE_NOIZE
            printf("kmalloc1: page 0x%x: bin_index %d, free_count %d\n", page[i], page[i].bin_index, page[i].free_count);
#endif
        }
    }

out:
    if(heap_lock) hal_mutex_unlock(heap_lock);

#if MAKE_NOIZE
    printf("kmalloc: asked to allocate size %d, returning ptr = %p\n", size, address);
#endif
    return address;
}

void free(void *address)
{
    struct heap_page *page;
    struct heap_bin *bin;
    unsigned int i;

    if (address == NULL)
        return;

    if ((addr_t)address < heap_base || (addr_t)address >= (heap_base + heap_size))
        panic("kfree: asked to free invalid address %p\n", address);

    if(heap_lock) hal_mutex_lock(heap_lock);

#if MAKE_NOIZE
    printf("kfree: asked to free at ptr = %p\n", address);
#endif

    page = &heap_alloc_table[((unsigned)address - heap_base) / PAGE_SIZE];

#if MAKE_NOIZE
    printf("kfree: page 0x%x: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count);
#endif

    if(page[0].bin_index >= bin_count)
        panic("kfree: page %p: invalid bin_index %d\n", page, page->bin_index);

    bin = &bins[page[0].bin_index];

    if(bin->element_size <= PAGE_SIZE && (addr_t)address % bin->element_size != 0)
        panic("kfree: passed invalid pointer %p! Supposed to be in bin for esize 0x%x\n", address, bin->element_size);

    for(i = 0; i < bin->element_size / PAGE_SIZE; i++) {
        if(page[i].bin_index != page[0].bin_index)
            panic("kfree: not all pages in allocation match bin_index\n");
        page[i].free_count++;
    }

#if PARANOID_KFREE
    // walk the free list on this bin to make sure this address doesn't exist already
    {
        unsigned int *temp;
        for(temp = bin->free_list; temp != NULL; temp = (unsigned int *)*temp) {
            if(temp == (unsigned int *)address) {
                panic("kfree: address %p already exists in bin free list\n", address);
            }
        }
    }
#endif
#if WIPE_KFREE
    memset(address, 0x99, bin->element_size);
#endif

    *(unsigned int *)address = (unsigned int)bin->free_list;
    bin->free_list = address;
    bin->alloc_count--;
    bin->free_count++;

    if(heap_lock) hal_mutex_unlock(heap_lock);
}


void *realloc(void *ptr, size_t size)
{
    // TODO poor man's realloc

    void *newmem = malloc( size );
    if( newmem == 0 ) return 0;

    memcpy( newmem, ptr, size ); // XXX ERROR TODO if smaller we're busted!

    free( ptr );

    return newmem;
}




