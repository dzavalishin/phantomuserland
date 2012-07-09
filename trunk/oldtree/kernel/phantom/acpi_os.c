/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ACPI to Phantom interface
 *
 *
**/

#ifdef ARCH_ia32

#define DEBUG_MSG_PREFIX "acpi"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include "acpi.h"
#include "acpi/accommon.h"
#include "acpi/amlcode.h"
#include "acpi/acparser.h"
#include "acpi/acdebug.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <threads.h>
#include <errno.h>

#include <kernel/sem.h>
#include <kernel/page.h>

#include <ia32/pio.h>
#include <kernel/bus/pci.h>

#include <hal.h>

#define _COMPONENT          ACPI_OS_SERVICES
        ACPI_MODULE_NAME    ("osphantom")


//extern FILE                    *AcpiGbl_DebugFile;
//FILE                           *AcpiGbl_OutputFile;

ACPI_PHYSICAL_ADDRESS
AeLocalGetRootPointer(void)
{
    return (0);
}


/* Upcalls to AcpiExec */

ACPI_PHYSICAL_ADDRESS
AeLocalGetRootPointer (
    void);

void
AeTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_TABLE_HEADER       **NewTable);

typedef void* (*PTHREAD_CALLBACK) (void *);

/* Apple-specific */

#ifdef __APPLE__
#define sem_destroy         sem_close
#endif


