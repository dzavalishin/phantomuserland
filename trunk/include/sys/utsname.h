#ifndef UTSNAME_H
#define UTSNAME_H

#define _UTSNAME_LENGTH 257

#define _UTSNAME_SYSNAME_LENGTH 	_UTSNAME_LENGTH
#define _UTSNAME_NODENAME_LENGTH 	_UTSNAME_LENGTH
#define _UTSNAME_RELEASE_LENGTH 	_UTSNAME_LENGTH
#define _UTSNAME_VERSION_LENGTH 	_UTSNAME_LENGTH
#define _UTSNAME_MACHINE_LENGTH 	_UTSNAME_LENGTH
#define _UTSNAME_DOMAIN_LENGTH		_UTSNAME_LENGTH

struct utsname
  {
    /* Name of the implementation of the operating system.  */
    char sysname[_UTSNAME_SYSNAME_LENGTH];

    /* Name of this node on the network.  */
    char nodename[_UTSNAME_NODENAME_LENGTH];

    /* Current release level of this implementation.  */
    char release[_UTSNAME_RELEASE_LENGTH];
    /* Current version level of this release.  */
    char version[_UTSNAME_VERSION_LENGTH];

    /* Name of the hardware type the system is running on.  */
    char machine[_UTSNAME_MACHINE_LENGTH];

    char domainname[_UTSNAME_DOMAIN_LENGTH];
  };

# define SYS_NMLN  _UTSNAME_LENGTH

#ifndef __THROW
#  define __THROW
#endif

/* Put information about the system in NAME.  */
extern int uname (struct utsname *__name) __THROW;


#endif // UTSNAME_H
