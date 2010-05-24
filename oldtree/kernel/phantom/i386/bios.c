/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * PC BIOS code
 *
**/

#define DEBUG_MSG_PREFIX "bios"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <string.h>
#include <pc/bios.h>
#include <phantom_libc.h>
#include <kernel/vm.h>
#include <errno.h>

#include "../driver_map.h"

#define BIOS_START	0xe0000
#define BIOS_SIZE	0x20000

#define BIOS_PADDRTOVADDR(x)	(phystokv(x))
#define BIOS_VADDRTOPADDR(x)	(kvtophys(x))


/**
 *
 * bios_sigsearch
 *
 * Search some or all of the BIOS region for a signature string.
 *
 * (start)	Optional offset returned from this function
 *		(for searching for multiple matches), or NULL
 *		to start the search from the base of the BIOS.
 *		Note that this will be a _physical_ address in
 *		the range 0xe0000 - 0xfffff.
 * (sig)	is a pointer to the byte(s) of the signature.
 * (siglen)	number of bytes in the signature.
 * (paralen)	signature paragraph (alignment) size.
 * (sigofs)	offset of the signature within the paragraph.
 *
 * Returns the _physical_ address of the found signature, 0 if the
 * signature was not found.
 *
**/

u_int32_t
bios_sigsearch(u_int32_t start, u_char *sig, int siglen, int paralen, int sigofs)
{
    u_char	*sp, *end;

    /* compute the starting address */
    if ((start >= BIOS_START) && (start <= (BIOS_START + BIOS_SIZE))) {
	sp = (u_char *)BIOS_PADDRTOVADDR(start);
    } else if (start == 0) {
	sp = (u_char *)BIOS_PADDRTOVADDR(BIOS_START);
    } else {
	return 0;				/* bogus start address */
    }

    /* compute the end address */
    end = (u_char *)BIOS_PADDRTOVADDR(BIOS_START + BIOS_SIZE);

    /* loop searching */
    while ((sp + sigofs + siglen) < end)
    {
        /* compare here */
        if (!bcmp(sp + sigofs, sig, siglen))
        {
	    /* convert back to physical address */
	    return((u_int32_t)BIOS_VADDRTOPADDR(sp));
	}
	sp += paralen;
    }
    return(0);
}









// -----------------------------------------------------------------------
// SMBios
// -----------------------------------------------------------------------

static int smbios_read( struct phantom_device *dev, void *buf, int len);


/*
 * System Management BIOS Reference Specification, v2.4 Final
 * http://www.dmtf.org/standards/published_documents/DSP0134.pdf
 */

/*
 * SMBIOS Entry Point Structure
 */
struct smbios_eps {
	u_int8_t	Anchor[4];		/* '_SM_' */
	u_int8_t	Checksum;
	u_int8_t	Length;

	u_int8_t	SMBIOS_Major;
	u_int8_t	SMBIOS_Minor;
	u_int16_t	Max_Size;
	u_int8_t	Revision;
	u_int8_t	Formatted_Area[5];

	u_int8_t	Intermediate_Anchor[5];	/* '_DMI_' */
	u_int8_t	Intermediate_Checksum;

	u_int16_t	Structure_Table_Length;
	u_int32_t	Structure_Table_Address;
	u_int16_t	Structure_Count;

	u_int8_t	SMBIOS_BCD_Revision;
} __packed;

/*struct smbios_softc {
	device_t		dev;
	struct resource *	res;
	int			rid;

	struct smbios_eps *	eps;
};*/

#define	SMBIOS_START	0xf0000
#define	SMBIOS_STEP	0x10
#define	SMBIOS_OFF	0
#define	SMBIOS_LEN	4
#define	SMBIOS_SIG	"_SM_"

//#define	RES2EPS(res)	((struct smbios_eps *)rman_get_virtual(res))
#define	ADDR2EPS(addr)  ((struct smbios_eps *)BIOS_PADDRTOVADDR(addr))



static int
smbios_cksum (struct smbios_eps *e)
{
    u_int8_t *ptr;
    u_int8_t cksum;
    int i;

    ptr = (u_int8_t *)e;
    cksum = 0;
    for (i = 0; i < e->Length; i++) {
        cksum += ptr[i];
    }

    return (cksum);
}


// Returns 0 or virtual adress of smbios
static struct smbios_eps *
smbios_identify()
{
    u_int32_t addr;
    int length;
    int rid;


    addr = bios_sigsearch(SMBIOS_START, (unsigned char *)SMBIOS_SIG, SMBIOS_LEN,
                          SMBIOS_STEP, SMBIOS_OFF);
    if (addr == 0)
        return 0;

    rid = 0;

    struct smbios_eps *ep = ADDR2EPS(addr);

    length = ep->Length;

    if (length != 0x1f)
    {
        u_int8_t major, minor;

        major = ep->SMBIOS_Major;
        minor = ep->SMBIOS_Minor;

        /* SMBIOS v2.1 implementation might use 0x1e. */
        if (length == 0x1e && major == 2 && minor == 1)
            length = 0x1f;
        else
            return 0;
    }

    /*
     child = BUS_ADD_CHILD(parent, 5, "smbios", -1);
     device_set_driver(child, driver);
     bus_set_resource(child, SYS_RES_MEMORY, rid, addr, length);
     device_set_desc(child, "System Management BIOS");
     */

    return ep;
}

static int seq_number = 0;
phantom_device_t * driver_etc_smbios_probe( const char *name, int stage )
{
    (void) stage;
    struct smbios_eps * ep = smbios_identify();

    if( seq_number || ep == 0 ) return 0;

    if(smbios_cksum(ep))
    {
        SHOW_ERROR0( 0, "SMBios checksum failed");
        //error = ENXIO;
        return 0;
    }

    SHOW_INFO( 0, "SMBios Version: %u.%u",
               ep->SMBIOS_Major, ep->SMBIOS_Minor);

    if(bcd2bin(ep->SMBIOS_BCD_Revision))
        SHOW_INFO( 0, "SMBios BCD Revision: %u.%u",
                   bcd2bin(ep->SMBIOS_BCD_Revision >> 4),
                   bcd2bin(ep->SMBIOS_BCD_Revision & 0x0f));


    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = name;
    dev->seq_number = seq_number++;
    dev->drv_private = ep;

    dev->dops.read = smbios_read;
    /*
    dev->dops.stop = beep_stop;

    dev->dops.write = beep_write;
    */

    return dev;
}


static int smbios_read( struct phantom_device *dev, void *buf, int len)
{
    //struct smbios_eps * ep = (struct smbios_eps *)dev->drv_private;
    (void) dev;
    (void) buf;
    (void) len;
    return -ENODEV;
}


















// Parts of this code are

/*-
 * Copyright (c) 1997 Michael Smith
 * Copyright (c) 1998 Jonathan Lemon
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */



