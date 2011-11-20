
//#include <pro/sntp.h>
//#include <sys/socket.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/resolv.h>
#include <arpa/inet.h>


#define RESOLVE 0

#define errno_t int

static errno_t SNTPGetTime(u_int32_t * server_adr, time_t * t);



typedef struct _sntpframe sntpframe;
struct _sntpframe {
    u_int8_t mode;
    u_int8_t stratum;
    u_int8_t poll;
    u_int8_t precision;
    u_int32_t root_delay;
    u_int32_t root_dispersion;
    u_int32_t reference_identifier;
    u_int32_t reference_ts_sec;
    u_int32_t reference_ts_frac;
    u_int32_t originate_ts_sec;
    u_int32_t originate_ts_frac;
    u_int32_t receive_ts_sec;
    u_int32_t receive_ts_frac;
    u_int32_t transmit_ts_sec;
    u_int32_t transmit_ts_frac;
};


#define NTP_PORT	123
#define SNTP_PORT NTP_PORT

struct SNTP_resync_args {
    u_int32_t server_addr;
    u_int32_t interval;
};


static char * dumpt(time_t t)
{
    static char buf[100] = "?";
    struct tm time;

    printf("tm b = '%s'\n", buf);

    localtime_rb( t, &time );
    asctime_r( &time, buf, sizeof(buf) );

    printf("tm b = '%s'\n", buf);

    return buf;
}

static void SNTP_resync(void * arg)
{
    u_int32_t server_addr = ((struct SNTP_resync_args *) arg)->server_addr;
    u_int32_t interval = ((struct SNTP_resync_args *) arg)->interval;
    u_int32_t cur_server_addr = server_addr;
    int retry = 0;
    time_t t;

    hal_set_thread_name("sntpd");
    //hal_set_current_thread_priority(PHANTOM_SYS_THREAD_PRIO);

    for (;;)
    {
        SHOW_FLOW( 7, "request time from %s", inet_itoa(server_addr) );
        if (SNTPGetTime(&cur_server_addr, &t))
        {     /* if any error retry */
            if (cur_server_addr != server_addr && server_addr == 0xFFFFFFFF)
            {
                cur_server_addr = server_addr;
                continue;
            }

            retry++;
#if RESOLVE
            if(retry == 2 )
            {
                in_addr_t out;
                char *host = "pool.ntp.org";

                SHOW_FLOW( 7, "resolve again %s", host );
                if( !name2ip( &out, host, RESOLVER_FLAG_NORCACHE ) )
                {
                    SHOW_FLOW( 7, "got new addr %s", inet_itoa(server_addr) );
                    server_addr = out;
                }
            }
#endif
            if(retry >= 3)
            { /* if numer of retries >= 3 wait 30 secs before next retry sequence ... */
                retry = 0;
                hal_sleep_msec(30000);
            } else              /* ... else wait 5 secs for next retry */
                hal_sleep_msec(5000);
        } else {                /* no error */
            //SHOW_FLOW( 7, "got time %ld, %s", t, dumpt(t) );

            set_time(t);          /* so set the time */
            retry = 0;
            hal_sleep_msec(interval); /* and wait the interval time */
        }
    }
}

static errno_t SNTPGetTime(u_int32_t * server_adr, time_t * t)
{
    /*first check the pointers */
    //u_int32_t rec_addr;
    void *sock = NULL;     /* the udp socket */
    sntpframe data;            /* we're using the heap to save stack space */
    //u_int16_t port;               /* source port from incoming packet */
    int len;
    errno_t result = EIO;

    /* Set UDP input buffer to 256 bytes */
    //u_int16_t bufsize = 256;

    sockaddr addr, raddr;

    addr.port = SNTP_PORT;
    addr.addr.len = 4;
    addr.addr.type = ADDR_TYPE_IP;
    NETADDR_TO_IPV4(addr.addr) = *server_adr;


    if (t == NULL)
        return -1;

    if (server_adr == NULL)
        return -1;

    if( udp_open(&sock) )
    {
        SHOW_ERROR0( 0, "UDP - can't prepare endpoint");
        return EIO;
    }

    //NutUdpSetSockOpt(sock, SO_RCVBUF, &bufsize, sizeof(bufsize));

    data.mode = 0x1B;          /* LI, VN and Mode bit fields (all in u_char mode); */

    if( (result = udp_sendto( sock, &data, sizeof(data), &addr)) )
    {
        if(result == ERR_NET_NO_ROUTE)
        {
            SHOW_ERROR( 0, "No route", result);
        }
        else
            SHOW_ERROR( 0, "Can't send, rc = %d", result);

        goto error;
    }
retry:
    NETADDR_TO_IPV4(raddr.addr) = 0;

    /* Receive packet with timeout of 5s */
    len = udp_recvfrom( sock, &data, sizeof(data), &raddr, SOCK_FLAG_TIMEOUT, 5000000L );
    if (len <= 0)
        goto error;             /* error or timeout occured */

    if (raddr.port != SNTP_PORT || (data.mode & 0xc0) == 0xc0)       /* if source port is not SNTP_PORT or server is not in sync return */
    {
        if ( NETADDR_TO_IPV4(raddr.addr) == 0xFFFFFFFF)
            goto retry;         /*  unusable packets will be just ignored. */
        else
            goto error;
    }

    *t = ntohl(data.transmit_ts_sec) - (70 * 365 + _LEAP_YEAR_ADJUST) * _DAY_SEC;
    *server_adr = NETADDR_TO_IPV4(raddr.addr);
    result = 0;

error:
    if(sock)
        udp_close(sock);

    return result;
}


int init_sntp(u_int32_t server_addr, u_int32_t interval)
{
    // NB! Just one instance!
    static struct SNTP_resync_args arg;

    in_addr_t out;
#if RESOLVE
    // Force resolver to do it from DNS, not from cache
    if( !name2ip( &out, "pool.ntp.org", RESOLVER_FLAG_NORCACHE ) )
        server_addr = out;
#endif
    arg.server_addr = server_addr;
    arg.interval = interval;

    hal_start_kernel_thread_arg( SNTP_resync, &arg );

    return 0;
}


#endif // HAVE_NET

