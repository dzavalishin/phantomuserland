/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Syscalls entry point
 *
**/

#define DEBUG_MSG_PREFIX "syscall"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include "../config.h"
#include "../net.h"

#include <i386/eflags.h>
#include <i386/trap.h>
#include <i386/seg.h>
#include <i386/ldt.h>
//#include <i386/tss.h>
//#include <i386/proc_reg.h>
//#include <i386/pio.h>
#include <sys/syslog.h>
#include <phantom_types.h>
#include <phantom_libc.h>
#include <kernel/vm.h>
#include <kernel/init.h>
#include <kernel/unix.h>
#include <x86/phantom_page.h>

#include <kernel/syscalls.h>

//#include <i386/vesa.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <dirent.h>

#include <unix/uuprocess.h>
extern struct utsname phantom_uname;

#include <hal.h>
//#include "vm86.h"

#include "../misc.h"
#include "../threads/thread_private.h"

#define adjustin( uaddr, __st__ ) ( ((void*)uaddr) + ((int)u->mem_start))

//static void *adjustin( int uaddr, struct trap_state *st );
errno_t copyout( unsigned useraddr, unsigned mina, unsigned maxa, void *src, int len );

#define CHECKA(a,count) do { \
    int __addr = (int)(a); \
    if( __addr < mina || (__addr+count) >= maxa ) \
        { \
        ret = -1; \
    	err = EFAULT; \
        goto err_ret; \
        } \
    } while(0)

#define AARG(type, var, index, size) \
    type var = adjustin( uarg[index], st ); \
    do { \
    int __addr = (int)(var); \
    if( __addr < mina || (__addr+size) >= maxa ) \
        { \
        ret = -1; \
    	err = EFAULT; \
        goto err_ret; \
        } \
    } while(0)


#define CHECK_CAP(CAP)
/*
#define CHECK_CAP(CAP) \
    do { \
    if( ! (u->capas & CAP) )
    {
        ret = -1; \
    	err = EPERM; \
        goto err_ret; \
    }
    } while(0)
*/

