#include <user/sys_fio.h>
#include <string.h>

// min stdio helpers for libc printf to work

int putchar( int c )
{
    char cc = c;
    write(1, &cc, 1);
    return cc;
}

int getchar()
{
    char cc;
    read(0, &cc, 1);
    return cc;
}

int puts(const char *s)
{
	int len = strlen(s);
    write(1, s, len);
	putchar( '\n' );
	return len;
}
