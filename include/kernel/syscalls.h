/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Unix subsystem. Syscall numbers. Don't you ever change.
 *
 *
**/

/**
 * \ingroup Unix
 * @{
**/

// -+ = +- syscall func. but have in kernel
// +- = have syscall func. but +- impl in kernel
// ++ = OK
// +! = have in kernel, but impl is temporary or stub

// in general.S
#define SYS__exit               0	// ++
#define SYS_ssyslog             1	// ++
#define SYS_getpagesize         2	// ++
#define SYS_personality         3	// +!
#define SYS_uname               4	// ++ - test!
#define SYS_sync                5	// +-
#define SYS_sysinfo             6	// +-
#define SYS_sysfs               7	// +-
#define SYS_klogctl             8	// +-
#define SYS_shutdown            9	// +-
#define SYS_reboot              10	// +-



#define SYS_sleepmsec           16	// ++
#define SYS_nanosleep           17	// +-
#define SYS_time                18	// +-
#define SYS_stime               19	// +-
#define SYS_getitimer           20	// +-
#define SYS_setitimer           21	// +-
#define SYS_gettimeofday        22	// +-



// in getset.S
#define SYS_gettid              32	// ++
#define SYS_getpid              33	// ++
#define SYS_getuid              34	// ++
#define SYS_getuid32            35	// ++
#define SYS_getpgid             36	// ++
#define SYS_getpgrp             37	// ++
#define SYS_getppid             38	// ++
#define SYS_getegid             39	// ++
#define SYS_getegid32           40	// ++
#define SYS_geteuid             41	// ++
#define SYS_geteuid32           42	// ++
#define SYS_getgid              43	// ++
#define SYS_getgid32            44	// ++

#define SYS_setgid              45	// +-
#define SYS_setgid32            46	// +-
#define SYS_setgroups           47	// +-
#define SYS_setgroups32         48	// +-
#define SYS_setpgid             49	// +-
#define SYS_setregid            50	// +-
#define SYS_setregid32          51	// +-
#define SYS_setresgid           52	// +-
#define SYS_setresgid32         53	// +-
#define SYS_setresuid           54	// +-
#define SYS_setresuid32         55	// +-
#define SYS_setreuid            56	// +-
#define SYS_setreuid32          57	// +-
#define SYS_setuid              58	// +-
#define SYS_setuid32            59	// +-
#define SYS_setpgrp             60	// ++






// in file.S
#define SYS_open                64	// ++
#define SYS_close               65	// ++
#define SYS_read                66	// ++
#define SYS_write               67	// ++
#define SYS_lseek               68	// ++
#define SYS_creat               69	// +!
#define SYS_chdir               70	// ++
#define SYS_link                71	// +-
#define SYS_unlink              72	// +-
#define SYS__llseek             73	// +-
#define SYS_chroot              74	// +-
#define SYS_mkdir               75	// +-
#define SYS_rmdir               76	// +-
#define SYS_dup                 77	// +-
#define SYS_dup2                78	// +-
#define SYS_getcwd              79	// +-
#define SYS_lstat               80	// +!
#define SYS_lstat64             81	// +-
#define SYS_mknod               82	// +-
#define SYS_pipe                83	// +-
#define SYS_symlink             84	// +-

#define SYS_mount               85	// +!
#define SYS_umount              86	// ++ - doesnt check usage

#define SYS_truncate            87	// ++
#define SYS_truncate64          88	// +-
#define SYS_fchdir              89	// ++
#define SYS_fchmod              90	// +!
#define SYS_fchown              91	// +!
#define SYS_fchown32            92	// +!
#define SYS_fcntl               93	// +-
#define SYS_fcntl64             94	// +-
#define SYS_fdatasync           95	// +-
#define SYS_flock               96	// +-
//#define SYS_                97	// +-
#define SYS_fstat               98	// ++
#define SYS_fstat64             99	// +-
#define SYS_fstatfs             100	// +-
#define SYS_fsync               101	// +-
//#define SYS_ftime              102	// +-
#define SYS_ftruncate           103	// ++
#define SYS_ftruncate64         104	// +-
#define SYS_utime               105	// +-
#define SYS_chmod               106	// +-
#define SYS_chown               107	// +-
#define SYS_chown32             108	// +-
#define SYS_access              109	// +-
#define SYS_lchown              110	// +-
#define SYS_lchown32            111	// +-
#define SYS_pread               112	// +-
#define SYS_pwrite              113	// +-
#define SYS_readv               114	// ++
#define SYS_writev              115	// ++
#define SYS_readlink            116	// +-

