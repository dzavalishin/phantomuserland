/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ACPI startup
 *
 *
**/

#define DEBUG_MSG_PREFIX "acpi"
#include <debug_ext.h>
#define debug_level_flow 8
#define debug_level_error 10
#define debug_level_info 10

#include <acpi.h>
#include <errno.h>

//#include <acpi/acnamesp.h>

#include <acpi/acconfig.h>
#include <acpi/aclocal.h>
#include <acpi/acobject.h>
#include <acpi/acresrc.h>

#include <acpi/acpixf.h>

ACPI_MODULE_NAME    ("examples")

ACPI_STATUS InstallHandlers (void);



static ACPI_STATUS walkCallback ( ACPI_HANDLE Object, UINT32 NestingLevel, void *Context, void **ReturnValue)
{
    (void) Context;
    (void) ReturnValue;

    while(NestingLevel--)
        printf("  ");
    printf("%d: ");


    ACPI_BUFFER OutName;

    OutName.Length = ACPI_ALLOCATE_BUFFER;

    if( AE_OK == AcpiGetName( Object, ACPI_SINGLE_NAME, &OutName ) )
        printf("%s", OutName.Pointer );
    else
        printf("?Err" );

    ACPI_OBJECT_TYPE Type;
    if( AE_OK == AcpiGetType ( Object, &Type ) )
        printf(" type %d", Type );


    if( Type == ACPI_TYPE_DEVICE )
    {
        ACPI_DEVICE_INFO *Info;

        ACPI_STATUS Status = AcpiGetObjectInfo (Object, &Info);
        if(ACPI_SUCCESS (Status))
        {
            printf (" HID: %.8X, ADR: %.8X, Status: %x",
                    Info->HardwareId, Info->Address, Info->CurrentStatus);
            free(Info);
        }

    }

    printf("\n");

    /*
    if( Type == ACPI_TYPE_DEVICE )
    {
        ACPI_BUFFER rt_buf;
        rt_buf.Length = ACPI_ALLOCATE_BUFFER;
        ACPI_STATUS Status = AcpiGetIRQRoutingTable( Object, &rt_buf );
        if(ACPI_SUCCESS (Status))
        {
            hexdump( rt_buf.Pointer, rt_buf.Length, 0, 0 );

            AcpiRsDumpIrqList( rt_buf.Pointer );

            free(rt_buf.Pointer);
        }
    } */

    return AE_OK;
}


/******************************************************************************
 *
 * Example ACPICA initialization code. This shows a full initialization with
 * no early ACPI table access.
 *
 *****************************************************************************/

errno_t
InitializeFullAcpi(void)
{
    ACPI_STATUS             Status;


    /* Initialize the ACPICA subsystem */

    Status = AcpiInitializeSubsystem ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing ACPICA"));
        return ENXIO;
    }

    /* Initialize the ACPICA Table Manager and get all ACPI tables */

    Status = AcpiInitializeTables (NULL, 16, FALSE);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing Table Manager"));
        return ENXIO;
    }

    /* Create the ACPI namespace from ACPI tables */

    Status = AcpiLoadTables ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While loading ACPI tables"));
        return ENXIO;
    }

    /* Install local handlers */

    Status = InstallHandlers ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While installing handlers"));
        return ENXIO;
    }

#if 1

    /* Initialize the ACPI hardware */

    Status = AcpiEnableSubsystem (ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While enabling ACPICA"));
        return ENXIO;
    }

    /* Complete the ACPI namespace object initialization */

    Status = AcpiInitializeObjects (ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing ACPICA objects"));
        return ENXIO;
    }
#endif

#if 0
    AcpiWalkNamespace ( ACPI_TYPE_ANY, ACPI_ROOT_OBJECT, 16, walkCallback, 0, 0, 0 );
#endif


    return 0;
}


/******************************************************************************
 *
 * Example ACPICA handler and handler installation
 *
 *****************************************************************************/

void
NotifyHandler (
    ACPI_HANDLE                 Device,
    UINT32                      Value,
    void                        *Context)
{
    (void) Device;
    (void) Context;

    ACPI_INFO ((AE_INFO, "Received a notify 0x%X", Value));
}


ACPI_STATUS
InstallHandlers (void)
{
    ACPI_STATUS             Status;


    /* Install global notify handler */

    Status = AcpiInstallNotifyHandler (ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY,
                                        NotifyHandler, NULL);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While installing Notify handler"));
        return (Status);
    }

    return (AE_OK);
}



