#ifndef UIO_H
#define UIO_H

#include <phantom_types.h>

struct iovec 
{
    void   *iov_base;  /* Base address. */
    size_t iov_len;    /* Length. */
};

//ssize_t read (int d void *buf size_t nbytes);
size_t pread (int d, void *buf, size_t nbytes, off_t offset);
size_t readv (int d, const struct iovec *iov, int iovcnt);
size_t preadv (int d, const struct iovec *iov, int iovcnt, off_t offset);

//ssize_t write (int d const void *buf size_t nbytes);
size_t pwrite (int d, const void *buf, size_t nbytes, off_t offset);
size_t writev (int d, const struct iovec *iov, int iovcnt);
size_t pwritev (int d, const struct iovec *iov, int iovcnt, off_t offset);

#endif // UIO
