#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>

#include <compat/tabos.h>
#include <user/sys_fio.h>

int doRemoveDirectory( char *pnName ) {
    struct sStatus sStatus;

    if ( Status( pnName, &sStatus ) ) {
        Print( "cannot stat the selected entry\n" );
        return -EFAIL;
    }
    if ( !S_ISDIR( sStatus.nMode ) ) {
        Print( "argument is not a directory\n" );
        return -EINVAL;
    }
    if ( RemoveDirectory( pnName ) ) {
        Print( "cannot remove the selected directory\n" );
        return -EFAIL;
    }
    return ESUCCESS;
}

int main( int argc, char **argv ) {
    int nIndex;

    if ( argc == 1 ) {
        Print( "Syntax: rmdir [DIR] ...\n" );
        return -EINVAL;
    }

    for ( nIndex = 1; nIndex < argc; nIndex++ ) {
        doRemoveDirectory( argv[ nIndex ] );
    }
    return ESUCCESS;
}
