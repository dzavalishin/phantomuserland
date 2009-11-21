/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/


#include "vm/internal_da.h"


struct pvm_object 	pvm_code_get_string(struct pvm_code_handler *code);
unsigned int 		pvm_code_get_rel_IP_as_abs(struct pvm_code_handler *code);
int 			pvm_code_get_int32(struct pvm_code_handler *code);
unsigned char 		pvm_code_get_byte(struct pvm_code_handler *code);

unsigned char		pvm_code_get_byte_speculative(struct pvm_code_handler *code);
unsigned char		pvm_code_get_byte_speculative2(struct pvm_code_handler *code);

void 			pvm_code_check_bounds( struct pvm_code_handler *code, unsigned int ip, char *where );


int                     pvm_code_do_get_int( const unsigned char *addr );



