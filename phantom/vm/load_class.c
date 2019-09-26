/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Load class from binary storage, such as filesystem file or blob from network.
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/VmLinker>
 *
**/


#include <phantom_libc.h>

#include <endian.h>

#include "vm/object.h"
#include "vm/object_flags.h"
#include "vm/internal_da.h"
#include "vm/code.h"
#include "vm/bulk.h"
#include "vm/alloc.h"

#include "vm/p2c.h"
#include "vm/alloc.h"


int debug_print = 0;


static int vm_code_linenum_cmp(const void *, const void *) __attribute__((used));



//---------------------------------------------------------------------------
// State keepers
//---------------------------------------------------------------------------


struct method_loader_handler
{
    struct pvm_code_handler     ch;

    pvm_object_t           my_name;
    pvm_object_t           my_code;
    int                         my_ordinal;
};


struct type_loader_handler
{
    struct pvm_code_handler     ch;

    pvm_object_t           class_name;
    pvm_object_t           contained_class_name;
    int                         is_container;
};


//---------------------------------------------------------------------------
// Load type def
//---------------------------------------------------------------------------


static void
pvm_load_type( struct pvm_code_handler *h , struct type_loader_handler *th )
{
    th->is_container = pvm_code_get_int32(h);
    th->class_name = pvm_code_get_string(h);
    th->contained_class_name = pvm_code_get_string(h);
}

static void
pvm_dump_type( struct type_loader_handler *th )
{
    pvm_object_print( th->class_name );

    if(th->is_container)
    {
        printf("[ ");
        pvm_object_print( th->contained_class_name );
        printf(" ]");
    }

}

//---------------------------------------------------------------------------
// Load one method
//---------------------------------------------------------------------------


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


//---------------------------------------------------------------------------
// Main entry point
//---------------------------------------------------------------------------


