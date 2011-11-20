/**********************************************************
 * Copyright 2002-2009 VMware, Inc.  All rights reserved.
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
 * vmmouse_defs.h --
 *
 *     VMware Virtual Mouse Protocol definitions.
 *
 */

#ifndef _VMMOUSE_DEFS_H_
#define _VMMOUSE_DEFS_H_

/*
 * Command related defines
 */
#define VMMOUSE_CMD_READ_ID             0x45414552
#define VMMOUSE_CMD_DISABLE             0x000000f5
#define VMMOUSE_CMD_REQUEST_RELATIVE    0x4c455252
#define VMMOUSE_CMD_REQUEST_ABSOLUTE    0x53424152

/*
 * Data related defines
 */
#define VMMOUSE_VERSION_ID_STR          "JUB4"
#define VMMOUSE_VERSION_ID              0x3442554a

/*
 * Device related defines
 */
#define VMMOUSE_DATAPORT                96
#define VMMOUSE_STATUSPORT              100
#define VMMOUSE_IRQ                     12
#define VMMOUSE_ERROR                   0xffff0000

/*
 * VMMouse Input packet flags
 */
#define VMMOUSE_MOVE_RELATIVE           1
#define VMMOUSE_MOVE_ABSOLUTE           0

/*
 * VMMouse Input button flags
 */
#define VMMOUSE_LEFT_BUTTON             0x20
#define VMMOUSE_RIGHT_BUTTON            0x10
#define VMMOUSE_MIDDLE_BUTTON           0x08

#endif
