#include <assert.h>
//#include <phantom_libc.h>  //printf
#include <kernel/debug.h> // lprintf


#define MAX_DEPTH 16

char * (*phantom_symtab_getname)( void *addr ) = 0;

static void stack_dump_one(void *ebp)
{
    void *addr = *( ((void**)ebp) + 1);
    if( addr == 0 )
        return;

    lprintf("- %8p", addr);

    if(phantom_symtab_getname)
    {
        lprintf(": %s\n", phantom_symtab_getname(addr) );
    }
    else
        lprintf("\n");


}


void stack_dump_from(void *ebp)
{
    int i;
    lprintf("Stack:\n");

    for( i = 0; i < MAX_DEPTH; i++ )
    {
        if( ebp == 0 )
            break;
        stack_dump_one(ebp);
        ebp = *((void **)ebp);
    }
}



