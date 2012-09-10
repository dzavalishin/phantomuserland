/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * SNMP MIB: OS.
 *
**/


#define DEBUG_MSG_PREFIX "snmp"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <sys/utsname.h>
#include <kernel/boot.h>
#include <time.h>

#include <compat/nutos.h>

#include <kernel/snmp/snmp.h>
#include <kernel/snmp/snmp_api.h>
#include <kernel/snmp/snmp_mib.h>


#define MAG_SYS_DESCR	    1
#define MAG_SYS_OID	    2
#define MAG_SYS_UPTIME      3
#define MAG_SYS_CONTACT	    4
#define MAG_SYS_NAME	    5
#define MAG_SYS_LOCATION    6
#define MAG_SYS_SERVICES    7


static uint8_t *MibVarsSysGet(CONST SNMPVAR *, OID *, size_t *, int, size_t *, WMETHOD **);


static OID base_oid[] = { SNMP_OID_MIB2, 1 };
static size_t base_oidlen = sizeof(base_oid) / sizeof(OID);

static SNMPVAR mib_variables[] = {

    {MAG_SYS_DESCR,  ASN_OCTET_STR, ACL_RONLY, MibVarsSysGet, 1, {1}},
    {MAG_SYS_OID,    ASN_OBJECT_ID, ACL_RONLY, MibVarsSysGet, 1, {2}},
    {MAG_SYS_UPTIME, ASN_TIMETICKS, ACL_RONLY, MibVarsSysGet, 1, {3}},
//    {MAG_SYS_CONTACT,ASN_OCTET_STR, ACL_RWRITE, MibVarsSysGet, 1, {4}},
    {MAG_SYS_CONTACT,ASN_OCTET_STR, ACL_RONLY, MibVarsSysGet, 1, {4}},
    {MAG_SYS_NAME,   ASN_OCTET_STR, ACL_RONLY, MibVarsSysGet, 1, {5}},
//    {MAG_SYS_NAME,   ASN_OCTET_STR, ACL_RWRITE, MibVarsSysGet, 1, {5}},
//    {MAG_SYS_LOCATION, ASN_OCTET_STR, ACL_RWRITE, MibVarsSysGet, 1, {6}},
    {MAG_SYS_LOCATION, ASN_OCTET_STR, ACL_RONLY, MibVarsSysGet, 1, {6}},
    {MAG_SYS_SERVICES, ASN_INTEGER,   ACL_RONLY, MibVarsSysGet, 1, {7}}

};

static time_t sys_starttime;




/*!
 * \brief Register MIB II variables of the kernel in general.
 */
int MibRegisterOSVars(void)
{
    sys_starttime = time(NULL);

    return SnmpMibRegister(base_oid, base_oidlen, mib_variables, sizeof(mib_variables) / sizeof(SNMPVAR));
}


/*!
 * \brief Access the specified MIB variable.
 *
 * \param vp
 * \param name    Contains the name to look for, either exact or one that
 *                is in front. On return the exact name is stored here.
 * \param namelen Number of sub identifiers in the name upon entry. On
 *                return the length of the exact name is stored here.
 * \param exact   If not zero, the name must match exactly. Otherwise we
 *                want the first name that is following the given one.
 * \param varlen  Size of the variable.
 */
static uint8_t *MibVarsSysGet(CONST SNMPVAR * vp, OID * name, size_t * namelen, int exact, size_t * varlen, WMETHOD ** wmethod)
{
    static uint8_t empty[1];
    int rc;
    OID *fullname;
    size_t fullnamelen = base_oidlen + vp->var_namelen + 1;

    fullname = malloc(fullnamelen * sizeof(OID));
    memcpy(fullname, base_oid, base_oidlen * sizeof(OID));
    memcpy(fullname + base_oidlen, vp->var_name, vp->var_namelen * sizeof(OID));
    *(fullname + fullnamelen - 1) = 0;

    rc = SnmpOidCmp(name, *namelen, fullname, fullnamelen);
    if ((exact && rc) || (!exact && rc >= 0))
    {
        free(fullname);
        return NULL;
    }

    memcpy(name, fullname, fullnamelen * sizeof(OID));
    free(fullname);
    *namelen = fullnamelen;

    *wmethod = NULL;
    *varlen = sizeof(long);

    switch (vp->var_magic) {

    case MAG_SYS_DESCR:
        {
#define MAXSD (_UTSNAME_LENGTH*5)
            static char sys_descr[MAXSD];

            snprintf( sys_descr, MAXSD-1, "%s ver. %s svn %s for %s",
                      phantom_uname.sysname,
                      phantom_uname.version,
                      phantom_uname.release,
                      phantom_uname.machine
                    );

            *varlen = strlen(sys_descr);
            return (uint8_t *) sys_descr;
        }

        //case MAG_SYS_OID:
        //*varlen = sizeof(sys_oid);
        //return (uint8_t *) sys_oid;

    case MAG_SYS_UPTIME:
        {
            static long sys_uptime;
            sys_uptime = time(NULL) - sys_starttime;
            sys_uptime *= 100;
            return (uint8_t *) & sys_uptime;
        }

    case MAG_SYS_NAME:
        //*wmethod = MibVarsSysSet;
        if (phantom_uname.nodename) {
            *varlen = strlen(phantom_uname.nodename);
            return (uint8_t *) phantom_uname.nodename;
        }
        *varlen = 0;
        return empty;

        // The physical location of this node (e.g., `telephone closet, 3rd floor'). If the location is unknown, the value is the zero-length string.
    case MAG_SYS_LOCATION:
        *varlen = 0;
        return empty;

        // TODO store somehow? Object?
    case MAG_SYS_CONTACT:
        {
            const char *cc = "dz@dz.ru :)";
            *varlen = strlen(cc);
            return (uint8_t *) cc;
        }

    }
    return NULL;
}

