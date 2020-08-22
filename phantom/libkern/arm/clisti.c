/******************************************************************************
 *
 * $RCSfile: $
 * $Revision: $
 *
 * This module provides the interface routines controlling the various
 * interrupt modes present on the ARM processor.
 * Copyright 2004, R O SoftWare
 * No guarantees, warrantees, or promises, implied or otherwise.
 * May be used for hobby or commercial purposes provided copyright
 * notice remains intact.
 *
 *****************************************************************************/

#include <hal.h>
#include <arm/proc_reg.h>

#define IRQ_MASK  0x00000080
#define FIQ_MASK  0x00000040
#define INTs_MASK (IRQ_MASK | FIQ_MASK)

/*
IF ({ARCHITECTURE} = "6"):LOR:({ARCHITECTURE} = "6K"):LOR:({ARCHITECTURE} = "6T2"):LOR:({ARCHITECTURE} = "6Z")
MOV R0, #0
MCR p15, 0, r0, c7, c0, 4
ELIF ({ARCHITECTURE} = "5T"):LOR:({ARCHITECTURE} = "5TE"):LOR:({ARCHITECTURE} = "5TEJ")
MOV R0, #0
MCR p15, 0, r0, c7, c0, 4
ELIF ({ARCHITECTURE} = "7"):LOR:({ARCHITECTURE} = "7-A"):LOR:({ARCHITECTURE} = "7-R"):LOR:({ARCHITECTURE} = "7-M")
WFI
ELSE
NOP
ENDIF
*/

void hal_wait_for_interrupt(void)
{
    hal_sti();
    // TODO ARM9 has "MCR p15,0,Rd,c7,c0,4" wait for interrupt
    //__asm __volatile("wfi" : : );
	__asm __volatile("\
		MOV R0, #0; \
		MCR p15, 0, r0, c7, c0, 4; \
	    " : : );
}


// gcc generates code which does not affect IRQ/FIQ flags!
#if 0
/**
 *
 * Gets the value of the CPSR.
 *
 * Returns:
 *    current value of CPSR
 *
 **/

static inline unsigned __get_cpsr(void)
{
    unsigned long retval;
    asm volatile ("mrs  %0, cpsr_all" : "=r" (retval) : /* no inputs */  );
    return retval;
}

/**
 *
 * Sets the CPSR to the supplied value.
 *
 *
**/
static inline void __set_cpsr(unsigned val)
{
    asm volatile ("msr  cpsr_all, %0" : /* no outputs */ : "r" (val)  );
}

#endif







void hal_cli()
{
    unsigned _cpsr = __get_cpsr();

    do
        __set_cpsr(_cpsr | IRQ_MASK);
    while (!(__get_cpsr() & IRQ_MASK));
}

// NB! interrupt/trap entry code is not reentrant! FIQs are disabled here!

void hal_sti()
{
    unsigned _cpsr;

    _cpsr = __get_cpsr();

    do
        __set_cpsr(_cpsr & ~IRQ_MASK);
    while ((__get_cpsr() ^ ~IRQ_MASK) & IRQ_MASK);

}

int hal_is_sti()
{
    return !(__get_cpsr() & IRQ_MASK);
}

int hal_save_cli()
{
    unsigned _cpsr;

    _cpsr = __get_cpsr();

    do
        __set_cpsr(_cpsr | IRQ_MASK);
    while (!(__get_cpsr() & IRQ_MASK));

    return !(_cpsr & IRQ_MASK);
}








#if 0
#include "armISR.h"




/******************************************************************************
 *
 * Function Name: disableIRQ()
 *
 * Description:
 *    This function sets the IRQ disable bit in the status register and
 *    returns the previous value.
 *
 * Calling Sequence:
 *    void
 *
 * Returns:
 *    previous value of CPSR
 *
 *****************************************************************************/
unsigned disableIRQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();

  do
    __set_cpsr(_cpsr | IRQ_MASK);
  while (!(__get_cpsr() & IRQ_MASK));

  return _cpsr;
}

/******************************************************************************
 *
 * Function Name: enableIRQ()
 *
 * Description:
 *    This function clears the IRQ disable bit in the status register
 *    and returns the previous value.
 *
 * Calling Sequence:
 *    void
 *
 * Returns:
 *    previous value of CPSR
 *
 *****************************************************************************/
