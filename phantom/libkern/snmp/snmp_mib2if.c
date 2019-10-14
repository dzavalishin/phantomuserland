#if HAVE_NET
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * SNMP MIB: Interfaces.
 *
**/


#define DEBUG_MSG_PREFIX "snmp"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/net.h>
#include <time.h>

#include <compat/nutos.h>

#include <kernel/snmp/snmp.h>
#include <kernel/snmp/snmp_api.h>
#include <kernel/snmp/snmp_mib.h>


#define MAX_MIB_IF_ENTRIES   4


static uint8_t *MibVarsIfGet(CONST SNMPVAR *, OID *, size_t *, int, size_t *, WMETHOD **);

#define MAG_IF_NUMBER           0

#define MAG_IF_INDEX            1
#define MAG_IF_DESCR            2
#define MAG_IF_TYPE             3
#define MAG_IF_MTU              4
#define MAG_IF_SPEED            5
#define MAG_IF_PHYSADDRESS      6
#define MAG_IF_ADMINSTATUS      7
#define MAG_IF_OPERSTATUS       8
#define MAG_IF_LASTCHANGE       9
#define MAG_IF_INOCTETS         10
#define MAG_IF_INUCASTPKTS      11
#define MAG_IF_INNUCASTPKTS     12
#define MAG_IF_INDISCARDS       13
#define MAG_IF_INERRORS         14
#define MAG_IF_INUNKNOWNPROTOS  15
#define MAG_IF_OUTOCTETS        16
#define MAG_IF_OUTUCASTPKTS     17
#define MAG_IF_OUTNUCASTPKTS    18
#define MAG_IF_OUTDISCARDS      19
#define MAG_IF_OUTERRORS        20
#define MAG_IF_OUTQLEN          21
#define MAG_IF_SPECIFIC         22

static OID base_oid[] = { SNMP_OID_MIB2, 2 };
static size_t base_oidlen = sizeof(base_oid) / sizeof(OID);

static SNMPVAR mib_variables[] = {
    {MAG_IF_NUMBER, ASN_INTEGER, ACL_RONLY, MibVarsIfGet, 1, {1}},
    {MAG_IF_INDEX, ASN_INTEGER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 1}},
    {MAG_IF_DESCR, ASN_OCTET_STR, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 2}},
    {MAG_IF_TYPE, ASN_INTEGER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 3}},
    {MAG_IF_MTU, ASN_INTEGER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 4}},
    {MAG_IF_SPEED, ASN_GAUGE, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 5}},
    {MAG_IF_PHYSADDRESS, ASN_OCTET_STR, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 6}},
    {MAG_IF_ADMINSTATUS, ASN_INTEGER, ACL_RWRITE, MibVarsIfGet, 3, {2, 1, 7}},
    {MAG_IF_OPERSTATUS, ASN_INTEGER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 8}},
    {MAG_IF_LASTCHANGE, ASN_TIMETICKS, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 9}},
    {MAG_IF_INOCTETS, ASN_COUNTER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 10}},
    {MAG_IF_INUCASTPKTS, ASN_COUNTER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 11}},
    {MAG_IF_INNUCASTPKTS, ASN_COUNTER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 12}},
    {MAG_IF_INDISCARDS, ASN_COUNTER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 13}},
    {MAG_IF_INERRORS, ASN_COUNTER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 14}},
    {MAG_IF_INUNKNOWNPROTOS, ASN_COUNTER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 15}},
    {MAG_IF_OUTOCTETS, ASN_COUNTER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 16}},
    {MAG_IF_OUTUCASTPKTS, ASN_COUNTER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 17}},
    {MAG_IF_OUTNUCASTPKTS, ASN_COUNTER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 18}},
    {MAG_IF_OUTDISCARDS, ASN_COUNTER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 19}},
    {MAG_IF_OUTERRORS, ASN_COUNTER, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 20}},
    {MAG_IF_OUTQLEN, ASN_GAUGE, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 21}},
    {MAG_IF_SPECIFIC, ASN_OBJECT_ID, ACL_RONLY, MibVarsIfGet, 3, {2, 1, 22}}
};

static OID if_specific[] = { 0, 0 };


/*!
 * \brief Register MIB II variables of the interfaces.
 *
 * \note Preliminary code with hard coded values.
 */
int MibRegisterIfVars(void)
{
    return SnmpMibRegister(base_oid, base_oidlen, mib_variables, sizeof(mib_variables) / sizeof(SNMPVAR));
}

/*
static int MibVarsIfSet(int action, uint8_t * var_val, uint8_t var_val_type, size_t var_val_len, OID * name, size_t name_len)
{
    size_t bigsize = 1000;
    uint32_t value = 0;

    if (action != SNMP_ACT_COMMIT) {
        return 0;
    }
    switch (name[9]) {
    case 7:
        if (var_val_type != ASN_INTEGER)
            return SNMP_ERR_WRONGTYPE;
        if (var_val_len > sizeof(uint32_t))
            return SNMP_ERR_WRONGLENGTH;
        AsnUnsignedParse(var_val, &bigsize, &var_val_type, &value);
        if (name[10] > 0 && name[10] < MAX_MIB_IF_ENTRIES) {
            if_admin_status[name[10] - 1] = value;
        }
        break;
    }
    return 0;
}
*/

