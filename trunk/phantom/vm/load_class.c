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


//---------------------------------------------------------------------------

#include <phantom_libc.h>

#include <endian.h>

#include "vm/object.h"
#include "vm/internal_da.h"
#include "vm/code.h"
#include "vm/bulk.h"
#include "vm/alloc.h"

#include "vm/p2c.h"
#include "vm/alloc.h"


int debug_print = 0;


static int vm_code_linenum_cmp(const void *, const void *);



//---------------------------------------------------------------------------
// Classs loader
//---------------------------------------------------------------------------


struct method_loader_handler
{
    struct pvm_code_handler 		ch;

    struct pvm_object 			my_name;
    struct pvm_object 			my_code;
    int            			my_ordinal;
};

static void
pvm_load_method( struct method_loader_handler *mh, const unsigned char *data, int in_size )
{

    mh->ch.code = data;
    mh->ch.IP_max = in_size;
    mh->ch.IP = 0;

    pvm_object_t name = pvm_code_get_string(&(mh->ch));

    if(debug_print) printf("Method is: " );
    if(debug_print) pvm_object_print( name );
    mh->my_name = name;
    //ref_dec_o(name);

    mh->my_ordinal = pvm_code_get_int32(&(mh->ch));
    if(debug_print) printf(", ordinal %d\n", mh->my_ordinal );

    int code_size   = in_size - mh->ch.IP;
    const unsigned char *code_data = mh->ch.code+mh->ch.IP;

    //if(debug_print) printf("code size %d, IP = %d, in_size = %d\n", code_size, IP, in_size );

    mh->my_code = pvm_create_code_object( code_size, (void *)code_data );
}



int pvm_load_class_from_memory( const void *data, int fsize, struct pvm_object *out )
{
    const unsigned char *rec_start = (const unsigned char *)data;
    int record_size = 0;

    struct pvm_object class_name;
    struct pvm_object base_class = pvm_get_null_class();

    int n_object_slots = 0; // for a new class
    int n_method_slots = 0;


    struct pvm_object iface;
    struct pvm_object ip2line_maps;
    struct pvm_object method_names;

    int got_class_header = 0;

    for( ; rec_start < (const unsigned char *)data + fsize; rec_start = rec_start+record_size )
    {
        const unsigned char *ptr = rec_start;

        //printf("%d bytes left\n", data + fsize - ptr );

        if( strncmp( (const char *)ptr, "phfr:", 5 ) )
        {
            printf("No record marker\n" );
            return 1;
        }

        ptr += 5;

        char record_type = *ptr++;
        //record_size = htonl( *((long *)ptr)++ );
        record_size = htonl( *((long *)ptr) );
        ptr += sizeof(long);

        if(debug_print) printf("type '%c', size %4d: ", record_type, record_size );

        if( record_size < 6+8 )
        {
            printf("Invalid record size\n" );
            return 1;
        }

        switch( record_type )
        {
        case 'C': // class
            {
                struct pvm_code_handler h;//( ptr, record_size - (ptr-rec_start));
                h.IP = 0;
                h.code = ptr;
                h.IP_max = record_size - (ptr-rec_start);

                if(debug_print) printf("Class is: " );

                class_name = pvm_code_get_string(&h);//.get_string();
                if(debug_print) pvm_object_print( class_name );

                n_object_slots = pvm_code_get_int32(&h); //.get_int32();
                if(debug_print) printf(", %d fileds", n_object_slots );

                n_method_slots = pvm_code_get_int32(&h);
                if(debug_print) printf(", %d methods\n", n_method_slots );

                struct pvm_object base_name = pvm_code_get_string(&h);
                //if(debug_print)
                {
                    printf("Based on: ");
                    pvm_object_print( base_name );
                    printf("\n");
                }

                // TODO turn on later, when we're sure all class collections have it
                //struct pvm_object version_string = pvm_code_get_string(&h);

                got_class_header = 1;
#if 0
#warning base class ignored
#else
                if( EQ_STRING_P2C(base_name,".internal.object") )
                    base_class = pvm_get_null_class();
                else
                    base_class = pvm_exec_lookup_class_by_name(base_name);
#endif
                ref_dec_o(base_name);

                // TODO BREAK ALERT make sure base class is not internal

                iface = pvm_create_interface_object( n_method_slots, base_class );
                ip2line_maps = pvm_object_create_dynamic( pvm_get_array_class(), n_method_slots );
                method_names = pvm_object_create_dynamic( pvm_get_array_class(), n_method_slots );

            }
            break;

        case 'M': // class
            {
                struct method_loader_handler mh;
                pvm_load_method( &mh, ptr, record_size - (ptr-rec_start));
                //methods.push_back(mh);
                //iface.save( mh.my_ordinal, mh.my_code );
                pvm_set_ofield( iface, mh.my_ordinal, mh.my_code );
                pvm_set_ofield( method_names, mh.my_ordinal, mh.my_name );
            }
            break;

        case 'S': // method signature
            {
                if(debug_print) printf("\n" );
            }
            break;

        case 'l': // IP to line num map
            {
                struct pvm_code_handler h;
                h.IP = 0;
                h.code = ptr;
                h.IP_max = record_size - (ptr-rec_start);

                if(debug_print) printf("line num map\n" );
                int ordinal = pvm_code_get_int32(&h);
                int mapsize = pvm_code_get_int32(&h);


                pvm_object_t map = pvm_create_binary_object(mapsize*(sizeof(struct vm_code_linenum)), 0);

                struct data_area_4_binary *bin= pvm_object_da( map, binary );

                struct vm_code_linenum *sp = (void *)bin->data;

                int i;
                for( i = 0 ; i < mapsize; i++ )
                {
                	sp->ip = pvm_code_get_int32(&h);
                	sp->line = pvm_code_get_int32(&h);
                	sp++;
                }

                qsort( bin->data, mapsize, sizeof(struct vm_code_linenum), vm_code_linenum_cmp );

                pvm_set_ofield( ip2line_maps, ordinal, map );
            }
            break;
        }

    }

    if( !got_class_header )
        return 1;


    // BUG! Need to check that class name and file name correspond?

    //if(debug_print) printf("Loaded %d methods\n", methods.size() );

    // Creating class
    //data_area_4_class       da(iface,class_name,base_class);
    //da.object_data_area_size   = sizeof(struct pvm_object) * n_object_slots;
    //da.object_flags     = 0;
    // BUG! Need
    //da.class_name       =

    struct pvm_object new_class = pvm_create_class_object(class_name, iface, sizeof(struct pvm_object) * n_object_slots);

    struct data_area_4_class *cda= pvm_object_da( new_class, class );
    cda->ip2line_maps = ip2line_maps;
    cda->method_names = method_names;

    *out = new_class;
    return 0;
}



static int vm_code_linenum_cmp(const void *_a, const void *_b)
{
	const struct vm_code_linenum *a = _a;
	const struct vm_code_linenum *b = _b;
	return (a->ip == b->ip) ? 0 : ( (a->ip > b->ip) ? 1 : 0 );
}







