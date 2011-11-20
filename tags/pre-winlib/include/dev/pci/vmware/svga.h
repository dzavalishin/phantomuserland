/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

/*
 * svga.h --
 *
 *      This is a simple example driver for the VMware SVGA device.
 *      It handles initialization, register accesses, low-level
 *      command FIFO writes, and host/guest synchronization.
 */

#ifndef __VMWARE_SVGA_H__
#define __VMWARE_SVGA_H__

#include <shorttypes.h>
#include <kernel/bus/pci.h>
//#include "intr.h"

// XXX: Shouldn't have to do this here.
#define INLINE __inline__

#include <dev/pci/vmware/svga_reg.h>
#include <dev/pci/vmware/svga_escape.h>
#include <dev/pci/vmware/svga_overlay.h>
#include <dev/pci/vmware/svga3d_reg.h>

typedef struct SVGADevice {
    int           found;
    //PCIAddress pciAddr;

    u_int32_t     ioBase;
    physaddr_t    fifoPhys;
    u_int32_t    *fifoMem;
    physaddr_t    fbMem;
    u_int32_t     fifoSize;
    u_int32_t     fbSize;
    u_int32_t     fbOffset;

    u_int32_t     deviceVersionId;
    u_int32_t     capabilities;

    u_int32_t     width;
    u_int32_t     height;
    u_int32_t     bpp;
    u_int32_t     pitch;

    struct {
        u_int32_t  reservedSize;
        Bool    usingBounceBuffer;
        uint8   bounceBuffer[1024 * 1024];
        u_int32_t  nextFence;
    } fifo;

    volatile struct {
        u_int32_t        pending;
        u_int32_t        switchContext;

        // trap_state
        //IntrContext   oldContext;
        //IntrContext   newContext;

        u_int32_t        count;
    } irq;

} SVGADevice;

extern SVGADevice gSVGA;

void SVGA_Init(void);
void SVGA_SetMode(u_int32_t width, u_int32_t height, u_int32_t bpp);
void SVGA_Disable(void);
void SVGA_Panic(const char *err);
void SVGA_DefaultFaultHandler(int vector);

u_int32_t SVGA_ReadReg(u_int32_t index);
void SVGA_WriteReg(u_int32_t index, u_int32_t value);
u_int32_t SVGA_ClearIRQ(void);
u_int32_t SVGA_WaitForIRQ();

Bool SVGA_IsFIFORegValid(unsigned int reg);
Bool SVGA_HasFIFOCap(int cap);

void *SVGA_FIFOReserve(u_int32_t bytes);
void *SVGA_FIFOReserveCmd(u_int32_t type, u_int32_t bytes);
void *SVGA_FIFOReserveEscape(u_int32_t nsid, u_int32_t bytes);
void SVGA_FIFOCommit(u_int32_t bytes);
void SVGA_FIFOCommitAll(void);

u_int32_t SVGA_InsertFence(void);
void SVGA_SyncToFence(u_int32_t fence);
Bool SVGA_HasFencePassed(u_int32_t fence);
void SVGA_RingDoorbell(void);

/* 2D commands */

void SVGA_Update(u_int32_t x, u_int32_t y, u_int32_t width, u_int32_t height);
void SVGA_BeginDefineCursor(const SVGAFifoCmdDefineCursor *cursorInfo,
                            void **andMask, void **xorMask);
void SVGA_BeginDefineAlphaCursor(const SVGAFifoCmdDefineAlphaCursor *cursorInfo,
                                 void **data);
void SVGA_MoveCursor(u_int32_t visible, u_int32_t x, u_int32_t y, u_int32_t screenId);

void SVGA_BeginVideoSetRegs(u_int32_t streamId, u_int32_t numItems,
                            SVGAEscapeVideoSetRegs **setRegs);
void SVGA_VideoSetAllRegs(u_int32_t streamId, SVGAOverlayUnit *regs, u_int32_t maxReg);
void SVGA_VideoSetReg(u_int32_t streamId, u_int32_t registerId, u_int32_t value);
void SVGA_VideoFlush(u_int32_t streamId);

#endif /* __VMWARE_SVGA_H__ */
