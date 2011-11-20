#include <stdio.h>

void perror(const char *s)
{
// TODO need errno
    printf("Error: %s\n", s);
}
