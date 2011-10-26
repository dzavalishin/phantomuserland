#include <sys/errno.h>
#include <stdio.h>

#include <compat/tabos.h>
#include <user/sys_fio.h>

int main( int argc, char **argv ) 
{

	Sync( 0 );

	return ESUCCESS;
}
