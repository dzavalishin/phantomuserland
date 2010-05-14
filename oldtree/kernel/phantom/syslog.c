#include <sys/syslog.h>
#include <stdarg.h>
#include <phantom_libc.h>
#include <time.h>
#include "net.h"

/*
 * syslog, vsyslog --
 *	print message on log file; output is intended for syslogd(8).
 */

void
    vsyslog(int pri, const char *fmt, va_list ap)
{
    //int cnt;
    //char ch, *p;
    //char *stdp, tbuf[2048], fmt_cpy[1024], timbuf[26], errstr[64];

#define	INTERNALLOG	LOG_ERR|LOG_CONS|LOG_PERROR|LOG_PID
    /* Check for invalid bits. */
    if (pri & ~(LOG_PRIMASK|LOG_FACMASK)) {
        syslog(INTERNALLOG,
               "syslog: unknown facility/priority: %x", pri);
        pri &= LOG_PRIMASK|LOG_FACMASK;
    }


    /* Check priority against setlogmask values. * /
     if (!(LOG_MASK(LOG_PRI(pri)) & LogMask)) {
     THREAD_UNLOCK();
     return;
     } */

    /* Set default facility if none specified. */
    if ((pri & LOG_FACMASK) == 0)
        pri |= LOG_KERN;




    char prefix_buf[32];
    {
        struct tm tmb;
        localtime_rb(hal_local_time(), &tmb);

        snprintf( prefix_buf, sizeof(prefix_buf)-1,
                  "<%d>%s %02d %02d:%02d:%02d",
                  pri,
                  monNames[tmb.tm_mon], tmb.tm_mday,
                  tmb.tm_hour, tmb.tm_min, tmb.tm_sec
                );
    }

    char msg_buf[800];
    (void)vsnprintf(msg_buf, sizeof(msg_buf)-1, fmt, ap);


#if HAVE_NET
    udp_syslog_send(prefix_buf, msg_buf);
#endif // HAVE_NET

    printf("%s %s\n", prefix_buf, msg_buf);
}



void
syslog(int pri, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsyslog(pri, fmt, ap);
    va_end(ap);
}
