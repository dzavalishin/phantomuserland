/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Intel 32 bit - usual pc
 *
**/

#include <kernel/board.h>
#include <kernel/init.h>
#include <kernel/drivers.h>

#include <i386/isa/pic.h>
#include <i386/isa/pic_regs.h>

char board_name[] = "PC32";


#include "common-pc.c"