void syscall_sw(struct trap_state *st)
{
    int callno = st->eax;

    /*
    unsigned int mina = 0, maxa = 0;
    {
        extern struct real_descriptor 	ldt[];

        struct real_descriptor  dsd = ldt[ st->ds/8 ];
        mina = dsd.base_low | (dsd.base_med << 16) | (dsd.base_high << 24);
        maxa = (dsd.limit_low | dsd.limit_high << 16);
        if( dsd.granularity & SZ_G)
            maxa *= PAGE_SIZE;
        maxa += mina;
        }
    */


    phantom_thread_t *t = GET_CURRENT_THREAD();
    uuprocess_t *u = t->u;

    assert( u != 0 );

    unsigned int mina = (int)u->mem_start, maxa = (int)u->mem_end;

    // trap state is good for interrupt.
    // call gate does not push eflags
    int user_esp = st->eflags;

    // list of user syscsall arguments
    int *uarg = adjustin( user_esp, st );
    uarg++; // syscall func return addr

    SHOW_FLOW( 7, "Syscall %d args %x, %x, %x", callno, uarg[0], uarg[1],uarg[2] );

    int ret = 0;
    int err = 0;

#if HAVE_UNIX

    switch(callno)
    {
    case SYS_exit:
        // TODO housekeeping?
        SHOW_FLOW( 2, "exit %d", uarg[0] );
        hal_exit_kernel_thread();
        err = ENOSYS;
        break;

    case SYS_ssyslog:
        syslog( uarg[0], "%s", (const char *)adjustin( uarg[1], st ) );
        SHOW_FLOW( 2, "syslog %d %s", uarg[0], (const char *)adjustin( uarg[1], st ) );
        //SHOW_ERROR0( 0, "syslog not impl" );
        break;





    case SYS_sleepmsec:
        SHOW_FLOW( 2, "sleepmsec %d", uarg[0] );
        hal_sleep_msec(uarg[0]);
        break;



    case SYS_getpagesize:       ret = PAGE_SIZE; break;

    case SYS_personality:
        if(uarg[0] == 0xffffffff)
        {
            ret = 0; // Say we're Linux... well...to some extent.
            break;
        }
        err = EINVAL;
        break;

    case SYS_uname:
        err = copyout(uarg[0], mina, maxa, &phantom_uname, sizeof(phantom_uname) );
        if( err ) ret = -1;
        break;


    case SYS_getuid:            ret = u->uid; break;
    case SYS_getuid32:          ret = u->uid;; break;
    case SYS_getegid:           ret = u->egid; break;
    case SYS_getegid32:         ret = u->egid; break;
    case SYS_geteuid:           ret = u->euid; break;
    case SYS_geteuid32:         ret = u->euid; break;
    case SYS_getgid:            ret = u->gid; break;
    case SYS_getgid32:          ret = u->gid; break;

    case SYS_gettid:            ret = t->tid; break;
    case SYS_getpid:            ret = u->pid; break;
    case SYS_getpgrp:           ret = u->pgrp_pid; break;
    case SYS_getppid:           ret = u->ppid; break;
    case SYS_getpgid:


    case SYS_sync:
    case SYS_sysinfo:
    case SYS_sysfs:
    case SYS_klogctl:
    case SYS_shutdown:
    case SYS_reboot:

    case SYS_nanosleep:
    case SYS_time:
    case SYS_getitimer:
    case SYS_setitimer:
    case SYS_gettimeofday:


    case SYS_setuid:
    case SYS_setuid32:          //CHECK_CAP(CAP_SETGID); u->egid = uarg[0]; break;

    case SYS_setgid:            
    case SYS_setgid32:          //CHECK_CAP(CAP_SETGID); u->egid = uarg[0]; break;

    case SYS_setgroups:
    case SYS_setgroups32:
        goto unimpl;

    case SYS_setpgid:
    case SYS_setregid:
    case SYS_setregid32:
    case SYS_setresgid:
    case SYS_setresgid32:
    case SYS_setresuid:
    case SYS_setresuid32:
    case SYS_setreuid:
    case SYS_setreuid32:
        goto unimpl;

    case SYS_open:              ret = usys_open(&err, u, adjustin( uarg[0], st ), uarg[1], uarg[2] ); break;
    case SYS_close:             ret = usys_close(&err, u, uarg[0] ); break;
    case SYS_read:
        {
            int count = uarg[2];
            void *addr = adjustin( uarg[1], st );
            CHECKA(addr,count);
            /*if( (int)addr < mina || (int)(addr+count) > maxa )
            {
                ret = -1;
                err = EFAULT;
            }*/
            ret = usys_read(&err, u, uarg[0], addr, count );
            break;
        }
    case SYS_write:
        {
            int count = uarg[2];
            void *addr = adjustin( uarg[1], st );
            CHECKA(addr,count);
            /*if( (int)addr < mina || (int)(addr+count) > maxa )
            {
                ret = -1;
                err = EFAULT;
            }*/
            ret = usys_write(&err, u, uarg[0], addr, count );
            break;
        }
    case SYS_lseek:             ret = usys_lseek(&err, u, uarg[0], uarg[1], uarg[2] ); break;
    case SYS_creat:		ret = usys_creat(&err, u, adjustin( uarg[0], st ), uarg[1] ); break;
    case SYS_chdir:
        {
            AARG(const char *, path, 0, 0);
            ret = usys_chdir(&err, u, path );
            break;
        }
    case SYS_link:
    case SYS_unlink:
    case SYS__llseek:
    case SYS_chroot:
    case SYS_mkdir:
    case SYS_rmdir:
    case SYS_dup:
    case SYS_dup2:
    case SYS_getcwd:
    case SYS_lstat64:
    case SYS_mknod:
    case SYS_pipe:
    case SYS_symlink:
        goto unimpl;

    case SYS_mount:
        {
            const char *source = adjustin( uarg[0], st );
            CHECKA(source,0);
            const char *target = adjustin( uarg[1], st );
            CHECKA(target,0);
            const char *fstype = adjustin( uarg[2], st );
            CHECKA(target,0);
            const void *data = adjustin( uarg[4], st );
            CHECKA(data,0);

            ret = usys_mount(&err, u, source, target, fstype, uarg[3], data );
            break;
        }
    case SYS_umount:
        {
            const char *target = adjustin( uarg[0], st );
            CHECKA(target,0);
            ret = usys_umount(&err, u, target, 0 );
            break;
        }

    /*
    case SYS_umount2:
        {
            const char *target = adjustin( uarg[0], st );
            CHECKA(target);
            ret = usys_umount(&err, u, target, uarg[1] );
            break;
        }
    */

    case SYS_truncate:
        {
            AARG(const char *, path, 0, 0);
            ret = usys_truncate(&err, u, path, uarg[1] );
            break;
        }
    case SYS_ftruncate:
        {
            ret = usys_ftruncate(&err, u, uarg[0], uarg[1] );
            break;
        }

    case SYS_truncate64:
    case SYS_ftruncate64:
        goto unimpl;

    case SYS_fchdir:
        ret = usys_fchdir( &err, u, uarg[0] );
        break;

    case SYS_readdir:
        {
            AARG(struct dirent *, ent, 1, sizeof(struct dirent));
            ret = usys_readdir( &err, u, uarg[0], ent );
            break;
        }
    case SYS_fchmod:
        ret = usys_fchmod( &err, u, uarg[0], uarg[1] );
        break;

    case SYS_fchown:
    case SYS_fchown32:
        ret = usys_fchown( &err, u, uarg[0], uarg[1], uarg[2] );
        break;

    case SYS_fcntl:
    case SYS_fcntl64:
    case SYS_fdatasync:
    case SYS_flock:

    case SYS_fstat64:
    case SYS_fstatfs:
    case SYS_fsync:

    case SYS_utime:
    case SYS_chmod:
    case SYS_chown:
    case SYS_chown32:
    case SYS_access:
    case SYS_lchown:
    case SYS_lchown32:
    case SYS_pread:
    case SYS_pwrite:
        goto unimpl;

    case SYS_readv:
        {
            int fd = uarg[0];
            int iovcnt = uarg[2];
            AARG(struct iovec *, list, 1, sizeof(struct iovec) * iovcnt);

            int onerc;
            int lp;

            for( lp = 0; lp < iovcnt; lp++ )
            {
                void *addr = adjustin( list[lp].iov_base, st );
                CHECKA(addr,list[lp].iov_len);
                onerc = usys_read(&err, u, fd, addr, list[lp].iov_len );

                if( onerc < 0 )
                {
                    ret = -1;
                    goto err_ret;
                }

                ret += onerc;
                if( onerc < list[lp].iov_len )
                    break;
            }
            break;
        }

    case SYS_writev:
        {
            int fd = uarg[0];
            int iovcnt = uarg[2];
            AARG(struct iovec *, list, 1, sizeof(struct iovec) * iovcnt);

            int onerc;
            int lp;

            for( lp = 0; lp < iovcnt; lp++ )
            {
                void *addr = adjustin( list[lp].iov_base, st );
                CHECKA(addr,list[lp].iov_len);
                onerc = usys_write(&err, u, fd, addr, list[lp].iov_len );

                if( onerc < 0 )
                {
                    ret = -1;
                    goto err_ret;
                }

                ret += onerc;
                if( onerc < list[lp].iov_len )
                    break;
            }
            break;
        }

    case SYS_readlink:
        goto unimpl;


    case SYS_gethostname:
        {
            int len = uarg[1];
            char *target = adjustin( uarg[0], st );
            CHECKA(target,len);
            ret = usys_gethostname(&err, u, target, len );
            break;
        }
    case SYS_sethostname:
        {
            int len = uarg[1];
            const char *target = adjustin( uarg[0], st );
            CHECKA(target,len);
            ret = usys_sethostname(&err, u, target, len );
            break;
        }



    case SYS_socket:
        ret = usys_socket( &err, u, uarg[0], uarg[1], uarg[2] );
        break;

    case SYS_setsockopt:
        {
            socklen_t optlen = uarg[4];
            AARG(const void *, optval, 3, optlen);
            ret = usys_setsockopt( &err, u, uarg[0], uarg[1], uarg[2], optval, optlen);
            break;
        }
    case SYS_getsockopt:
        {
            AARG(socklen_t *, optlen, 4, sizeof(socklen_t));
            AARG(void *, optval, 3, *optlen);
            ret = usys_getsockopt( &err, u, uarg[0], uarg[1], uarg[2], optval, optlen);
            break;
        }

    case SYS_getpeername:
        {
            AARG(socklen_t *, namelen, 2, sizeof(socklen_t));
            AARG(struct sockaddr *, name, 1, *namelen);
            ret = usys_getpeername( &err, u, uarg[0], name, namelen);
            break;
        }
    case SYS_getsockname:
        {
            AARG(socklen_t *, namelen, 2, sizeof(socklen_t));
            AARG(struct sockaddr *, name, 1, *namelen);
            ret = usys_getsockname( &err, u, uarg[0], name, namelen);
            break;
        }

    case SYS_bind:
        {
            AARG(const struct sockaddr *, addr, 1, sizeof(struct sockaddr));
            ret = usys_bind( &err, u, uarg[0], addr, uarg[2] );
            break;
        }

    case SYS_listen:
        ret = usys_listen( &err, u, uarg[0], uarg[1] );
        break;

    case SYS_accept:
    case SYS_connect:

    case SYS_recv:
    case SYS_recvfrom:
    case SYS_recvmsg:
    case SYS_send:
    case SYS_sendto:
    case SYS_sendmsg:
    case SYS_sendfile:

    case SYS_socketcall:
    case SYS_socketpair:


    case SYS_brk:
    case SYS_fork:
    case SYS_vfork:
    case SYS_nice:
        goto unimpl;

    case SYS_ioctl:
        {
            void *data = adjustin( uarg[2], st );
            CHECKA(data,0);
            ret = usys_ioctl( &err, u, uarg[0], uarg[1], data );
            break;
        }

        int statlink;
    case SYS_lstat: statlink = 1; goto dostat;
    case SYS_stat: statlink = 0; goto dostat;

    dostat:
        {
            const char *path = adjustin( uarg[0], st );
            CHECKA(path,0);
            struct stat *data = adjustin( uarg[1], st );
            CHECKA(data,sizeof(struct stat));
            ret = usys_stat( &err, u, path, data, statlink );
            break;
        }

    case SYS_fstat:
        {
            struct stat *data = adjustin( uarg[1], st );
            CHECKA(data,sizeof(struct stat));
            ret = usys_fstat( &err, u, uarg[0], data, 0 );
            break;
        }

    case SYS_umask:
        {
            ret = u->umask;
            u->umask = uarg[0];
        }

    case SYS_kill:
    case SYS_clone:


    case SYS_madvise:
    case SYS_mincore:
    case SYS_mlock:
    case SYS_mlockall:
    case SYS_mmap:
    case SYS_mprotect:
    case SYS_mremap:
    case SYS_msync:
    case SYS_munlock:
    case SYS_munlockall:
    case SYS_munmap:
    unimpl:
        SHOW_ERROR( 0, "Unimplemented syscall %d called", callno );
        err = ENOSYS;
        break;

    default:
        SHOW_ERROR( 0, "Unknown syscall %d called", callno );
        err = ENOSYS;
        break;
    }

#else // HAVE_UNIX
    err = ENOSYS;
    ret = -1;
#endif // HAVE_UNIX

err_ret:
    st->eax = ret;
    st->edx = err;
}

/*
static void *adjustin( int uaddr, struct trap_state *st )
{
    extern struct real_descriptor 	ldt[];

    //panic("adjust in not impl");
    struct real_descriptor  dsd = ldt[ st->ds/8 ];
    int ds_base = dsd.base_low | (dsd.base_med << 16) | (dsd.base_high << 24);
    return (void *)(uaddr+ds_base);
}
*/

errno_t copyout( unsigned useraddr, unsigned mina, unsigned maxa, void *src, int len )
{
    void *dest = (void *)(useraddr+mina);

    if( (int)dest < mina || (int)dest > maxa )
        return EFAULT;

    memcpy( dest, src, len );
    return 0;
}


