/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * VM - code (class) loader helper
 *
 *    * load from bulk (binary multi-class image brought from
 *      kernel module or elsewhere)
 *
 *    * load from file (if some FS is available and UNIX subsys
 *      compiled in.
 *
 *    * TODO load from network
 *    * TODO check class signature - better do it in VM code
 *
**/

#define DEBUG_MSG_PREFIX "vm_load"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include <multiboot.h>

#include <kernel/init.h>
#include <kernel/vm.h>

#include <sys/fcntl.h>
#include <sys/stat.h>
#include <kunix.h>

#include <vm/bulk.h>

#if 1
int load_code(void **out_code, unsigned int *out_size, const char *fn)
{
#if HAVE_UNIX
    return k_load_file( out_code, out_size, fn );
#else
    (void)out_code;
    (void)out_size;
    (void)fn;

    return ENOENT;
#endif
}
#else
// TODO use k_load_file?
int load_code(void **out_code, unsigned int *out_size, const char *fn)
{
    errno_t ret;
    long fsize = -1;

    SHOW_FLOW( 6, "load '%s'", fn );

    struct stat st;
    if( k_stat( fn, &st, 0 ) )
    {
        SHOW_ERROR( 0, "can't stat '%s'", fn );
        return ENOENT;
    }

    fsize = st.st_size;

    SHOW_FLOW( 6, "will open '%s', %d bytes", fn, fsize );

    int fd;
    if( (ret = k_open( &fd, fn, _O_RDONLY, 0 )) )
    {
        SHOW_ERROR( 0, "can't open '%s', err %d", fn, ret );
        return ENOENT;
    }

    unsigned char *code = (unsigned char *)malloc(fsize);
    if( code == 0 )
    {
        SHOW_ERROR( 0, "can't alloc %d", fsize );
        return ENOMEM;
    }

    int nread;
    ret = k_read( &nread, fd, code, fsize );
    if( ret || (fsize != nread) )
    {
        SHOW_ERROR( 0, "Can't read code: ret = %d", ret );
        free( code );
        return EIO;
    }

    k_close( fd );

    *out_size = (unsigned)fsize;
    *out_code = code;

    return 0;
}
#endif



// -----------------------------------------------------------------------
// Boot module classloader support
// -----------------------------------------------------------------------



static void *bulk_code;
static unsigned int bulk_size = 0;
static void *bulk_read_pos;

int bulk_seek_f( int pos )
{
    bulk_read_pos = bulk_code + pos;
    return bulk_read_pos >= bulk_code + bulk_size;
}

int bulk_read_f( int count, void *data )
{
    if( count < 0 )
        return -1;

    int left = (bulk_code + bulk_size) - bulk_read_pos;

    if( count > left )
        count = left;

    memcpy( data, bulk_read_pos, count );

    bulk_read_pos += count;

    return count;
}

void load_classes_module()
{
    // In fact we need this only if boot classloader is called,
    // and it is called only if completely fresh system is set up
    struct multiboot_module *classes_module = phantom_multiboot_find("classes");

    SHOW_FLOW( 2, "Classes boot module is %sfound", classes_module ? "" : "not " );

    bulk_read_pos = bulk_code;
    bulk_size = 0;

    if(classes_module != 0)
    {
        bulk_code = (void *)phystokv(classes_module->mod_start);
        bulk_size = classes_module->mod_end - classes_module->mod_start;
    }
    else
    {
        // Don't panic - there are more ways to obtain classes
        //SHOW_ERROR0( 0, "Classes boot module is not found" );
        //panic("no boot classes module found");
    }

    pvm_bulk_init( bulk_seek_f, bulk_read_f );
}


