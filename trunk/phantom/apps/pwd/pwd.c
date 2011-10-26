#include <sys/errno.h>
#include <unistd.h>
#include <stdio.h>

#include <compat/tabos.h>
#include <user/sys_fio.h>

#define PATH_MAX	2048

/**
 * \brief Simple print working directory program
 * \todo Replace 4096 with a define!
 */
int main( int argc, char **argv ) {
	char anCwd[ PATH_MAX ];

	GetCurrentWorkingDirectory( anCwd, PATH_MAX );
	Print( "%s\n", anCwd );

	return ESUCCESS;
}
