#ifndef RESOLV_H
#define RESOLV_H

#include <errno.h>
#include <netinet/in.h>
#include <kernel/net.h>

// Give random (not allways first) address - not impl
#define RESOLVER_FLAG_RANDOM    (1<<0)

// Give answer from cache or fail, no block
#define RESOLVER_FLAG_NOWAIT    (1<<1)

// Try to request just once, don't waste time
#define RESOLVER_FLAG_NORETRY   (1<<2)

// Don't try to lookup in cache, allways reach DNS
#define RESOLVER_FLAG_NORCACHE  (1<<3)

// Don't pollute cache with this result
#define RESOLVER_FLAG_NOWCACHE  (1<<4)


errno_t name2ip( in_addr_t *out, const char *name, int flags );

errno_t dns_server_add( ipv4_addr a );
errno_t dns_server_remove( ipv4_addr a );


#endif // RESOLV_H
