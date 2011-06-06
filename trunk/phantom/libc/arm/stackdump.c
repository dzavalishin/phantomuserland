#include <assert.h>
#include <phantom_libc.h>  //printf

// ARM FP = R11

/*

fp --> 0 (save code pointer) [fp]
       0 (return link value) [fp, #-4]
       0 (return sp value)   [fp, #-8]
       0 (return fp value)   [fp, #-12]
sp ->> 

 */


#define MAX_DEPTH 16

char * (*phantom_symtab_getname)( void *addr ) = 0;

static void stack_dump_one(void *fp)
{
    if( fp == 0 )
        return;

    //void *addr = *( ((void**)fp) - 1);
    void *addr = *( ((void**)fp));

    printf("- %8p", addr);

    if(phantom_symtab_getname)
    {
        printf(": %s\n", phantom_symtab_getname(addr-8) );
    }
    else
        printf("\n");

}


void stack_dump_from(void *fp)
{
    int i;
    printf("Stack:\n");

    for( i = 0; i < MAX_DEPTH; i++ )
    {
        if( fp == 0 )
            break;
        stack_dump_one(fp);
        fp = ((void **)fp)[-3];
    }
}




