#ifndef _RTC_REGS_H
#define _RTC_REGS_H

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif

enum {
    RTC_SEC   = 0,
    RTC_SECALRM	= 1,	/* seconds alarm */
    RTC_MIN   = 2,
    RTC_MINALRM	= 3,	/* minutes alarm */
    RTC_HOUR  = 4,
    RTC_HRSALRM	= 5,	/* hours alarm */
    RTC_WDAY	= 6,	/* week day */
    RTC_DAY   = 7,
    RTC_MONTH = 8,
    RTC_YEAR  = 9,

    RTC_BASELO	= 0x15,	/* low byte of basemem size */
    RTC_BASEHI	= 0x16,	/* high byte of basemem size */
    RTC_EXTLO	= 0x17,	/* low byte of extended mem size */
    RTC_EXTHI	= 0x18,	/* low byte of extended mem size */

    RTC_CENTURY = 0x32
};

// RTC A reg NMI mask bit
#define RTC_NMI_DIS		0x80
#define RTC_NMI_ENA		0x00

#define RTC_REG_A		0x70
#define RTC_REG_D		0x71


#define RTC_A_UIP		0x80



#endif // _RTC_REGS_H
