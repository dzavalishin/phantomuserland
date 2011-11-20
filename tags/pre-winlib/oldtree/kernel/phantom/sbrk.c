#if 0

// this attempt is wrong. problem is with stack which is right on top of
// ds :(

// I don't know, why this limit :)
#define MAX_U_DS (1024*1024*128)

int usys_brk( errno_t *err, uuprocess_t *u, int ds_size )
{
    if( ds_size > MAX_U_DS )
    {
        *err = ENOMEM;
        return -1;
    }

    struct exe_module *old_em = u->em;

    if( ds_size < (cs_pages*PAGE_SIZE) )
    {
        *err = ENOMEM;
        return -1;
    }

    // TODO use failable phys alloc
    // TODO alloc virt mem in persistent space!
    // * Allocate physical RAM
    void *va;
    physaddr_t pa;
    hal_pv_alloc( &pa, &va, ds_size );

    memset( va, 0, ds_size ); // Clear BSS! :)

    int memsize = ds_size;

    struct exe_module *em = calloc( sizeof(struct exe_module), 1 );
    em->refcount++;

    // TODO Need some table of running modules?

    em->mem_start = va;
    em->mem_end = va+memsize;


    em->mem_size = memsize;
    em->pa = pa;

    em->start = old_em->start;
    em->esp = old_em->esp; // I don't have a clue what's for
#warn impl
    em->stack_bottom = memsize-stack_size;
    // TODO DS must not intersect with CS
    // TODO CS size is wrong (includes DS)

    em->cs_pages = maxpage;
    em->ds_pages = maxpage;





    return 0;
}

#endif

