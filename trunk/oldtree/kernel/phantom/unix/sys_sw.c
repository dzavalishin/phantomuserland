/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Syscalls dispatcher
 *
**/

#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "syscall"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/config.h>
#include <kernel/net.h>
#include <netinet/resolv.h>

//#include <i386/eflags.h>
//#include <i386/seg.h>
//#include <i386/ldt.h>
#include <newos/port.h>
#include <sys/syslog.h>
#include <phantom_types.h>
#include <phantom_libc.h>

#include <kernel/vm.h>
#include <kernel/init.h>
#include <kernel/unix.h>
#include <kernel/trap.h>
#include <kernel/page.h>

#include <kernel/syscalls.h>

#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include <dirent.h>
#include <signal.h>
#include <time.h>

#include <unix/uuprocess.h>
#include <unix/uusignal.h>

extern struct utsname phantom_uname;

#include <hal.h>

#include "../misc.h"
#include <thread_private.h>

#define adjustin( uaddr, __st__ ) ( ((void*)uaddr) + ((int)u->mem_start))

//static void *adjustin( int uaddr, struct trap_state *st );
errno_t copyout( unsigned useraddr, unsigned mina, unsigned maxa, void *src, int len );
errno_t user_args_load( int mina, int maxa, char **oav, int omax,  const char **iav );

#define CHECKA(a,count) do { \
    unsigned int __addr = (int)(a); \
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
    unsigned int __addr = (int)(var); \
    if( __addr < mina || (__addr+size) >= maxa ) \
        { \
        ret = -1; \
    	err = EFAULT; \
        goto err_ret; \
        } \
    } while(0)


#define CHECK_CAP(CAP) \
    do { \
    if( (u->uid != 0) && (u->euid != 0) ) \
    { \
        ret = -1; \
    	err = EPERM; \
        goto err_ret; \
    } \
    } while(0)
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




