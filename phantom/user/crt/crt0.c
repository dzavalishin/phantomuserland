#include <unistd.h>

//#include "../stdio_private.h"

extern int main(int ac, char **av, char **env);

extern int __stdio_init(void);
extern int __stdio_deinit(void);


void __start(int ac, char **av, char **env)
{
    // 4mb
    static char heap[1024*1024*4];
    __init_malloc( heap, sizeof(heap) );

    __stdio_init();

    int rc = main( ac, av, env );

    __stdio_deinit();

    exit( rc );
}

// arm build needs this
void _start(int ac, char **av, char **env)
{
    __start( ac, av, env);
}


// automatically called by GCC from main in SOME (!) cases
void __main()
{
}
