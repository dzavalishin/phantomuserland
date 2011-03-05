#ifndef AMP_H
#define AMP_H

#include <stdio.h>
#include <unistd.h>
#include <compat/tabos.h>

#define FileRead(__buf, __n1, __n2, __f ) read( __f, __buf, (__n1)*(__n2) )
#define FileWrite(__buf, __n1, __n2, __f ) write( __f, __buf, (__n1)*(__n2) )
#define FileSeek lseek
#define Rewind(__f) lseek(__f,0,SEEK_SET)
#define FileEndOfFile eof

#define tell(__f) lseek(__f,0,SEEK_CUR)

/* this file is a part of amp software, (C) tomislav uzelac 1996,1997
*/

/* these should not be touched
*/
#define		SYNCWORD	0xfff
#define		TRUE		1
#define		FALSE		0

/* version 
*/
#define		MAJOR		0
#define		MINOR		7
#define		PATCH		6

#define MAX(a,b)	((a) > (b) ? (a) : (b))
#define MAX3(a,b,c)	((a) > (b) ? MAX(a, c) : MAX(b, c))
#define MIN(a,b)	((a) < (b) ? (a) : (b))


/* Debugging flags */

#define DEBUG

struct debugFlags_t {
  int audio,args,buffer,buffer2,misc,misc2;
};

struct debugLookup_t {
  char *name; int *var;
};

extern struct debugFlags_t debugFlags;

/* This is only here to keep all the debug stuff together */
#ifdef AMP_UTIL
struct debugLookup_t debugLookup[] = {
  {"audio", &debugFlags.audio},
  {"args",  &debugFlags.args},
  {"buffer",  &debugFlags.buffer},
  {"buffer2",  &debugFlags.buffer2},
  {"misc",  &debugFlags.misc},
  {"misc2",  &debugFlags.misc2},
  {0,0}
};
#endif /* AMP_UTIL */

extern struct debugLookup_t debugLookup[];


#ifdef DEBUG
  #define DB(type,cmd) if (debugFlags.type) { cmd ; }	
#else
  #define DB(type,cmd)
#endif

#include "proto.h"

#endif // AMP_H