int pvm_load_class_from_memory( const void *data, int fsize, pvm_object_t *out )
{
    const unsigned char *rec_start = (const unsigned char *)data;
    int record_size = 0;

    pvm_object_t class_name;
    //pvm_object_t base_class = pvm_get_null_class();

    int n_object_slots = 0; // for a new class
    int n_method_slots = 0;


    pvm_object_t base_class = pvm_get_null_class();
    pvm_object_t iface        = { 0, 0 };
    pvm_object_t ip2line_maps = { 0, 0 };
    pvm_object_t method_names = { 0, 0 };
    pvm_object_t field_names  = { 0, 0 };
    pvm_object_t const_pool  = { 0, 0 };

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
        record_size = htonl( *((long *)ptr) ); // TODO meant to be ntohl?
        ptr += sizeof(long);

        if(debug_print) printf("type '%c', size %4d: ", record_type, record_size );

        if( record_size < 6+8 )
        {
            printf("Invalid record size\n" );
            return 1;
        }

        const int record_data_size = record_size - (ptr-rec_start);

        struct pvm_code_handler h;//( ptr, record_size - (ptr-rec_start));
        h.IP = 0;
        h.code = ptr;
        h.IP_max = record_data_size;

        switch( record_type )
        {
        case 'C': // class
            {
                if(0||debug_print) printf("Class is: " );

                class_name = pvm_code_get_string(&h);//.get_string();
                if(0||debug_print) pvm_object_print( class_name );

                n_object_slots = pvm_code_get_int32(&h); //.get_int32();
                if(debug_print) printf(", %d fields", n_object_slots );

                n_method_slots = pvm_code_get_int32(&h);
                if(debug_print) printf(", %d methods", n_method_slots );

                if(0||debug_print) printf("\n");	// terminate string

                pvm_object_t base_name = pvm_code_get_string(&h);

                // TODO turn on later, when we're sure all class collections have it
                //pvm_object_t version_string = pvm_code_get_string(&h);

                got_class_header = 1;
#if 0
#warning base class ignored
#else

                if( EQ_STRING_P2C(base_name,".internal.object") )
                    base_class = pvm_get_null_class();
                else
                {
                    base_class = pvm_exec_lookup_class_by_name(base_name);
                    if( pvm_is_internal_class(base_class) )
                    {
                        base_class = pvm_get_null_class();
                        printf("Class ");
                        pvm_object_print( class_name );
                        printf(" attempted to extend internal class. Child of void now.\n");
                    }
                }
#endif
                if(debug_print)
                {
                    printf("Class ");
                    pvm_object_print( class_name );
                    printf(" based on: " );
                    pvm_object_print( base_name );
                    printf(" @%p\n", base_class);
                }

                ref_dec_o(base_name);

                iface = pvm_create_interface_object( n_method_slots, base_class );

                ip2line_maps = pvm_create_object( pvm_get_array_class() );
                method_names = pvm_create_object( pvm_get_array_class() );
                field_names = pvm_create_object( pvm_get_array_class() );
                const_pool = pvm_create_array_object();

            }
            break;

        case 'M': // method
            {
                struct method_loader_handler mh;
                pvm_load_method( &mh, ptr, record_data_size );
                pvm_set_ofield( iface, mh.my_ordinal, mh.my_code );
                pvm_set_ofield( method_names, mh.my_ordinal, mh.my_name );
            }
            break;

        case 'l': // IP to line num map
            {

                if(debug_print) printf(" line num map\n");
                int ordinal = pvm_code_get_int32(&h);
                int mapsize = pvm_code_get_int32(&h);


                pvm_object_t map = pvm_create_binary_object(mapsize*(sizeof(struct vm_code_linenum)), 0);

                struct data_area_4_binary *bin= pvm_object_da( map, binary );

                struct vm_code_linenum *sp = (void *)bin->data;

                //qsort( bin->data, mapsize, sizeof(struct vm_code_linenum), vm_code_linenum_cmp );

                if(0 && debug_print)
                {
                    int i;
                    for( i = 0 ; i < mapsize; i++, sp++ )
                    {
                        sp->ip = pvm_code_get_int32(&h);
                        sp->line = pvm_code_get_int32(&h);

                        printf("map l %d -> ip %ld\n", sp->line, sp->ip );
                    }

                    printf("! "); pvm_object_print( map ); printf(" !\n");
                }

                pvm_set_ofield( ip2line_maps, ordinal, map );
            }
            break;

        case 'S': // method signature
            {
                if(debug_print) printf("meth sig\n" );

                pvm_object_t m_name = pvm_code_get_string(&h);
                int m_ordinal = pvm_code_get_int32(&h);
                int m_n_args = pvm_code_get_int32(&h);
                int is_ctor = pvm_code_get_int32(&h); // 1 = method is constructor

                struct type_loader_handler mth;
                pvm_load_type( &h , &mth );

                if(debug_print)
                {
                    printf("Method ");
                    pvm_dump_type( &mth );
                    printf(" '");
                    pvm_object_print( m_name );
                    printf("' ord %d args %d ( ", m_ordinal, m_n_args );
                }

                int i = m_n_args;
                while(i-- > 0)
                {
                    pvm_object_t a_name = pvm_code_get_string(&h);

                    struct type_loader_handler th;
                    pvm_load_type( &h , &th );


                    if(debug_print)
                    {
                        pvm_object_print( a_name );
                        printf(" : " );
                        pvm_dump_type( &th );
                        if(i > 0 )
                            printf(", ");
                    }

                }

                if(debug_print) printf(" )\n");

            }
            break;

        case 'c': // constant for const pool
            {
                int c_ordinal = pvm_code_get_int32(&h); // const pool position (id)

                struct type_loader_handler th;
                pvm_load_type( &h , &th );

                pvm_object_t c_value = { 0, 0 };

                // No const containers (yet?)
                if( th.is_container ) goto unk_const;

                if( (!th.is_container) && EQ_STRING_P2C(th.class_name,".internal.string") )
                {
                    c_value = pvm_create_string_object_binary( (void *)(h.code+h.IP), h.IP_max-h.IP);
                }

            unk_const:
                if( c_value )
                {
                    // had bug in pool read - no ref inc, fixed, turned daturate off
                    //ref_saturate_o(c_value);
                    // but, frankly, maybe saturate is ok here?
                    pvm_set_ofield( const_pool, c_ordinal, c_value );
                }
                else
                    //if(debug_print)
                {
                    printf("unknown const (id %d) type: ", c_ordinal );
                    pvm_dump_type( &th );
                    printf("\n" );
                }
            }
            break;

        case 'f': // field names
            {
                if(debug_print) printf("fields\n" );

                while( h.IP < h.IP_max )
                {
                    pvm_object_t f_name = pvm_code_get_string(&h);
                    int f_ordinal = pvm_code_get_int32(&h);

                    struct type_loader_handler th;
                    pvm_load_type( &h , &th );

                    if(debug_print)
                    {
                        printf("Field '");
                        pvm_object_print( f_name );

                        printf("' ord %d type: ", f_ordinal );

                        pvm_dump_type( &th );
                        printf("\n");
                    }

                    pvm_set_ofield( field_names, f_ordinal, f_name );

                }

            }
            break;

        default:
            {
                printf("Class record '%c' ignored\n", record_type );
            }
        }

    }

    if( !got_class_header )
        return 1;


    pvm_object_t new_class = pvm_create_class_object(class_name, iface, sizeof(pvm_object_t) * n_object_slots);

    struct data_area_4_class *cda= pvm_object_da( new_class, class );
    cda->ip2line_maps = ip2line_maps;
    cda->method_names = method_names;
    cda->field_names = field_names;
    cda->const_pool = const_pool; //ref_inc_o(const_pool);
    cda->class_parent = base_class;

    if(debug_print)
    {
        printf("\nDone loading "); 
        pvm_object_print( class_name );
        printf(" @%p\n", new_class); 
    }

    *out = new_class;
    return 0;
}



static int vm_code_linenum_cmp(const void *_a, const void *_b)
{
    const struct vm_code_linenum *a = _a;
    const struct vm_code_linenum *b = _b;
    return (a->ip == b->ip) ? 0 : ( (a->ip > b->ip) ? 1 : 0 );
}








