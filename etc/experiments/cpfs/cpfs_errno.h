
#ifndef __CPFS_ERRNO_H__
#define __CPFS_ERRNO_H__

#include <errno.h>


#ifdef __LIBJET_ERRNO_H__

#define EIO 5
#define EACCES 13
#define ENOENT 2
#define EINVAL 22
#define EMFILE 24
#define ENOSPC 28
#define ENOMEM 12
#define EEXIST 17
#define ENOTDIR 20
#define ENOTEMPTY 90

#define	EAGAIN 11
#define EWOULDBLOCK EAGAIN


#define E2BIG 7
#define EFAULT 14



#endif



#endif // __CPFS_ERRNO__
