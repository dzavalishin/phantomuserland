#error dont use me
// used by NE2000 only?


/* $Id: //depot/blt/srv/ne2000/err.h#2 $
**
** Copyright 1998 Brian J. Swetland
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions, and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions, and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __ERR
#define __ERR

/* NOTE If ret is negative then it is an error otherwise it returns a 
//	success code. */

/////
// Error codes
/////

#define NUM_ERR		22
#define NOERR		0
#define ERR		-1
#define ERRNOMEM	-2
#define ERRRESOURCE	-3
#define ERRMEMCORRUPT	-4
#define ERRFORMAT	-5
#define ERRNOTFOUND	-6
#define ERRTYPE		-7
#define ERRTIMEOUT	-8
#define ERRNOTSUPPORTED	-9
#define ERROUTOFRANGE	-10
#define ERRPRIV		-11
#define ERRNOTREADY	-12
#define ERRNONEFREE	-13
#define ERRARG		-14
#define ERRINVALID	-15
#define ERRNOTOPEN	-16
#define ERRALREADYDONE	-17
#define ERRVER		-18
#define ERROVERFLOW	-19
#define ERRINUSE	-20
#define ERRTOOBIG	-21

/////
// Success codes
/////

#define SFOUND		1
#define SNOTFOUND	2
#define SALT		3

/////
// Facility codes
/////

/* Bits 16-31 are reserved for the facility code. */
#define OWNOS		0x70
#define OWNNOMAD	0x70

/////
// Error Messages
/////

#define MAX_ERR_MSG	32
static const char err_msg[NUM_ERR][MAX_ERR_MSG] = {  "success",
					"error",
					"out of memory",
					"resource allocation",
					"memory corrupt!!!",
					"format",
					"not found",
					"type",
					"timeout",
					"not supported",
					"out of range",
					"access denied",
					"not ready",	
					"none free",
					"bad argument",
					"invalid",
					"not open",
					"already done",
					"incorrect version",
					"overflow",
					"in use",
					"too big" };

/////
// Return code object
/////

//typedef int ret_t;


#endif /* __ERR */
