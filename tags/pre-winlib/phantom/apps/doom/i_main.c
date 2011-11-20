// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//      Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_main.c,v 1.4 1997/02/03 22:45:10 b1 Exp $";

#include "doomdef.h"

#include "m_argv.h"
#include "d_main.h"
#include "kolibri.h"

int main(int argc, char **argv) 
{ 
//    static char * tmp[]=
//      {"/hd/1/menuetos/doom/mdoom",
//       NULL};
    myargc = 1; 
    myargv = argv;
 
//  InitHeap(32*1024*1024);
  __asm__ (
  " \
    pushl %ebx        ;\
    pushl %ecx        ;\
    movl $66, %eax     ;\
    movl $1, %ebx      ;\
    movl $1, %ecx      ;\
    int $0x40        ;\
    popl %ecx         ;\
    popl %ebx         ;\
  ");
  
    D_DoomMain (); 

    return 0;
} 
