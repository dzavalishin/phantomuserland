/*
 * Copyright (C) 2001-2003 by egnite Software GmbH. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY EGNITE SOFTWARE GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL EGNITE
 * SOFTWARE GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 *
 * Thanks to Lars H. Andersson, who submitted the first idea of this simple function
 */






#include <sys/unistd.h>

#include <sys/socket.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <threads.h>
#include <time.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/resolv.h>

#include <arpa/inet.h>

#include "main.h"


errno_t name2ip( in_addr_t *out, const char *name, int flags );


static errno_t SNTPGetTime(u_int32_t * server_adr, time_t * t);


#ifndef NUT_THREAD_SNTPSTACK
#define NUT_THREAD_SNTPSTACK    256
#endif

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

/*
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
*/


void SNTP_resync(u_int32_t server_addr, u_int32_t interval)
{
    u_int32_t cur_server_addr = server_addr;
    int retry = 2; // if no answer on given addr, go to dns immediately
    time_t t;

    for (;;)
    {
        printf( "request time from %s\n", inet_itoa(htonl(server_addr)) );
        if (SNTPGetTime(&cur_server_addr, &t))
        {     /* if any error retry */
            if (cur_server_addr != server_addr && server_addr == 0xFFFFFFFF)
            {
                cur_server_addr = server_addr;
                continue;
            }

            if (retry++ >= 2)
            { /* if numer of retries >= 3 wait 30 secs before next retry sequence ... */
                retry = 0;
                sleepmsec(100);

                printf("will resolve\n");
		in_addr_t out; // ticktock.net.ru  ru.pool.org pool.ntp.org
                if( !name2ip( &out, "ntp3.ntp-servers.net", RESOLVER_FLAG_NORCACHE ) )
                {
                    server_addr = cur_server_addr = ntohl(out);
                    printf( "resolved to %s\n", inet_itoa(htonl(server_addr)) );
                }

            } else              /* ... else wait 5 secs for next retry */
                sleepmsec(500);
        } else {                /* no error */
            printf( "got time %ld\n", t );
            //printf( "got time %ld, %s", t, dumpt(t) );

            //set_time(t);          /* so set the time */
            retry = 0;
            sleepmsec(interval); /* and wait the interval time */
        }
    }
}

static errno_t SNTPGetTime(u_int32_t * server_adr, time_t * t)
{
    /*first check the pointers */
    //u_int32_t rec_addr;
    int sock;     /* the udp socket */
    sntpframe data;            /* we're using the heap to save stack space */
    //u_int16_t port;               /* source port from incoming packet */
    int len;
    errno_t result = EIO;

    /* Set UDP input buffer to 256 bytes */
    //u_int16_t bufsize = 256;

    struct sockaddr_in addr, raddr;

    addr.sin_port = htons(SNTP_PORT);
    addr.sin_len = sizeof(struct sockaddr_in);
    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = htonl(*server_adr);


    if (t == NULL)
        return -1;

    if (server_adr == NULL)
        return -1;

    sock = socket( PF_INET, SOCK_DGRAM, 0 );
    if( sock < 0 )
    {
        printf( "UDP - can't get socket\n");
        return EIO;
    }

    //setsockopt(sock, SO_RCVBUF, &bufsize, sizeof(bufsize));

    data.mode = 0x1B;          /* LI, VN and Mode bit fields (all in u_char mode); */


    if( sendto( sock, &data, sizeof(data), 0, (struct sockaddr *)&addr, sizeof(addr) ) )
        goto error;


//retry:
    raddr.sin_addr.s_addr = 0;

    /* Receive packet with timeout of 5s */
    //len = recvfrom( sock, &data, sizeof(data), &raddr, SOCK_FLAG_TIMEOUT, 5000000L );
    int alen = sizeof(raddr);

    len = recvfrom( sock, &data, sizeof(data), 0, (struct sockaddr *)&raddr, &alen );

    if (len <= 0)
        goto error;             /* error or timeout occured */

    /*
    if (raddr.sin_port != SNTP_PORT || (data.mode & 0xc0) == 0xc0)       // if source port is not SNTP_PORT or server is not in sync return
    {
        if ( raddr.sin_addr.s_addr == 0xFFFFFFFF )
            goto retry;         //  unusable packets will be just ignored.
        else
            goto error;
    }
    */

    *t = ntohl(data.transmit_ts_sec) - (70 * 365 + _LEAP_YEAR_ADJUST) * _DAY_SEC;
    *server_adr = raddr.sin_addr.s_addr;
    result = 0;

error:
    if( sock>=0 )
        close(sock);

    return result;
}



