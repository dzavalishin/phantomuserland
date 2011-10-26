#include <errno.h>

#include <compat/tabos.h>
#include <user/sys_fio.h>

int main( int argc, char **argv ) {
	int nIndex;

	if ( argc != 1 ) {
		Print( "Syntax: shutdown\n" );
		return -EINVAL;
	}

	Shutdown();

	return ESUCCESS;
}