unsigned enableIRQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();

  do
    __set_cpsr(_cpsr & ~IRQ_MASK);
  while (__get_cpsr() & IRQ_MASK);

  return _cpsr;
}

/******************************************************************************
 *
 * Function Name: restoreIRQ()
 *
 * Description:
 *    This function restores the IRQ disable bit in the status register
 *    to the value contained within passed oldCPSR
 *
 * Calling Sequence:
 *    new value for the CPSR
 *
 * Returns:
 *    previous value of CPSR
 *
 *****************************************************************************/
unsigned restoreIRQ(unsigned oldCPSR)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();

  do
    __set_cpsr((_cpsr & ~IRQ_MASK) | (oldCPSR & IRQ_MASK));
  while ((__get_cpsr() ^ oldCPSR) & IRQ_MASK);

  return _cpsr;
}

/******************************************************************************
 *
 * Function Name: disableFIQ()
 *
 * Description:
 *    This function sets the FIQ disable bit in the status register
 *
 * Calling Sequence:
 *    void
 *
 * Returns:
 *    previous value of CPSR
 *
 *****************************************************************************/
unsigned disableFIQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();

  do
    __set_cpsr(_cpsr | FIQ_MASK);
  while (!(__get_cpsr() & FIQ_MASK));

  return _cpsr;
}

/******************************************************************************
 *
 * Function Name: enableFIQ()
 *
 * Description:
 *    This function clears the FIQ disable bit in the status register
 *
 * Calling Sequence:
 *    void
 *
 * Returns:
 *    previous value of CPSR
 *
 *****************************************************************************/
unsigned enableFIQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();

  do
    __set_cpsr(_cpsr & ~FIQ_MASK);
  while (__get_cpsr() & FIQ_MASK);

  return _cpsr;
}

/******************************************************************************
 *
 * Function Name: restoreFIQ()
 *
 * Description:
 *    This function restores the FIQ disable bit in the status register
 *    to the value contained within passed oldCPSR
 *
 * Calling Sequence:
 *    new value for the CPSR
 *
 * Returns:
 *    previous value of CPSR
 *
 *****************************************************************************/
unsigned restoreFIQ(unsigned oldCPSR)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();

  do
    __set_cpsr((_cpsr & ~FIQ_MASK) | (oldCPSR & FIQ_MASK));
  while ((__get_cpsr() ^ oldCPSR) & FIQ_MASK);

  return _cpsr;
}

/******************************************************************************
 *
 * Function Name: disableINTs()
 *
 * Description:
 *    This function sets the IRQ & FIQ disable bits in the status register
 *
 * Calling Sequence:
 *    void
 *
 * Returns:
 *    previous value of CPSR
 *
 *****************************************************************************/
unsigned disableINTs(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();

  do
    __set_cpsr(_cpsr | INTs_MASK);
  while ((__get_cpsr() ^ INTs_MASK) & INTs_MASK);

  return _cpsr;
}

/******************************************************************************
 *
 * Function Name: enableINTs()
 *
 * Description:
 *    This function clears the IRQ & FIQ disable bits in the status register
 *
 * Calling Sequence:
 *    void
 *
 * Returns:
 *    previous value of CPSR
 *
 *****************************************************************************/
unsigned enableINTs(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();

  do
    __set_cpsr(_cpsr & ~INTs_MASK);
  while ((__get_cpsr() ^ ~INTs_MASK) & INTs_MASK);

  return _cpsr;
}

/******************************************************************************
 *
 * Function Name: restoreINTs()
 *
 * Description:
 *    This function restores the IRQ & FIQ disable bits in the status register
 *    to the value contained within passed oldCPSR
 *
 * Calling Sequence:
 *    new value for the CPSR
 *
 * Returns:
 *    previous value of CPSR
 *
 *****************************************************************************/
unsigned restoreINTs(unsigned oldCPSR)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();

  do
    __set_cpsr((_cpsr & ~INTs_MASK) | (oldCPSR & INTs_MASK));
  while ((__get_cpsr() ^ oldCPSR) & INTs_MASK);

  return _cpsr;
}
#endif



