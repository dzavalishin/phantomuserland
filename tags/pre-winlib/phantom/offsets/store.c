#include "offsets.h"

#include <stdio.h>
#include <string.h>

FILE *cout;
FILE *jout;


static void out(const char *type, const char *field, const char *what, int data)
{
    static char *pad_spaces = "                                                                                                          ";

    int pad = 60-strlen(type)-strlen(field)-strlen(what);
    fprintf(cout, "#define %s__%s__%s %.*s%d\n", type, field, what, pad, pad_spaces, data);
    fprintf(jout, "\tfinal static int %s__%s__%s %.*s= %d;\n", type, field, what, pad, pad_spaces, data);
}

void out_offset(const char *type, const char *field, int offset)
{
    out(type, field, "OFFSET", offset);
}



int main(int ac, char **av)
{
    cout = fopen("field_offsets.h", "wt");
    jout = fopen("FieldOffsets.java", "wt");

    fprintf(jout, "package ru.dz.phantom;\n\n");
    fprintf(jout, "class FieldOffsets {\n\n");
    generate();
    fprintf(jout, "\n\n}\n");

    fflush(cout);
    fflush(jout);

    if( (ferror(cout) || ferror(jout) ) )
    {
        printf("File write error\n");
        return 1;
    }

    fclose(cout);
    fclose(jout);

    return 0;
}

void phantom_debug_register_stray_catch( void *a, int s, const char*n )
{
    // Ignore
    (void) a;
    (void) s;
    (void) n;
}

