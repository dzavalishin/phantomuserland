/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Properties - generic funcs.
 *
**/

#define DEBUG_MSG_PREFIX "prop"
#include <debug_ext.h>
#define debug_level_flow 9
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/properties.h>
#include <kernel/libkern.h>
#include <errno.h>
#include <device.h>


errno_t gen_dev_listproperties( struct phantom_device *dev, int nProperty, char *pValue, int vlen )
{
    if(0 == dev->props) return ENOTTY;
    return gen_listproperties( dev->props, nProperty, pValue, vlen );
}

errno_t	gen_dev_getproperty( struct phantom_device *dev, const char *pName, char *pValue, int vlen )
{
    if(0 == dev->props) return ENOTTY;
    return gen_getproperty( dev->props, dev, pName, pValue, vlen );
}

errno_t	gen_dev_setproperty( struct phantom_device *dev, const char *pName, const char *pValue )
{
    if(0 == dev->props) return ENOTTY;
    return gen_setproperty( dev->props, dev, pName, pValue );
}


errno_t gen_listproperties( properties_t *ps, int nProperty, char *pValue, int vlen )
{
    SHOW_FLOW( 8, "gen_listproperties %d", nProperty );
    if( ((unsigned)nProperty) >= ps->lsize )
        return ENOENT;

    strlcpy( pValue, ps->list[nProperty].name, vlen );
    return 0;
}

static property_t *get_property( properties_t *ps, const char *pName )
{
    // TODO check/eat prefix

    // TODO binsearch
    unsigned int i;
    for( i = 0; i < ps->lsize; i++ )
        if( 0 == stricmp(pName, ps->list[i].name) )
            return ps->list+i;
    return 0;
}


errno_t	gen_getproperty( properties_t *ps, void *context, const char *pName, char *pValue, int vlen )
{
    property_t *p = get_property( ps, pName );
    if( 0 == p )
        return ENOENT;

    void *vp;

    if( p->valp )
        vp = p->valp;
    else
        vp = ps->valp(ps, context, p->offset );

    // If have get func - use
    if( p->getf )
    {
        errno_t rc = p->getf( ps, context, p->offset, vp, pValue, vlen );
        //if( rc )
        return rc;

        //goto act;
    }

    if( 0 == vp )
        return EFAULT;

    switch( p->type )
    {
    case pt_int32:
        {
            u_int32_t *ip = vp;
            snprintf( pValue, vlen, "%d", *ip );
            return 0;
        }

    case pt_mstring:
        snprintf( pValue, vlen, "%s", vp );
        return 0;

    default:
        return ENXIO;

    }

    return ENOTTY;
}

errno_t	gen_setproperty( properties_t *ps, void *context, const char *pName, const char *pValue )
{
    property_t *p = get_property( ps, pName );
    if( 0 == p )
        return ENOENT;

    void *vp;

    if( p->valp )
        vp = p->valp;
    else
        vp = ps->valp(ps, context, p->offset );

    // If have set func - use
    if( p->setf )
    {
        errno_t rc = p->setf( ps, context, p->offset, vp, pValue );
        if( rc )
            return rc;

        goto act;
    }

    // No set func? Really need vp
    if( 0 == vp )
        return EFAULT;

    switch( p->type )
    {
    case pt_int32:
        {
            u_int32_t *ip = vp;
            if( 1 != sscanf( pValue, "%d", ip ) )
                return EINVAL;
        }
        break;

    case pt_mstring:
        {
            size_t len = strlen( pValue )+1;
            if( len > 64*1024 )
                return EINVAL;

            char *olds = *(char **)vp;
            char *news = calloc( 1, len );

            if( olds ) free( olds );

            memcpy( news, pValue, len );

            *(char **)vp = news;
        }
        break;

    default:
        return ENXIO;

    }

act:
    // now tell 'em to activate new value
    p->activate(ps, context, p->offset, vp );

    return 0;
}