static uint8_t *MibVarsIfGet(CONST SNMPVAR * vp, OID * name, size_t * namelen, int exact, size_t * varlen, WMETHOD ** wmethod)
{
    int rc;
    int ifc = 0; // number of interface we got request for
    OID index = 0;
    static long zero = 0;
    OID *fullname;
    size_t fullnamelen = base_oidlen + vp->var_namelen + 1;

    fullname = malloc(fullnamelen * sizeof(OID));
    memcpy(fullname, base_oid, base_oidlen * sizeof(OID));
    memcpy(fullname + base_oidlen, vp->var_name, vp->var_namelen * sizeof(OID));

    if (vp->var_magic == MAG_IF_NUMBER) {
        /* Determine the number of interfaces. */
        *(fullname + fullnamelen - 1) = index;
        rc = SnmpOidCmp(name, *namelen, fullname, fullnamelen);
        if ((exact && rc) || (!exact && rc >= 0)) {
            free(fullname);
            return NULL;
        }
    } else {
        int if_number = if_get_count();
        /* Determine the interface number. */
        for (; ifc < if_number; ifc++) {
            *(fullname + fullnamelen - 1) = ifc + 1;
            rc = SnmpOidCmp(name, *namelen, fullname, fullnamelen);
            if ((exact && rc == 0) || (!exact && rc < 0)) {
                index = ifc + 1;
                break;
            }
        }
        if (index == 0) {
            free(fullname);
            return NULL;
        }
    }

    memcpy(name, fullname, fullnamelen * sizeof(OID));
    free(fullname);
    *namelen = fullnamelen;
    *wmethod = NULL;
    *varlen = sizeof(long);

    if( vp->var_magic == MAG_IF_NUMBER)
    {
        static uint8_t if_count;
        if_count = if_get_count();
        return & if_count;
    }

    ifnet *iface = if_id_to_ifnet( ifc );
    if( iface == 0 )
        return 0;


    switch (vp->var_magic) {

    /*
    case MAG_IF_NUMBER:
        {
            uint8_t if_count = if_get_count();
            return & if_count;
        }
    */

    case MAG_IF_INDEX:
        {
            static long if_index;
            if_index = iface->id;
            return (uint8_t *) &if_index;
        }

    case MAG_IF_DESCR:
        {
            const char *typename = "unknown";

            switch(iface->type)
            {
            case IF_TYPE_LOOPBACK: typename = "loopback"; break;
            case IF_TYPE_ETHERNET: typename = "ethernet"; break;
            }

            const char *devname = "none";
            if( iface->dev )
            {
                devname = iface->dev->name;
            }

            static char if_descr[128];
            snprintf( if_descr, sizeof(if_descr)-1,
                      "%s on %s",
                      typename, devname
                    );
            *varlen = strlen(if_descr);
            return (uint8_t *) if_descr;
        }
    case MAG_IF_TYPE:
        {
            static long if_type = 6; // ?
            return (uint8_t *) &if_type;
        }
    case MAG_IF_MTU:
        {
            static long if_mtu;
            if_mtu = iface->mtu;
            return (uint8_t *) & if_mtu;
        }

    case MAG_IF_SPEED:
        {
            static long if_speed = 100000000UL;
            return (uint8_t *) & if_speed;
        }

    case MAG_IF_PHYSADDRESS:
        {
            static unsigned char mac[6];

            if( (iface->dev == 0) || (iface->dev->dops.get_address == 0) )
            {
                bzero( mac, sizeof(mac) );
            }
            else
            {
                errno_t err = iface->dev->dops.get_address(iface->dev, mac, sizeof(mac));

                if(err)
                    bzero( mac, sizeof(mac) );
            }

            *varlen = sizeof(mac);
            return mac;
        }

    case MAG_IF_ADMINSTATUS: // up 1 down 2
    case MAG_IF_OPERSTATUS:
        {
            //*wmethod = MibVarsIfSet;
            static long st = 1;
            return (uint8_t *) &st;
        }
    case MAG_IF_LASTCHANGE:
    case MAG_IF_INOCTETS:
    case MAG_IF_INUCASTPKTS:
    case MAG_IF_INNUCASTPKTS:
    case MAG_IF_INDISCARDS:
    case MAG_IF_INERRORS:
    case MAG_IF_INUNKNOWNPROTOS:
    case MAG_IF_OUTOCTETS:
    case MAG_IF_OUTUCASTPKTS:
    case MAG_IF_OUTNUCASTPKTS:
    case MAG_IF_OUTDISCARDS:
    case MAG_IF_OUTERRORS:
    case MAG_IF_OUTQLEN:
        return (uint8_t *) & zero;
    case MAG_IF_SPECIFIC:
        *varlen = sizeof(if_specific);
        return (uint8_t *) if_specific;
    }
    return NULL;
}
#endif // HAVE_NET
