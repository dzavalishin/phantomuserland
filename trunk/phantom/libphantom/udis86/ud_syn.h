/* -----------------------------------------------------------------------------
 * syn.h
 *
 * Copyright (c) 2006, Vivek Mohan <vivek@sig9.com>
 * All rights reserved. See LICENSE
 * -----------------------------------------------------------------------------
 */
#ifndef UD_SYN_H
#define UD_SYN_H

#include <phantom_libc.h>
#include <stdarg.h>
#include "ud_types.h"

extern const char* ud_reg_tab[];

static void mkasm(struct ud* u, const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  u->insn_fill += vsprintf((char*) u->insn_buffer + u->insn_fill, fmt, ap);
  va_end(ap);
}

#endif
