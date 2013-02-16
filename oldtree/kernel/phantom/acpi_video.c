/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ACPI to Phantom interface - Video
 *
 *
**/

#ifdef ARCH_ia32

#define DEBUG_MSG_PREFIX "acpi"
#include <debug_ext.h>
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 10

#include <assert.h>


/**
 * \file
 * \brief ACPI video hacks
 */

/*
 * Copyright (c) 2009, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#include <stdio.h>
//#include <barrelfish/barrelfish.h>
#include <acpi.h>

//#include "acpi_shared.h"
//#include "acpi_debug.h"

#include <acpi/accommon.h>
//#include "acpi/amlcode.h"
//#include "acpi/acparser.h"
#include <acpi/acdebug.h>

//#define ACPI_DEBUG(x...) printf(x)
#define ACPI_DEBUG(f,x...) SHOW_FLOW( 1, f, x )


ACPI_STATUS acpi_eval_integer(ACPI_HANDLE handle, char *name, ACPI_INTEGER *ret);


static ACPI_STATUS walk_video_device(ACPI_HANDLE handle, UINT32 level,
                                     void *context, void **dummy)
{
    (void) dummy;
    (void) level;
    (void) context;

    ACPI_OBJECT_LIST ArgList;
    ACPI_OBJECT Arg;
    ACPI_STATUS as;
    char namebuf[128];
    ACPI_BUFFER namebufobj = {.Length = sizeof(namebuf), .Pointer = namebuf};

    (void) level;
    (void) context;
    (void) dummy;

    /* get the node's name */
    as = AcpiGetName(handle, ACPI_FULL_PATHNAME, &namebufobj);
    if (ACPI_FAILURE(as)) {
        return as;
    }
    assert(namebufobj.Pointer == namebuf);

#if 1
    /* Execute the _DOS method (Enable/Disable Output Switching) B.4.1 */
    ArgList.Count = 1;
    ArgList.Pointer = &Arg;
    Arg.Type = ACPI_TYPE_INTEGER;
    Arg.Integer.Value = 0x1; /* automatically switch outputs */
    as = AcpiEvaluateObject(handle, "_DOS", &ArgList, NULL);
    if (ACPI_SUCCESS(as)) {
        ACPI_DEBUG("%s: successfully enabled video output switching\n", namebuf);
    }
#endif

    /* Execute the _DOD method if present (enumerate all devices, B.4.2 */
    char packagebuf[1024];
    ACPI_BUFFER retbuf = {.Length = sizeof(packagebuf), .Pointer = packagebuf};
    as = AcpiEvaluateObjectTyped(handle, "_DOD", NULL, &retbuf, ACPI_TYPE_PACKAGE);
    if (ACPI_SUCCESS(as)) {
        ACPI_DEBUG("called %s._DOD ok\n", namebuf);
    } else if (as != AE_NOT_FOUND) {
        ACPI_DEBUG("error executing _DOD method on %s: 0x%x\n", namebuf, as);
    }

    /* Execute the _DCS method if present (output device status) B.6.6 */
    ACPI_INTEGER retval;
    as = acpi_eval_integer(handle, "_DCS", &retval);
    if (ACPI_FAILURE(as)) {
        if (as != AE_NOT_FOUND) {
            ACPI_DEBUG("error executing _DCS method on %s: 0x%x\n", namebuf, as);
        }
        return AE_OK; // skip the rest
    }

    ACPI_DEBUG("%s: current video state is 0x%lx\n", namebuf, retval);

    /* if connector exists and is ready to switch, enable it! */
    if ((strstr(namebuf, "CRT0") || strstr(namebuf, "LCD0"))
        && (retval & 0xf) == 0xd) {
        ArgList.Count = 1;
        ArgList.Pointer = &Arg;
        Arg.Type = ACPI_TYPE_INTEGER;
        Arg.Integer.Value = 0x80000001;
        as = AcpiEvaluateObject(handle, "_DSS", &ArgList, NULL);
        if (ACPI_SUCCESS(as)) {
            ACPI_DEBUG("%s: successfully enabled video output\n", namebuf);
        } else {
            ACPI_DEBUG("%s: enabling video output failed: 0x%x\n", namebuf, as);
        }
    }

    return AE_OK;
}

// XXX: enable video output switching
void acpi_video_init(void)
{
    ACPI_STATUS as;

    /* find all devices with a _DOS method and call it */
    ACPI_DEBUG("Walking for video devices\n",0);
    //as = AcpiGetDevices(NULL, walk_video_device, NULL, NULL);
    //as = AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, ACPI_UINT32_MAX, walk_video_device, NULL, NULL);
    as = AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, ACPI_UINT32_MAX, walk_video_device, NULL, NULL, NULL);
    assert(ACPI_SUCCESS(as));
}


// TODO move to util src file

ACPI_STATUS acpi_eval_integer(ACPI_HANDLE handle, char *name, ACPI_INTEGER *ret)
{
    assert(ret != NULL);
    ACPI_STATUS as;
    char intbuf[sizeof(ACPI_OBJECT)];
    ACPI_BUFFER intbufobj = {.Length = sizeof(intbuf), .Pointer = intbuf};

    as = AcpiEvaluateObjectTyped(handle, name, NULL, &intbufobj, ACPI_TYPE_INTEGER);
    if (ACPI_SUCCESS(as)) {
        ACPI_OBJECT *obj = intbufobj.Pointer;
        *ret = obj->Integer.Value;
    }

    return as;
}


#endif // ARCH_ia32

