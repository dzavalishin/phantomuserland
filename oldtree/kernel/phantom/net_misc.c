#if HAVE_NET

#define DEBUG_MSG_PREFIX "net"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include "net.h"
#include "udp.h"

#include <phantom_libc.h>
#include <endian.h>
#include <phantom_time.h>
#include <time.h>
#include <sys/syslog.h>

int phantom_tcpip_active = 0;





/*

THE TFTP PROTOCOL (REVISION 2)
http://spectral.mscs.mu.edu/RFC/rfc1350.html

  TFTP supports five types of packets, all of which have been mentioned
   above:

          opcode  operation
            1     Read request (RRQ)
            2     Write request (WRQ)
            3     Data (DATA)
            4     Acknowledgment (ACK)
            5     Error (ERROR)

   The TFTP header of a packet contains the  opcode  associated  with
   that packet.
*/


#define TFTP_MAX_PACKET 512

struct tftp_t {
	unsigned short opcode;
        union {
		struct {
			unsigned short block;
		} ack;
                struct {
			unsigned short block;
			char download[TFTP_MAX_PACKET];
		} data;

                /*
		char rrq[TFTP_DEFAULTSIZE_PACKET];
		struct {
			unsigned short errcode;
			char errmsg[TFTP_DEFAULTSIZE_PACKET];
		} err;
		struct {
			char data[TFTP_DEFAULTSIZE_PACKET+2];
		} oack;
*/
        } u;
};



static int do_tftp_test(void *prot_data)
{
    int rc;
    char buf[1024];


    sockaddr addr;
    addr.port = 1069; // local port

    addr.addr.len = 4;
    addr.addr.type = ADDR_TYPE_IP;
    NETADDR_TO_IPV4(addr.addr) = IPV4_DOTADDR_TO_ADDR(10, 0, 2, 2);

    if( 0 != (rc = udp_bind(prot_data, &addr)) )
        return rc;

    addr.port = 69; // TFTP


#define RRQ "__tftp/menu.lst\0octet\0"

    memcpy( buf, RRQ, sizeof(RRQ) );


#if 0
    printf("TFTP invalid RRQ...");
    if( 0 != (rc = udp_sendto(prot_data, buf, sizeof(RRQ), &addr)) )
        return rc;

    hal_sleep_msec(500);
#endif

    ((struct tftp_t*)buf)->opcode = htons(1); // RRQ
    SHOW_FLOW0( 1, "TFTP RRQ...");

    if( 0 != (rc = udp_sendto(prot_data, buf, sizeof(RRQ), &addr)) )
    {
        if(rc == ERR_NET_NO_ROUTE)
        {
            SHOW_ERROR( 0, "UDP tftp - No route\n", rc);
        }
        else
            SHOW_ERROR( 0, "UDP tftp - can't send, rc = %d\n", rc);

        return rc;
    }


    while(1)
    {

        SHOW_FLOW0( 1, "TFTP recv...");
        if( 0 >= (rc = udp_recvfrom(prot_data, buf, sizeof(buf), &addr, 0, 0)) )
            return rc;
        SHOW_FLOW0( 1, "TFTP GOT PKT...");

        if( rc < 4 )
        {
            SHOW_ERROR0( 0, "TFTP too short pkt received\n");
            return -1;
        }

        int opcode, nblock;
        opcode = ntohs( ((struct tftp_t*)buf)->opcode );
        nblock = ntohs( ((struct tftp_t*)buf)->u.ack.block );


        if( opcode != 3 )
        {
            if( opcode == 5 )
            {
                SHOW_ERROR( 0, "TFTP got error: %d (%s)...", nblock, buf+4);
            }
            else
                SHOW_ERROR( 0, "TFTP error: got opcode %d...", opcode);
            return -1;
        }

        SHOW_FLOW( 1, "TFTP blk %d...", nblock);

        if( rc < 4+512 )
        {
            printf("TFTP finish...");
            return 0;
        }


        ((struct tftp_t*)buf)->opcode = htons(4); // ACK
        //((struct tftp_t*)buf)->u.ack.block = htons(nblock); //orig block

       if( 0 != (rc = udp_sendto(prot_data, buf, 4, &addr)) )
           return rc;
    }

}


