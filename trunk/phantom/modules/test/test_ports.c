#include <sys/unistd.h>
#include <user/sys_port.h>
#include <phantom_libc.h>

#include "test.h"


#define BSIZE 256


int do_test_ports(const char *test_parm)
{
    (void) test_parm;

    int	id = find_port("__regress_test_port");
    if( id < 0 )
    {
        ssyslog( 0, "userland regress test port NOT found" );
        return ENOENT;
    }

    ssyslog( 0, "userland regress test port found" );

    int rc;
    int32_t msg_code;
    char buf[BSIZE];

    rc = read_port(id, &msg_code, &buf, BSIZE);
    if( rc < 0 )
        return EIO;

    rc = write_port(id, 0xAA, &buf, BSIZE);

    //TEST(usermode);

    rc = write_port(id, 0x55, &buf, BSIZE);


    if( close_port( id ) )
    {
        ssyslog( 0, "can't close regress test port" );
    }

    return 0;
}

