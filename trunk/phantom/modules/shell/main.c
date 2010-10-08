/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "commands.h"
#include "parse.h"
#include "statements.h"
#include "shell_defs.h"
#include "shell_vars.h"
#include "args.h"


int main(int argc,char *argv[])
{
	char buf[1024];

	init_vars();
	init_statements();
	init_arguments(argc,argv);

	if(af_script_file_name != NULL){
		run_script(af_script_file_name);
		if(af_exit_after_script) exit(0);
	}

	/*
	for(;;) {

		printf("> ");

		fgets(buf, sizeof(buf), stdin);
 		if(strlen(buf) > 0) {
			parse_string(buf);
		}
		buf[0] = '\0';
	}
	*/

	return 0;
}


