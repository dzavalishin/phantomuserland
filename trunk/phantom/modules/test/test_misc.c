#include <user/sys_misc.h>
#include <sys/utsname.h>
#include <phantom_libc.h>

#include "test.h"

#define BSIZE 256

int do_test_misc(const char *test_parm)
{
    (void) test_parm;

    char buf[BSIZE];
    int rc;

    test_check_eq(getpagesize(),4096);

    test_check_eq(personality(0xffffffff), 0);

    {
    struct utsname phantom_uname;
    test_check_eq(uname(&phantom_uname), 0);

    printf("uland: uname sys '%s'\n\tnode    %s\n\trel    %s\n\tver     %s\n\tmachine %s\n\tdomain  %s\n",
           phantom_uname.sysname,
           phantom_uname.nodename,
           phantom_uname.release,
           phantom_uname.version,
           phantom_uname.machine,
           phantom_uname.domainname
          );
    }

    rc = gethostname(buf, BSIZE);
    test_check_ge(rc,0);
    printf("uland: hostname %s\n", buf );


    return 0;
}
