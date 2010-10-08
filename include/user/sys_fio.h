#include <phantom_types.h>

//int write( int, void *, int);
//int read( int, void *, int);

ssize_t read(int, void *, size_t);
ssize_t write(int, void const*, size_t);


//int open(const char *name, int mode);

int     open(char const *, int, ...);

int close(int fd);

int chdir(const char *name);
