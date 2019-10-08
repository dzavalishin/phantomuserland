/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment.
 *
**/

void save_mem( void *addr, int size );


void setDiffMem( void *mem, void *copy, int size );
void checkDiffMem(void);


int load_class_from_file(const char *cn, pvm_object_t *out);


extern int debug_print_instr;
extern int debug_trace;


void videotest_truetype(void);
void videotest_overlay(void);
void videotest_pbm( void );

