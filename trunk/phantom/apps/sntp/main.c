#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>

#include "main.h"

// TODO move to hdr
#define IPV4_DOTADDR_TO_ADDR(a, b, c, d) \
	(((in_addr_t)(a) << 24) | (((in_addr_t)(b) & 0xff) << 16) | (((in_addr_t)(c) & 0xff) << 8) | ((in_addr_t)(d) & 0xff))


int main(int ac, char **av)
{
    (void) ac;
    (void) av;

    u_int32_t server_addr = IPV4_DOTADDR_TO_ADDR(85,21,78,91);
    u_int32_t interval = 20000;

    SNTP_resync( server_addr, interval );

    return 0;
}