#define SYS_readdir             117	// +! no impl in fsystems
#define SYS_stat                118	// ++

// Specific for Phantom - like ioctl, but property like
// setproperty( "samplerate", "44100" );
#define SYS_getproperty         119	// --
#define SYS_setproperty         120	// --
#define SYS_listproperties      121	// --




// in net.S
#define SYS_gethostname         128	// +! No access from obj code, not persisted
#define SYS_sethostname         129	// +!

#define SYS_socket              130	// ++
#define SYS_bind                131	// ++
#define SYS_accept              132	// +-
#define SYS_connect             133	// +-
#define SYS_listen              134	// ++
#define SYS_recv                135	// +-
#define SYS_recvfrom            136	// +-
#define SYS_recvmsg             137	// +-
#define SYS_send                138	// +-
#define SYS_sendto              139	// +-
#define SYS_sendmsg             140	// +-
#define SYS_sendfile            141	// +-
#define SYS_setsockopt          142	// +!
#define SYS_socketcall          143	// +-
#define SYS_socketpair          144	// +-
#define SYS_getpeername         145	// +!
#define SYS_getsockname         146	// +!
#define SYS_getsockopt          147	// +!


#define SYS_name2ip             180	// +?


// in general.S
#define SYS_brk                 256	// +-
#define SYS_fork                257	// +-
#define SYS_vfork               258	// +-
#define SYS_nice                259	// +-
#define SYS_ioctl               260	// +!
#define SYS_kill                261	// +-
//#define SYS_pipe                262	// +-
#define SYS_umask               263	// ++
#define SYS_clone               264	// +-

#define SYS_wait                265	// +-
#define SYS_waitpid             266 // +-


#define SYS_sigpending          267 // --
#define SYS_sigprocmask         268 // --
#define SYS_raise               269 // --
#define SYS_sigaction           270 // --

#define SYS_signal              271 // --

#define SYS_siginterrupt        272 // --
#define SYS_sigsuspend          273 // --




// in mmap.S
#define SYS_madvise             512	// +-
#define SYS_mincore             513	// +-
#define SYS_mlock               514	// +-
#define SYS_mlockall            515	// +-
#define SYS_mmap                516	// +-
#define SYS_mprotect            517	// +-
#define SYS_mremap              518	// +-
#define SYS_msync               519	// +-
#define SYS_munlock             520	// +-
#define SYS_munlockall          521	// +-
#define SYS_munmap              522	// +-




// in newos.S

#define SYS_create_port         1024	// +-
#define SYS_close_port          1025	// +-
#define SYS_delete_port         1026	// +-
#define SYS_find_port           1027	// ++
#define SYS_get_port_info      	1028	// +-
#define SYS_get_port_bufsize   	1029	// +-
#define SYS_get_port_bufsize_etc		1030	// +-
#define SYS_get_port_count     	1031	// +-
#define SYS_read_port           1032	// ++
#define SYS_read_port_etc      	1033	// +-
#define SYS_write_port          1034	// ++
#define SYS_write_port_etc      1035	// +-
#define SYS_set_port_owner     	1036	// +-
#define SYS_get_next_port_info	1037	// +-
//#define SYS		1038	// +-
//#define SYS		1039	// +-




// in phantom.S

#define SYS_phantom_run         2048	// +-
#define SYS_phantom_method      2049
#define SYS_phantom_toobject    2050
#define SYS_phantom_fromobject  2051
#define SYS_phantom_intmethod   2052
#define SYS_phantom_strmethod   2053


#define SYS_phantom_runclass    2096

