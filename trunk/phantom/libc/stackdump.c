#include <assert.h>

#define MAX_DEPTH 16

char * (*phantom_symtab_getname)( void *addr ) = 0;

static void stack_dump_one(void *ebp)
{
    void *addr = *( ((void**)ebp) + 1);
    if( addr == 0 )
        return;

    printf("- %08x", addr);

    if(phantom_symtab_getname)
    {
        printf(": %s\n", phantom_symtab_getname(addr) );
    }
    else
        printf("\n");


}


void stack_dump_ebp(void *ebp)
{
    printf("Stack:\n");

    int i;
    for( i = 0; i < MAX_DEPTH; i++ )
    {
        if( ebp == 0 )
            break;
        stack_dump_one(ebp);
        ebp = *((void **)ebp);
    }
}


void stack_dump()
{
    void *ebp;
    asm volatile ("movl %%ebp,%0" : "=r" (ebp));
    stack_dump_ebp(ebp);

}


