/**********************************************************
 * Copyright 1998-2009 VMware, Inc.  All rights reserved.
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
 * backdoor_def.h --
 *
 *    This contains constants that define the backdoor I/O port, a
 *    simple hypercall mechanism that is used by VMware Tools.
 */


#ifndef _BACKDOOR_DEF_H_
#define _BACKDOOR_DEF_H_

#define BDOOR_MAGIC 0x564D5868

#define BDOOR_PORT 0x5658

#define BDOOR_CMD_GETMHZ                1
#define BDOOR_CMD_APMFUNCTION           2
#define BDOOR_CMD_GETDISKGEO            3
#define BDOOR_CMD_GETPTRLOCATION        4
#define BDOOR_CMD_SETPTRLOCATION        5
#define BDOOR_CMD_GETSELLENGTH          6
#define BDOOR_CMD_GETNEXTPIECE          7
#define BDOOR_CMD_SETSELLENGTH          8
#define BDOOR_CMD_SETNEXTPIECE          9
#define BDOOR_CMD_GETVERSION            10
#define BDOOR_CMD_GETDEVICELISTELEMENT  11
#define BDOOR_CMD_TOGGLEDEVICE          12
#define BDOOR_CMD_GETGUIOPTIONS         13
#define BDOOR_CMD_SETGUIOPTIONS         14
#define BDOOR_CMD_GETSCREENSIZE         15
#define BDOOR_CMD_MONITOR_CONTROL       16
#define BDOOR_CMD_GETHWVERSION          17
#define BDOOR_CMD_OSNOTFOUND            18
#define BDOOR_CMD_GETUUID               19
#define BDOOR_CMD_GETMEMSIZE            20
#define BDOOR_CMD_HOSTCOPY              21 /* Devel only */
#define BDOOR_CMD_SERVICE_VM            22 /* prototype only */
#define BDOOR_CMD_GETTIME               23 /* Deprecated. Use GETTIMEFULL. */
#define BDOOR_CMD_STOPCATCHUP           24
#define BDOOR_CMD_PUTCHR                25 /* Devel only */
#define BDOOR_CMD_ENABLE_MSG            26 /* Devel only */
#define BDOOR_CMD_GOTO_TCL              27 /* Devel only */
#define BDOOR_CMD_INITPCIOPROM          28
#define BDOOR_CMD_INT13                 29
#define BDOOR_CMD_MESSAGE               30
#define BDOOR_CMD_RSVD0                 31
#define BDOOR_CMD_RSVD1                 32
#define BDOOR_CMD_RSVD2                 33
#define BDOOR_CMD_ISACPIDISABLED        34
#define BDOOR_CMD_TOE                   35 /* Not in use */
#define BDOOR_CMD_ISMOUSEABSOLUTE       36
#define BDOOR_CMD_PATCH_SMBIOS_STRUCTS  37
#define BDOOR_CMD_MAPMEM                38 /* Devel only */
#define BDOOR_CMD_ABSPOINTER_DATA       39
#define BDOOR_CMD_ABSPOINTER_STATUS     40
#define BDOOR_CMD_ABSPOINTER_COMMAND    41
#define BDOOR_CMD_TIMER_SPONGE          42
#define BDOOR_CMD_PATCH_ACPI_TABLES     43
#define BDOOR_CMD_DEVEL_FAKEHARDWARE    44 /* Debug only - needed in beta */
#define BDOOR_CMD_GETHZ                 45
#define BDOOR_CMD_GETTIMEFULL           46
#define BDOOR_CMD_STATELOGGER           47
#define BDOOR_CMD_CHECKFORCEBIOSSETUP   48
#define BDOOR_CMD_LAZYTIMEREMULATION    49
#define BDOOR_CMD_BIOSBBS               50
#define BDOOR_CMD_VASSERT               51
#define BDOOR_CMD_ISGOSDARWIN           52
#define BDOOR_CMD_DEBUGEVENT            53
#define BDOOR_CMD_OSNOTMACOSXSERVER     54
#define BDOOR_CMD_GETTIMEFULL_WITH_LAG  55
#define BDOOR_CMD_ACPI_HOTPLUG_DEVICE   56
#define BDOOR_CMD_ACPI_HOTPLUG_MEMORY   57
#define BDOOR_CMD_ACPI_HOTPLUG_CBRET    58
#define BDOOR_CMD_GET_HOST_VIDEO_MODES  59
#define BDOOR_CMD_ACPI_HOTPLUG_CPU      60
#define BDOOR_CMD_MAX                   61

/*
 * High-bandwidth backdoor port.
 */

#define BDOORHB_PORT 0x5659

#define BDOORHB_CMD_MESSAGE 0
#define BDOORHB_CMD_VASSERT 1
#define BDOORHB_CMD_MAX 2

/*
 * There is another backdoor which allows access to certain TSC-related
 * values using otherwise illegal PMC indices when the pseudo_perfctr
 * control flag is set.
 */

#define BDOOR_PMC_HW_TSC      0x10000
#define BDOOR_PMC_REAL_NS     0x10001
#define BDOOR_PMC_APPARENT_NS 0x10002

#define IS_BDOOR_PMC(index)  (((index) | 3) == 0x10003)
#define BDOOR_CMD(ecx)       ((ecx) & 0xffff)

#endif /* _BACKDOOR_DEF_H_ */
