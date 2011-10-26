#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
//#include <mm.h>

#include <compat/tabos.h>
#include <user/sys_fio.h>

int main( int argc, char **argv )
{
    struct sStatus sb;
    int ret;

    if ( argc < 3 ) {
        Print( "Syntax: mv [src] ... [dest]\n" );
        return -EINVAL;
    }

    /*
     * If moving a bunch of files to a directory, last element has to be
     * a directory.
     */
    Print( "stat: %s\n", argv[ argc - 1 ] );
    if ( Status( argv[ argc - 1 ], &sb ) < 0 ) {
        /* a, b, c -> d		But d doesn't exis */
        if ( argc > 3 ) {
            Print( "Last argument should be dir: %s\n", argv[ argc - 1 ] );
            return -EINVAL;
        }
        /* a -> b */
        Print( "stat: %s\n", argv[ 1 ] );
        if ( Status( argv[ 1 ], &sb ) < 0 ) {
            Print( "source file does not exist!\n" );
            return -EINVAL;
        }
        Print( "rename: %s -> %s\n", argv[ 1 ], argv[ 2 ] );
        if ( Rename( argv[ 1 ], argv[ 2 ] ) < 0 ) {
            Print( "cannot rename file!\n" );
            return -EFAIL;
        }
        return -EFAIL;
    }

    /* If destination is a directory, move each into it */
    ret = 0;
    if ( ( sb.nMode & S_IFMT ) == S_IFDIR ) {
        int x, len;
        char *base;

        argc -= 1;
        base = argv[ argc ];
        len = StringLength( base ) + 1;
        for ( x = 0; x < argc; ++x ) {
            char *dest;

            size_t dlen = len + StringLength( argv[ x ] + 1 );
            dest = ( char * ) MemoryAllocation( dlen );
            StringPrint( dest, dlen, "%s/%s", base, argv[ x ] );
            Print( "rename: %s -> %s\n", argv[ x ], dest );
            /*if ( rename( argv[ x ], dest ) < 0 ) {
             Print( "cannot rename file!\n" );
             return -EFAIL;
             }*/
        }
    } else {
        /* Existing destination -- overwrite if a -> b, otherwise error */
        if ( argc != 2 ) {
            Print( "Last argument should be dir: %s\n", argv[ argc - 1 ] );
            return -EINVAL;
        }
        Print( "unlink: %s\n", argv[ 1 ] );
        Unlink( argv[ 1 ] );
        Print( "rename: %s -> %s\n", argv[ 0 ], argv[ 1 ] );
        /*if ( rename( argv[ 0 ], argv[ 1 ] ) < 0 ) {
         Print( "cannot rename file!\n" );
         ret = 1;
         }*/
    }

    return ret ? 1 : 0;
}