/******************************************************************************
 *
 * FUNCTION:    AcpiOsInitialize, AcpiOsTerminate
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Init and terminate. Nothing to do.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsInitialize (
    void)
{

    //AcpiGbl_OutputFile = stdout;
    return (AE_OK);
}


ACPI_STATUS
AcpiOsTerminate (
    void)
{

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetRootPointer
 *
 * PARAMETERS:  None
 *
 * RETURN:      RSDP physical address
 *
 * DESCRIPTION: Gets the ACPI root pointer (RSDP)
 *
 *****************************************************************************/

ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
    void)
{
    ACPI_SIZE TableAddress;

    ACPI_STATUS Status = AcpiFindRootPointer( &TableAddress );

    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While getting root ptr"));
        return 0;
    }

    return TableAddress;

    //return (AeLocalGetRootPointer ());
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsPredefinedOverride
 *
 * PARAMETERS:  InitVal             - Initial value of the predefined object
 *              NewVal              - The new value for the object
 *
 * RETURN:      Status, pointer to value. Null pointer returned if not
 *              overriding.
 *
 * DESCRIPTION: Allow the OS to override predefined names
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsPredefinedOverride (
    const ACPI_PREDEFINED_NAMES *InitVal,
    ACPI_STRING                 *NewVal)
{

    if (!InitVal || !NewVal)
    {
        return (AE_BAD_PARAMETER);
    }

    *NewVal = NULL;
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsTableOverride
 *
 * PARAMETERS:  ExistingTable       - Header of current table (probably
 *                                    firmware)
 *              NewTable            - Where an entire new table is returned.
 *
 * RETURN:      Status, pointer to new table. Null pointer returned if no
 *              table is available to override
 *
 * DESCRIPTION: Return a different version of a table if one is available
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_TABLE_HEADER       **NewTable)
{

    if (!ExistingTable || !NewTable)
    {
        return (AE_BAD_PARAMETER);
    }

    *NewTable = NULL;

#ifdef ACPI_EXEC_APP

    AeTableOverride (ExistingTable, NewTable);
    return (AE_OK);
#else

    return (AE_NO_ACPI_TABLES);
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsRedirectOutput
 *
 * PARAMETERS:  Destination         - An open file handle/pointer
 *
 * RETURN:      None
 *
 * DESCRIPTION: Causes redirect of AcpiOsPrintf and AcpiOsVprintf
 *
 *****************************************************************************/

void
AcpiOsRedirectOutput (
    void                    *Destination)
{
    (void) Destination;
    printf("acpi??");
    //AcpiGbl_OutputFile = Destination;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsPrintf
 *
 * PARAMETERS:  fmt, ...            - Standard printf format
 *
 * RETURN:      None
 *
 * DESCRIPTION: Formatted output
 *
 *****************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *Fmt,
    ...)
{
    va_list                 Args;


    va_start (Args, Fmt);
    AcpiOsVprintf (Fmt, Args);
    va_end (Args);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsVprintf
 *
 * PARAMETERS:  fmt                 - Standard printf format
 *              args                - Argument list
 *
 * RETURN:      None
 *
 * DESCRIPTION: Formatted output with argument list pointer
 *
 *****************************************************************************/

void
AcpiOsVprintf (
    const char              *Fmt,
    va_list                 Args)
{
    INT32                   Count = 0;
    UINT8                   Flags;


    Flags = AcpiGbl_DbOutputFlags;
    if (Flags & ACPI_DB_REDIRECTABLE_OUTPUT)
    {
        /* Output is directable to either a file (if open) or the console */
#if 0
        if (AcpiGbl_DebugFile)
        {
            /* Output file is open, send the output there */

            Count = vprintf (Fmt, Args);
        }
        else
#endif
        {
            /* No redirection, send output to console (once only!) */

            Flags |= ACPI_DB_CONSOLE_OUTPUT;
        }
    }

    if (Flags & ACPI_DB_CONSOLE_OUTPUT)
    {
        Count = vprintf(Fmt, Args);
    }
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetLine
 *
 * PARAMETERS:  Buffer              - Where to return the command line
 *              BufferLength        - Maximum length of Buffer
 *              BytesRead           - Where the actual byte count is returned
 *
 * RETURN:      Status and actual bytes read
 *
 * DESCRIPTION: Formatted input with argument list pointer
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsGetLine (
    char                    *Buffer,
    UINT32                  BufferLength,
    UINT32                  *BytesRead)
{
    UINT8                   Temp;
    UINT32                  i;


    for (i = 0; ; i++)
    {
        if (i >= BufferLength)
        {
            return (AE_BUFFER_OVERFLOW);
        }

        Temp = getchar();
        if (!Temp || Temp == '\n')
        {
            break;
        }

        Buffer [i] = Temp;
    }

    /* Null terminate the buffer */

    Buffer [i] = 0;

    /* Return the number of bytes in the string */

    if (BytesRead)
    {
        *BytesRead = i;
    }
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsMapMemory
 *
 * PARAMETERS:  where               - Physical address of memory to be mapped
 *              length              - How much memory to map
 *
 * RETURN:      Pointer to mapped memory. Null on error.
 *
 * DESCRIPTION: Map physical memory into caller's address space
 *
 *****************************************************************************/

void *AcpiOsMapMemory(
    ACPI_PHYSICAL_ADDRESS   where,
    ACPI_SIZE               length)
{
    SHOW_FLOW( 9, "%p, %d bytes" , where, length );

    int offset = where - PREV_PAGE_ALIGN(where);

    size_t nump = BYTES_TO_PAGES(length+offset);
    void *result;
    errno_t rc = hal_alloc_vaddress(&result, nump); // alloc address of a page, but not memory
    if(rc) return (void *)0;

    hal_pages_control_etc( where-offset, result, nump, page_map_io, page_rw, 0 );

    return result+offset;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsUnmapMemory
 *
 * PARAMETERS:  where               - Logical address of memory to be unmapped
 *              length              - How much memory to unmap
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete a previously created mapping. Where and Length must
 *              correspond to a previous mapping exactly.
 *
 *****************************************************************************/

void
AcpiOsUnmapMemory (
    void                    *where,
    ACPI_SIZE               length)
{
    int offset = (addr_t)where - PREV_PAGE_ALIGN(((addr_t)where));

    size_t nump = BYTES_TO_PAGES(length+offset);
    hal_pages_control_etc( 0, where-offset, nump, page_unmap, page_noaccess, 0 );
    hal_free_vaddress( where-offset, nump );
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsAllocate
 *
 * PARAMETERS:  Size                - Amount to allocate, in bytes
 *
 * RETURN:      Pointer to the new allocation. Null on error.
 *
 * DESCRIPTION: Allocate memory. Algorithm is dependent on the OS.
 *
 *****************************************************************************/

void *
AcpiOsAllocate (
    ACPI_SIZE               size)
{
    return malloc ((size_t) size);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsFree
 *
 * PARAMETERS:  mem                 - Pointer to previously allocated memory
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Free memory allocated via AcpiOsAllocate
 *
 *****************************************************************************/

void
AcpiOsFree (
    void                    *mem)
{
    free (mem);
}


#ifdef ACPI_SINGLE_THREADED
/******************************************************************************
 *
 * FUNCTION:    Semaphore stub functions
 *
 * DESCRIPTION: Stub functions used for single-thread applications that do
 *              not require semaphore synchronization. Full implementations
 *              of these functions appear after the stubs.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsCreateSemaphore (
    UINT32              MaxUnits,
    UINT32              InitialUnits,
    ACPI_HANDLE         *OutHandle)
{
    *OutHandle = (ACPI_HANDLE) 1;
    return (AE_OK);
}

ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_HANDLE         Handle)
{
    return (AE_OK);
}

ACPI_STATUS
AcpiOsWaitSemaphore (
    ACPI_HANDLE         Handle,
    UINT32              Units,
    UINT16              Timeout)
{
    return (AE_OK);
}

ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_HANDLE         Handle,
    UINT32              Units)
{
    return (AE_OK);
}

#else
/******************************************************************************
 *
 * FUNCTION:    AcpiOsCreateSemaphore
 *
 * PARAMETERS:  InitialUnits        - Units to be assigned to the new semaphore
 *              OutHandle           - Where a handle will be returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create an OS semaphore
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsCreateSemaphore (
    UINT32              MaxUnits,
    UINT32              InitialUnits,
    ACPI_HANDLE         *OutHandle)
{
    (void) MaxUnits;

    hal_sem_t               *Sem;

    if (!OutHandle)
    {
        return (AE_BAD_PARAMETER);
    }

    Sem = AcpiOsAllocate (sizeof (hal_sem_t));
    if (!Sem)
    {
        return (AE_NO_MEMORY);
    }

    if(hal_sem_init(Sem, "ACPI"))
    {
        AcpiOsFree(Sem);
        return (AE_BAD_PARAMETER);
    }


    while(InitialUnits--)
        hal_sem_release( Sem);

    *OutHandle = (ACPI_HANDLE) Sem;
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsDeleteSemaphore
 *
 * PARAMETERS:  Handle              - Handle returned by AcpiOsCreateSemaphore
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete an OS semaphore
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_HANDLE         Handle)
{
    hal_sem_t               *Sem = (hal_sem_t *) Handle;


    if (!Sem)
    {
        return (AE_BAD_PARAMETER);
    }

    hal_sem_destroy(Sem);
    free(Sem);
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWaitSemaphore
 *
 * PARAMETERS:  Handle              - Handle returned by AcpiOsCreateSemaphore
 *              Units               - How many units to wait for
 *              Timeout             - How long to wait
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Wait for units
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWaitSemaphore (
    ACPI_HANDLE         Handle,
    UINT32              Units,
    UINT16              Timeout)
{
    ACPI_STATUS         Status = AE_OK;
    hal_sem_t               *Sem = (hal_sem_t *) Handle;
    //struct timespec     T;


    if (!Sem)
    {
        return (AE_BAD_PARAMETER);
    }

    switch (Timeout)
    {
    /*
     * No Wait:
     * --------
     * A zero timeout value indicates that we shouldn't wait - just
     * acquire the semaphore if available otherwise return AE_TIME
     * (a.k.a. 'would block').
     */
    case 0:

        if(hal_sem_acquire_etc( Sem, Units, SEM_FLAG_TIMEOUT, 1 ))
        {
            Status = (AE_TIME);
        }
        break;

    /* Wait Indefinitely */

    case ACPI_WAIT_FOREVER:

        if(hal_sem_acquire(Sem))
        {
            Status = (AE_TIME);
        }
        break;

    /* Wait with Timeout */

    default:

        //T.tm_sec = Timeout / 1000;
        //T.tm_nsec = (Timeout - (T.tv_sec * 1000)) * 1000000;

#if 0 //def ACPI_USE_ALTERNATE_TIMEOUT
        /*
         * Alternate timeout mechanism for environments where
         * sem_timedwait is not available or does not work properly.
         */
        while (Timeout)
        {
            if (sem_trywait (Sem) == 0)
            {
                /* Got the semaphore */
                return (AE_OK);
            }
            hal_sleep_msec(1);  /* one millisecond */
            Timeout--;
        }
        Status = (AE_TIME);
#else

        if(hal_sem_acquire_etc( Sem, Units, SEM_FLAG_TIMEOUT, Timeout*1000 ))
        {
            Status = (AE_TIME);
        }
#endif

        break;
    }

    return (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsSignalSemaphore
 *
 * PARAMETERS:  Handle              - Handle returned by AcpiOsCreateSemaphore
 *              Units               - Number of units to send
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Send units
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_HANDLE         Handle,
    UINT32              Units)
{
    hal_sem_t               *Sem = (hal_sem_t *)Handle;


    if (!Sem)
    {
        return (AE_BAD_PARAMETER);
    }

    if(Units != 1)
        printf("sem sig uints %d\n", Units);

    hal_sem_release(Sem);

    return (AE_OK);
}

#endif /* ACPI_SINGLE_THREADED */


/******************************************************************************
 *
 * FUNCTION:    Spinlock interfaces
 *
 * DESCRIPTION: Map these interfaces to semaphore interfaces
 *
 *****************************************************************************/

#include <spinlock.h>

ACPI_STATUS
AcpiOsCreateLock (
    ACPI_SPINLOCK           *OutHandle)
{

    //return (AcpiOsCreateSemaphore (1, 1, OutHandle));
    hal_spinlock_t *sl = calloc( sizeof(hal_spinlock_t), 1);
    assert(sl);
    hal_spin_init( sl );
    *OutHandle = sl;
    return (AE_OK);
}


void
AcpiOsDeleteLock (
    ACPI_SPINLOCK           Handle)
{
    //AcpiOsDeleteSemaphore (Handle);

    hal_spinlock_t *sl = (void *)Handle;
    free(sl);
}


ACPI_CPU_FLAGS
AcpiOsAcquireLock (
    ACPI_HANDLE             Handle)
{
    //AcpiOsWaitSemaphore (Handle, 1, 0xFFFF);
    hal_spinlock_t *sl = (void *)Handle;
    hal_spin_lock_cli( sl );
    return (0);
}


void
AcpiOsReleaseLock (
    ACPI_SPINLOCK           Handle,
    ACPI_CPU_FLAGS          Flags)
{
    (void) Flags;
    //AcpiOsSignalSemaphore (Handle, 1);
    hal_spinlock_t *sl = (void *)Handle;
    hal_spin_unlock_sti( sl );
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsInstallInterruptHandler
 *
 * PARAMETERS:  InterruptNumber     - Level handler should respond to.
 *              Isr                 - Address of the ACPI interrupt handler
 *              ExceptPtr           - Where status is returned
 *
 * RETURN:      Handle to the newly installed handler.
 *
 * DESCRIPTION: Install an interrupt handler. Used to install the ACPI
 *              OS-independent handler.
 *
 *****************************************************************************/

UINT32
AcpiOsInstallInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine,
    void                    *Context)
{
    errno_t rc = hal_irq_alloc( InterruptNumber, (void *)ServiceRoutine, Context, 1 );
    if( rc ) return AE_ERROR;

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsRemoveInterruptHandler
 *
 * PARAMETERS:  Handle              - Returned when handler was installed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Uninstalls an interrupt handler.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine)
{
    hal_irq_free( InterruptNumber, (void *)ServiceRoutine, 0 );
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsStall
 *
 * PARAMETERS:  microseconds        - Time to sleep
 *
 * RETURN:      Blocks until sleep is completed.
 *
 * DESCRIPTION: Sleep at microsecond granularity
 *
 *****************************************************************************/

void
AcpiOsStall ( UINT32 microseconds )
{
    if(microseconds > 0)
    {
        microseconds -= 10;
        tenmicrosec();
    }
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsSleep
 *
 * PARAMETERS:  milliseconds        - Time to sleep
 *
 * RETURN:      Blocks until sleep is completed.
 *
 * DESCRIPTION: Sleep at millisecond granularity
 *
 *****************************************************************************/

void
AcpiOsSleep ( UINT64 milliseconds )
{
    hal_sleep_msec((int)milliseconds);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetTimer
 *
 * PARAMETERS:  None
 *
 * RETURN:      Current time in 100 nanosecond units
 *
 * DESCRIPTION: Get the current system time
 *
 *****************************************************************************/

UINT64
AcpiOsGetTimer (void)
{
    return 10 * hal_local_time();
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadPciConfiguration
 *
 * PARAMETERS:  PciId               - Seg/Bus/Dev
 *              Register            - Device Register
 *              Value               - Buffer where value is placed
 *              Width               - Number of bits
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read data from PCI configuration space
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Register,
    UINT64                  *Value,
    UINT32                  Width)
{
    int sz = 4;
    switch(Width)
    {
    case  8: sz = 1; break;
    case 16: sz = 2; break;
    }

    u_int32_t ret = phantom_pci_read(PciId->Bus, PciId->Device, PciId->Function, Register, sz );
    *Value = ret;

    return AE_OK;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritePciConfiguration
 *
 * PARAMETERS:  PciId               - Seg/Bus/Dev
 *              Register            - Device Register
 *              Value               - Value to be written
 *              Width               - Number of bits
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Write data to PCI configuration space
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Register,
    UINT64                  Value,
    UINT32                  Width)
{
    (void) PciId;
    (void) Register;
    (void) Value;
    (void) Width;

    return AE_ERROR;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadPort
 *
 * PARAMETERS:  Address             - Address of I/O port/register to read
 *              Value               - Where value is placed
 *              Width               - Number of bits
 *
 * RETURN:      Value read from port
 *
 * DESCRIPTION: Read data from an I/O port or register
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width)
{
    switch (Width)
    {
    case 8:
        *Value = inb(Address);
        break;

    case 16:
        *Value = inw(Address);
        break;

    case 32:
        *Value = inl(Address);
        break;

    default:
        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritePort
 *
 * PARAMETERS:  Address             - Address of I/O port/register to write
 *              Value               - Value to write
 *              Width               - Number of bits
 *
 * RETURN:      None
 *
 * DESCRIPTION: Write data to an I/O port or register
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWritePort ( ACPI_IO_ADDRESS Address,
    UINT32 Value,
    UINT32 Width)
{
    switch (Width)
    {
    case 8:
        outb(Address,Value);
        break;

    case 16:
        outw(Address,Value);
        break;

    case 32:
        outl(Address,Value);
        break;

    default:
        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadMemory
 *
 * PARAMETERS:  Address             - Physical Memory Address to read
 *              Value               - Where value is placed
 *              Width               - Number of bits
 *
 * RETURN:      Value read from physical memory address
 *
 * DESCRIPTION: Read data from a physical memory address
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  *Value,
    UINT32                  Width)
{

    switch (Width)
    {
    case 8:
        *Value = *((u_int8_t *)Address);
        break;
    case 16:
        *Value = *((u_int16_t *)Address);
        break;
    case 32:
        *Value = *((u_int32_t *)Address);
        break;

    default:
        return (AE_BAD_PARAMETER);
    }
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWriteMemory
 *
 * PARAMETERS:  Address             - Physical Memory Address to write
 *              Value               - Value to write
 *              Width               - Number of bits
 *
 * RETURN:      None
 *
 * DESCRIPTION: Write data to a physical memory address
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  Value,
    UINT32                  Width)
{
    (void) Address;
    (void) Value;
    (void) Width;

    return AE_ERROR;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadable
 *
 * PARAMETERS:  Pointer             - Area to be verified
 *              Length              - Size of area
 *
 * RETURN:      TRUE if readable for entire length
 *
 * DESCRIPTION: Verify that a pointer is valid for reading
 *
 *****************************************************************************/

BOOLEAN
AcpiOsReadable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
    (void) Pointer;
    (void) Length;
    return (TRUE);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritable
 *
 * PARAMETERS:  Pointer             - Area to be verified
 *              Length              - Size of area
 *
 * RETURN:      TRUE if writable for entire length
 *
 * DESCRIPTION: Verify that a pointer is valid for writing
 *
 *****************************************************************************/

BOOLEAN
AcpiOsWritable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
    (void) Pointer;
    (void) Length;

    return (TRUE);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsSignal
 *
 * PARAMETERS:  Function            - ACPI CA signal function code
 *              Info                - Pointer to function-dependent structure
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Miscellaneous functions. Example implementation only.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{
    (void) Info;
    printf("acpi sig\n");
    switch (Function)
    {
    case ACPI_SIGNAL_FATAL:
        break;

    case ACPI_SIGNAL_BREAKPOINT:
        break;

    default:
        break;
    }

    return (AE_OK);
}

/* Optional multi-thread support */

#ifndef ACPI_SINGLE_THREADED
/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetThreadId
 *
 * PARAMETERS:  None
 *
 * RETURN:      Id of the running thread
 *
 * DESCRIPTION: Get the ID of the current (running) thread
 *
 *****************************************************************************/

ACPI_THREAD_ID
AcpiOsGetThreadId (
    void)
{
    return (ACPI_CAST_PTHREAD_T (get_current_tid()));
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsExecute
 *
 * PARAMETERS:  Type                - Type of execution
 *              Function            - Address of the function to execute
 *              Context             - Passed as a parameter to the function
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Execute a new thread
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context)
{
    (void) Type;
    tid_t t = hal_start_thread( Function, Context, 0 );
    if(t <= 0)
    {
        AcpiOsPrintf("Create thread failed");
        return AE_ERROR;
    }
    return (0);
}

#endif /* ACPI_SINGLE_THREADED */

#endif // ARCH_ia32