static void do_syscall_sw( uuprocess_t *u, struct trap_state *st)
{
#if HAVE_UNIX
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


    //phantom_thread_t *t = GET_CURRENT_THREAD();
    //uuprocess_t *u = t->u;

    /*
    tid_t tid = get_current_tid();
    pid_t pid;
    assert( !t_get_pid( tid, &pid ));
    uuprocess_t *u = proc_by_pid(pid);
    */

    assert( u != 0 );

    unsigned int mina = (int)u->mem_start, maxa = (int)u->mem_end;

    // trap state is good for interrupt.
    // call gate does not push eflags
    int user_esp = st->eflags;

#ifndef ARCH_ia32
#  warning machdep ia32 arg pass convention
#endif
    // list of user syscsall arguments
    int *uarg = adjustin( user_esp, st );
    uarg++; // syscall func return addr

    //SHOW_FLOW( 10, "Syscall pid %2d tid %2d, our esp %p, user esp %p, t kstack %p", u->pid, t->tid, &st, user_esp, t->kstack_top );
    SHOW_FLOW( 8, "Syscall %d args %x, %x, %x", callno, uarg[0], uarg[1],uarg[2] );

    int ret = 0;
    int err = 0;


    switch(callno)
    {
    case SYS__exit:
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
        //SHOW_FLOW( 2, "sleepmsec %d", uarg[0] );
        hal_sleep_msec(uarg[0]);
        break;



    case SYS_getpagesize:       ret = PAGE_SIZE; break;

    case SYS_personality:
        if( ((unsigned) uarg[0]) == 0xffffffff)
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

    case SYS_gettid:            ret = get_current_tid(); break;
    case SYS_getpid:            ret = u->pid; break;
    case SYS_getpgrp:           ret = u->pgrp_pid; break;
    case SYS_getppid:           ret = u->ppid; break;

    case SYS_getpgid:
        goto unimpl;

    case SYS_time:
        {
            time_t t = time(0);

            AARG(time_t *, tp, 0, sizeof(time_t));

            *tp = t;
            ret = t;
            break;
        }
    case SYS_nanosleep:
        // todo check for POSIX compliance, make it interruptible by signals
        //int nanosleep(const struct timespec *, struct timespec *);
        {
            goto unimpl;
            //AARG(struct timespec *,  req_time, 0, sizeof(struct timespec));
            //AARG(struct timespec *, rest_time, 1, sizeof(struct timespec));
            //ret = usys_nanosleep( &err, u, req_time, rest_time );
            //break;
        }


    case SYS_sync:
    case SYS_sysinfo:
    case SYS_sysfs:
    case SYS_klogctl:

    case SYS_shutdown:
    case SYS_reboot:



    case SYS_getitimer:
    case SYS_setitimer:
    case SYS_gettimeofday:


    case SYS_setuid:
    case SYS_setuid32:          CHECK_CAP(CAP_SETUID); u->euid = uarg[0]; break;

    case SYS_setgid:            
    case SYS_setgid32:          CHECK_CAP(CAP_SETGID); u->egid = uarg[0]; break;

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
            ret = usys_read(&err, u, uarg[0], addr, count );
            break;
        }
    case SYS_write:
        {
            int count = uarg[2];
            void *addr = adjustin( uarg[1], st );
            CHECKA(addr,count);
            ret = usys_write(&err, u, uarg[0], addr, count );
            break;
        }
    case SYS_lseek: ret = usys_lseek(&err, u, uarg[0], uarg[1], uarg[2] ); break;
    case SYS_creat: ret = usys_creat(&err, u, adjustin( uarg[0], st ), uarg[1] ); break;
    case SYS_chdir:
        {
            AARG(const char *, path, 0, 0);
            ret = usys_chdir(&err, u, path );
            break;
        }

    case SYS_pipe:
        {
            AARG(int *, fds, 0, sizeof(int) * 2);
            ret = usys_pipe( &err, u, fds );
            break;
        }

    case SYS_rmdir:
    case SYS_unlink:
        {
            AARG(const char *, name, 0, 1);
            ret = usys_rm( &err, u, name );
            break;
        }


    case SYS_dup:
        ret = usys_dup( &err, u, uarg[0] );
        break;

    case SYS_dup2:
        ret = usys_dup2( &err, u, uarg[0], uarg[1] );
        break;

    case SYS_symlink:
        {
            AARG(const char *, n1, 0, 1);
            AARG(const char *, n2, 1, 1);
            ret = usys_symlink( &err, u, n1, n2 );
            break;
        }

    case SYS_getcwd:
        {
            AARG(char *, buf, 0, uarg[1]);
            ret = usys_getcwd( &err, u, buf, uarg[1] );
            break;
        }

    case SYS_mkdir:
        {
            AARG(const char *, path, 0, 2);
            ret = usys_mkdir( &err, u, path );
            break;
        }

    case SYS_link:
    case SYS__llseek:
    case SYS_chroot:
    case SYS_lstat64:
    case SYS_mknod:
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
        {
            //AARG(void *, str, 2, sizeof(int)); // FIXME size of arg?
            ret = usys_fcntl( &err, u, uarg[0], uarg[1], uarg[0] );
        }
        break;

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

            unsigned int onerc;
            int lp;

            for( lp = 0; lp < iovcnt; lp++ )
            {
                void *addr = adjustin( list[lp].iov_base, st );
                CHECKA(addr,list[lp].iov_len);
                onerc = usys_read(&err, u, fd, addr, list[lp].iov_len );

                /*
                if( onerc < 0 )
                {
                    ret = -1;
                    goto err_ret;
                }
                */
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

            unsigned int onerc;
            int lp;

            for( lp = 0; lp < iovcnt; lp++ )
            {
                void *addr = adjustin( list[lp].iov_base, st );
                CHECKA(addr,list[lp].iov_len);
                onerc = usys_write(&err, u, fd, addr, list[lp].iov_len );

                /*
                if( onerc < 0 )
                {
                    ret = -1;
                    goto err_ret;
                }
                */
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
        {
            AARG( socklen_t *, len, 2, sizeof(socklen_t));
            if( len  && uarg[0] && uarg[1] )
            {
                AARG( struct sockaddr *, acc_addr, 1, *len );
                ret = usys_accept( &err, u, uarg[0], acc_addr, len );
            }
            else
                ret = usys_accept( &err, u, uarg[0], 0, 0 );
            break;
        }

    case SYS_recv:
        {
            int len = uarg[2];
            AARG( void *, buf, 0, len );
            ret =  usys_recv( &err, u, uarg[0], buf, len, uarg[3] );
            break;
        }

    case SYS_recvmsg:
        {
            AARG( struct msghdr *, msg, 0, sizeof(struct msghdr) );
            ret = usys_recvmsg( &err, u, uarg[0], msg, uarg[2] );
            break;
        }

    case SYS_send:
        {
            int len = uarg[2];
            AARG( const void *, buf, 0, len );
            ret =  usys_send( &err, u, uarg[0], buf, len, uarg[3] );
            break;
        }

    case SYS_sendmsg:
        {
            AARG( const struct msghdr *, msg, 0, sizeof(struct msghdr) );
            ret = usys_sendmsg( &err, u, uarg[0], msg, uarg[2] );
            break;
        }

    case SYS_sendto:
        {
            socklen_t tolen = uarg[5];
            AARG( const struct sockaddr *, to, 4, tolen );
            int len = uarg[2];
            AARG( const void *, buf, 0, len );
            ret =  usys_sendto( &err, u, uarg[0], buf, len, uarg[3], to, tolen );
            break;
        }

    case SYS_recvfrom:
        {
            AARG( socklen_t *, fromlen, 5, sizeof(socklen_t) );
            AARG( struct sockaddr *, from, 4, *fromlen );
            int len = uarg[2];
            AARG( void *, buf, 0, len );
            ret =  usys_recvfrom( &err, u, uarg[0], buf, len, uarg[3], from, fromlen );
            break;
        }



    case SYS_connect:
        // int connect(int socket, const struct sockaddr *address, socklen_t address_len);
        {
            int len = uarg[2];
            AARG( struct sockaddr *, acc_addr, 1, len );
            ret = usys_connect( &err, u, uarg[0], acc_addr, len );
            break;
        }

    case SYS_socketpair:
        {
            AARG( int *, sv, 3, sizeof(int) * 2 );
            ret = usys_socketpair( &err, u, uarg[0], uarg[1], uarg[2], sv );
            break;
        }




    case SYS_sendfile:
    case SYS_socketcall:
        goto unimpl;

    case SYS_nice:
        {
            // int nice = uarg[0];
            // set thr prio
            // break;
            goto unimpl;
        }

    case SYS_brk:
        goto unimpl;
        /*{
            ret = usys_sbrk( &err, u, uarg[0] );
            break;
        }*/

    case SYS_fork:
    case SYS_vfork:
        goto unimpl;

    case SYS_ioctl:
        {
            void *data = adjustin( uarg[2], st );
            CHECKA(data,0);
            ret = usys_ioctl( &err, u, uarg[0], uarg[1], data, uarg[3] );
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
            break;
        }

    case SYS_kill:
        {
            ret = usys_kill( &err, u, uarg[0], uarg[1] );
            break;
        }

    case SYS_waitpid:
        {
            AARG(int *, addr, 1, sizeof(int));
            ret = usys_waitpid( &err, u, uarg[0], addr, uarg[2] );
            break;
        }

    case SYS_wait:
        {
            AARG(int *, addr, 0, sizeof(int));
            ret = usys_waitpid( &err, u, -1, addr, 0 );
            break;
        }


    case SYS_clone:
        goto unimpl;


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
        goto unimpl;


        // NewOS/BeOS/Haiku
    case SYS_create_port:
        {
            // TODO check string length and final addr to be valid
            AARG(const char *, name, 1, 0);
            ret = port_create( uarg[0], name );
            break;
        }
    case SYS_close_port:
        ret = port_close( uarg[0] );
        break;
    case SYS_delete_port:
        ret = port_delete( uarg[0] );
        break;
    case SYS_find_port:
        {
            // TODO check string length and final addr to be valid
            AARG(const char *, name, 0, 0);
            ret = port_find(name);
            break;
        }
    case SYS_get_port_info:
        {
            AARG(struct port_info *, info, 1, sizeof(struct port_info));
            ret = port_get_info( uarg[0], info);
        }
    case SYS_get_port_bufsize:
        ret = port_buffer_size( uarg[0] );
        break;
    case SYS_get_port_bufsize_etc:
        ret = port_buffer_size_etc( uarg[0], uarg[1], uarg[2] );
        break;
    case SYS_get_port_count:
        ret = port_count( uarg[0] );
        break;
    case SYS_read_port:
        {
            AARG(int32_t *, msg_code, 1, sizeof(int32_t));
            AARG(void  *, msg_buffer, 2, uarg[3]);
            ret = port_read( uarg[0],
                             msg_code,
                             msg_buffer,
                             uarg[3]);
            break;
        }
    case SYS_write_port:
        {
            //AARG(int32_t *, msg_code, 1, sizeof(int32_t));
            //AARG(int32_t, msg_code, 1, sizeof(int32_t));
            AARG(void  *, msg_buffer, 2, uarg[3]);
            ret = port_write( uarg[0],
                             uarg[1],
                             msg_buffer,
                             uarg[3]);
            break;
        }
    case SYS_read_port_etc:
    case SYS_write_port_etc:
    case SYS_set_port_owner:
    case SYS_get_next_port_info:
        goto unimpl;

    case SYS_phantom_run:
        {
            // extern int phantom_run(const char *fname, const char **argv, const char **envp, int flags);
            AARG(const char  *, fname, 0, 1);

            AARG(const char **, uav,   1, 4);
            AARG(const char **, uep,   2, 4);

            if( 0 == uarg[1] ) uav = 0;
            if( 0 == uarg[2] ) uep = 0;

            SHOW_FLOW( 2, "run %s flags 0x%b", fname, uarg[3], "\020\1<WAIT>\2<NEWWIN>\3<NEWPGRP>" );

            char *a[1024];
            char *e[1024];

            SHOW_FLOW( 2, "run %s load args", fname );

            if(
               user_args_load( mina, maxa, a, 1024, uav ) ||
               user_args_load( mina, maxa, e, 1024, uep )
              )
            {
                ret = -1;
                err = EFAULT;
                SHOW_ERROR( 0, "fault reading args for %s", fname );
                goto err_ret;
            }

            ret = usys_run( &err, u, fname, (const char**)a, (const char**)e, uarg[3] );
            break;
        }

    case SYS_phantom_method:
        // AARG(const char *, m_name, 0, 1);
        // int nfd = aarg[1];
        // AARG(int *, fds, 0, sizeof(int)*nfd);

        // ret = usys_pmethod( &err, u, m_name, int nfd, int fds[] );

    case SYS_phantom_toobject:
    case SYS_phantom_fromobject:
    case SYS_phantom_intmethod:
    case SYS_phantom_strmethod:
        goto unimpl;

        // extern int phantom_runclass(const char *cname, int nmethod, int flags);
    case SYS_phantom_runclass:
        {
            AARG(const char  *, cname, 0, 1);
            unsigned flags = uarg[2];

            if(flags)
                SHOW_ERROR( 0, "SYS_phantom_runclass: unknown flags %x" , flags );

            usys_phantom_runclass( &err, u, cname, uarg[1] );
            ret = err;
        }
        break;

    case SYS_setproperty:
        {
            AARG(const char *, pName, 1, 1); // TODO check zero term string!
            AARG(const char *, pValue, 2, 1);

            usys_setproperty( &err, u, uarg[0], pName, pValue );
            ret = err;
        }
        break;

    case SYS_getproperty:
        {
            AARG(const char *, pName, 1, 1); // TODO check zero term string!
            AARG(char *, pValue, 2, uarg[3]);

            usys_getproperty( &err, u, uarg[0], pName, pValue, uarg[3] );
            ret = err;
        }
        break;

    case SYS_listproperties:
        {
            AARG(char *, buf, 2, uarg[3]);
            usys_listproperties( &err, u, uarg[0], uarg[1], buf, uarg[3] );
            ret = err;
        }
        break;


    case SYS_name2ip:
        {
            AARG(in_addr_t *, out, 0, sizeof(in_addr_t));
            AARG(const char *, name, 1, 1);

            ret = name2ip( out, name, uarg[2] );
            if( ret != 0 ) err = ret;
        }
        break;


    case SYS_sigpending:
        {
            AARG(sigset_t *, set, 0, sizeof(sigset_t *));
            ret = usys_sigpending( &err, u, set);
            break;
        }

    case SYS_signal:
        {
#if 0
            AARG(sighandler_t, hand, 1, sizeof(sighandler_t));
            hand = usys_signal( &err, u, uarg[0], hand);
            // FIXME 64 bit error
            ret = ((int)hand) - mina; // Convert pointer back
#else
            // We do not use pointer (ret and uarg 1), so we don't have to convert it
            ret = (int)usys_signal( &err, u, uarg[0], (void *)uarg[1]);
#endif
            break;
        }

    case SYS_sigprocmask:
    //case raise:
    case SYS_sigaction:
    case SYS_siginterrupt:
    case SYS_sigsuspend:
        goto unimpl;


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
    int err = ENOSYS;
    int ret = -1;
    goto err_ret; // to clear warning
#endif // HAVE_UNIX

err_ret:

#ifdef ARCH_ia32
#define _RET_OK
    st->eax = ret;
    st->edx = err;
#endif

#ifdef ARCH_mips
#define _RET_OK
    st->r2 = ret; // v0 (normal ret register, low)
    st->r3 = err; // v1 (normal ret register, hi)
#endif

#ifdef ARCH_arm
#define _RET_OK
    st->r0 = ret; // normal ret register
    st->r1 = err; // arg1 reg, we can safely use for return of errno
#endif

#ifndef _RET_OK
#error arch ret
#endif

}






// TODO no interlock - process can be klled when we use it - use pool for processes!
void syscall_sw(struct trap_state *st)
{
    //phantom_thread_t *t = GET_CURRENT_THREAD();
    //uuprocess_t *u = t->u;

    tid_t tid = get_current_tid();
    pid_t pid;
    assert( !t_get_pid( tid, &pid ));
    uuprocess_t *u = proc_by_pid(pid);

/*
    int ret;
    if( (ret = setjmp(u->signal_jmpbuf)) )
    {
        execute_signals(u, st);
        return;
    }
*/
    do_syscall_sw( u, st );
    execute_signals(u, st);
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

    if( (unsigned int)dest < mina || (unsigned int)dest > maxa )
        return EFAULT;

    memcpy( dest, src, len );
    return 0;
}



errno_t user_args_load( int mina, int maxa, char **oav, int omax,  const char **iav )
{
    omax--; // leave one for zero ptr
    while( omax-- )
    {
        if( (0 == iav) || (0 == *iav) )
            break;
        // todo 64 bit bug mina/maxa must be 64 bits
        // do (long for 64 bits) integer ops for else
        // iav will be incremented by mina * sizeof(void*)
        *oav = (void *) (((long int)(*iav)) + mina);
        if( ((long int)*oav) > maxa )
            return EFAULT;

        SHOW_FLOW( 7, "argv %s", *oav );

        oav++;
        iav++;
    }

    *oav = 0;

    return 0;
}

#endif // HAVE_UNIX
