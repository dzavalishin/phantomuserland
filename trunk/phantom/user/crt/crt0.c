#include <unistd.h>

extern int main(int ac, char **av, char **env);
void __start(int ac, char **av, char **env)
{
	exit( main( ac, av, env ) );
	//main( 0, 0, 0 );
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
