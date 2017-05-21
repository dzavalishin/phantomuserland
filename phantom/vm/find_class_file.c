/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Load class from filesystem file.
 * Strinc Phantom kernel environment works withount file system,
 * but debug env lets us access host file system from within Phantom.
 * It is handy to have ability toload class files from there.
 *
**/


#include <vm/bulk.h>
#include "main.h"


static int do_load_class_from_file(const char *fn, struct pvm_object *out)
{
    void *code;
    unsigned int size;
    int rc = load_code( &code, &size, fn);

    if(rc)
        return rc;

    //struct pvm_object out;
    rc = pvm_load_class_from_memory( code, size, out );

    free(code);
    return rc;
}


int load_class_from_file(const char *cn, struct pvm_object *out)
{
    char * have_suffix = (char *)strstr( cn, ".pc" );

    if(*cn == '.') cn++;

    // TODO check all automounts?
    char *path[] =
    {
        ".", // Reserved for getenv search

//        ".",
        "/amnt0/class",
        "/amnt1/class",
        "/amnt2/class",
        "/amnt3/class",
        "../../plib/bin",
//        "../compiler",
//        "./pcode",
        0
    };

/*
    char *dir = getenv("PHANTOM_HOME");
    char *rest = "plib/bin";

    if( dir == NULL )
    {
        dir = "pcode";
        rest = "classes";
    }

    char fn[1024];
    snprintf( fn, 1024, "%s/%s", dir, rest );
    path[0] = fn;
*/

#define BS 1024
    char       buf[BS+1];

    char **prefix;
    for( prefix = path; *prefix; prefix++ )
    {
        snprintf( buf, BS, "%s/%s%s", *prefix, cn, have_suffix ? "" : ".pc" );

        //printf("try '%s'\n", buf );
        if(!do_load_class_from_file(buf, out))
        {
            printf("OK: File found for class '%s'\n", cn );
            return 0;
        }
    }

    printf("ERR: File not found for class '%s'\n", cn );

    return 1;
}


//phantom_is_a_real_kernel()
