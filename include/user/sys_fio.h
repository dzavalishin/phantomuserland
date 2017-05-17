#include <phantom_types.h>
#include <errno.h>

//int write( int, void *, int);
//int read( int, void *, int);

// TODO sync for all archs
#ifdef ARCH_e2k

int 		read(int __fd, void *__buf, size_t __nbyte );
int			write(int __fd, const void *__buf, size_t __nbyte );

#else

ssize_t read(int, void *, size_t);
ssize_t write(int, void const*, size_t);

#endif

//int open(const char *name, int mode);

int     open(char const *, int, ...);
int     creat(char const *, int mode );

int close(int fd);

int chdir(const char *name);
int mkdir(const char *name);

int unlink(const char *name);
int link(const char *n1, const char *n2);

// in sys/stat.h
//int stat( const char *name, struct stat* );

// returns name of property with sequential number nProperty, or error
errno_t listproperties( int fd, int nProperty, char *buf, int buflen );

errno_t getproperty( int fd, const char *pName, char *buf, int buflen );
errno_t setproperty( int fd, const char *pName, const char *pValue );


