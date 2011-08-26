/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * \brief
 * Properties list management.
 *
**/

#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <errno.h>
#include <phantom_types.h>


typedef enum
{
    pt_int32,
    pt_mstring,         // malloced string
    pt_enum32,         	// enum int32 - unimpl!
} property_type_t;


struct property;

typedef struct properties {
    //u_int32_t           prefix;         // 4-byte char prefix of this group, like 'dev.', 'gen.' or 'fsp.'
    const char *          prefix;         // 4-byte char prefix of this group, like 'dev.', 'gen.' or 'fsp.'

    struct property     *list;
    size_t              lsize;

    //! Get pointer to property value
    void *              (*valp)(struct properties *ps, void *context, size_t offset );



} properties_t;



typedef struct property {
    property_type_t     type;
    const char 		*name;
    size_t              offset;
    void                *valp;

    char                **val_list; // for enums

    void                (*activate)(struct properties *ps, void *context, size_t offset, void *vp );
    errno_t             (*setf)(struct properties *ps, void *context, size_t offset, void *vp, const char *val);
    // unused yet
    errno_t             (*getf)(struct properties *ps, void *context, size_t offset, void *vp, char *val, size_t len);
} property_t;



#define PROP_COUNT(__plist) (sizeof(__plist)/sizeof(__plist[0]))


struct phantom_device;

errno_t gen_dev_listproperties( struct phantom_device *dev, int nProperty, char *pValue, int vlen );
errno_t	gen_dev_getproperty( struct phantom_device *dev, const char *pName, char *pValue, int vlen );
errno_t	gen_dev_setproperty( struct phantom_device *dev, const char *pName, const char *pValue );

errno_t gen_listproperties( properties_t *ps, int nProperty, char *pValue, int vlen );
errno_t	gen_getproperty( properties_t *ps, void *context, const char *pName, char *pValue, int vlen );
errno_t	gen_setproperty( properties_t *ps, void *context, const char *pName, const char *pValue );





#endif // PROPERTIES_H
