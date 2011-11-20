#include <vm/bulk.h>

int fwrite( void *, int, int, void * );

extern void *outf;

void save_hdr( char *classnm, long size )
{
    struct pvm_bulk_class_head h;
    strncpy( h.name, classnm, PVM_BULK_CN_LENGTH );
    h.data_length = size;

    //return 1 ==
    fwrite( &h, sizeof(h), 1, outf );
}