static void tftp_test(void)
{
    void *prot_data;

    printf("\ntftp start... ");
    if( udp_open(&prot_data) )
    {
        SHOW_ERROR0( 0, "UDP - can't prepare endpoint");
        return;
    }

    int rc;
    if( (rc = do_tftp_test(prot_data)) )
        SHOW_ERROR( 0 , "\ntftp failed, rc = %d\n", rc);

    udp_close(prot_data);
}


















#define SYSLOGD_PORT 514

static void *syslog_socket = 0;
static int syslog_failed = 0;
static char syslog_src_addr[64] = "(unknown)";
static sockaddr syslog_addr;

// Set in boot_cmd_line
extern char *syslog_dest_address_string;

static int connect_syslog(void)
{
    if( udp_open(&syslog_socket) )
    {
        SHOW_FLOW0( 1, "UDP syslog - can't open endpoint");
        return -1;
    }

    syslog_addr.port = SYSLOGD_PORT; // local port to be the same

    syslog_addr.addr.len = 4;
    syslog_addr.addr.type = ADDR_TYPE_IP;
    NETADDR_TO_IPV4(syslog_addr.addr) = IPV4_DOTADDR_TO_ADDR(192, 168, 1, 100);

    // It won't assign if address string is null or wrong
    parse_ipv4_addr( &(NETADDR_TO_IPV4(syslog_addr.addr)), syslog_dest_address_string );


    int rc;
    if( 0 != (rc = udp_bind(syslog_socket, &syslog_addr)) )
        return rc;

    syslog_addr.port = SYSLOGD_PORT; // Remote port

#if 1
    ipv4_addr src_addr;
    if(ipv4_lookup_srcaddr_for_dest( NETADDR_TO_IPV4(syslog_addr.addr), &src_addr))
    {
        SHOW_ERROR0( 0 , "UDP logger: unable to find my ip address");
        //return -1;
    }
    else
    {
        const char *ssap = (const char *)&src_addr;
        snprintf( syslog_src_addr, sizeof(syslog_src_addr)-1, "%d.%d.%d.%d",
                  ssap[3], ssap[2], ssap[1], ssap[0] );
    }
#else
    snprintf( syslog_src_addr, sizeof(syslog_src_addr)-1, "10.0.2.123");
#endif
    return 0;
}

#if 0
void udp_syslog_send(const char *message)
{
    if(syslog_failed)
        return;
    if(syslog_socket == 0)
        if(connect_syslog())
        {
            syslog_failed = 1;
            return;
        }


    struct tm tmb;
    localtime_rb(hal_local_time(), &tmb);

    char buf[1024];
    // kernel (0), debug(7)
    snprintf( buf, sizeof(buf)-1,
              "<%d%d>%s %02d %02d:%02d:%02d %s %s",
              0, 7,
              monNames[tmb.tm_mon], tmb.tm_mday,
              tmb.tm_hour, tmb.tm_min, tmb.tm_sec,
              syslog_src_addr,
              message);

    SHOW_FLOW( 1, "UDP syslog send '%s'\n", buf);

    int rc;
    if( 0 != (rc = udp_sendto(syslog_socket, buf, strlen(buf), &syslog_addr)) )
    {
        if(rc == ERR_NET_NO_ROUTE)
        {
            SHOW_ERROR( 0 , "UDP syslog - No route\n", rc);
        }
        else
            SHOW_ERROR( 0 , "UDP syslog - can't send, rc = %d\n", rc);
    }
}
#else
void udp_syslog_send(const char *prefix, const char *message)
{
    if(syslog_failed || !phantom_tcpip_active)
        return;
    if(syslog_socket == 0)
        if(connect_syslog())
        {
            syslog_failed = 1;
            return;
        }

    char buf[1024];
    // kernel (0), debug(7)
    snprintf( buf, sizeof(buf)-1,
              "%s %s %s",
              prefix,
              syslog_src_addr,
              message);

    SHOW_FLOW( 2, "UDP syslog send '%s'\n", buf);

    int rc;
    if( 0 != (rc = udp_sendto(syslog_socket, buf, strlen(buf), &syslog_addr)) )
    {
        if(rc == ERR_NET_NO_ROUTE)
        {
            SHOW_ERROR( 1, "UDP syslog - No route\n", rc);
        }
        else
            SHOW_ERROR( 1, "UDP syslog - can't send, rc = %d\n", rc);
    }
}

#endif













void net_test(void)
{
    syslog(LOG_DEBUG|LOG_KERN,"Test of UDP syslog");
    //tftp_test();
    //getchar();

}


#endif // HAVE_NET

