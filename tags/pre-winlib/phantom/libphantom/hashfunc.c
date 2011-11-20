
#include <hashfunc.h>

/*--- HashPJW ---------------------------------------------------
 *  An adaptation of Peter Weinberger's (PJW) generic hashing
 *  algorithm based on Allen Holub's version. Accepts a pointer
 *  to a datum to be hashed and returns an unsigned integer.
 *     Modified by sandro to include datum_end
 *     Taken from http://www.ddj.com/articles/1996/9604/9604b/9604b.htm?topic=algorithms
 *-------------------------------------------------------------*/
#include <limits.h>
#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))

static void update_hash(unsigned int *hash_value, const char *datum)
{
    unsigned int i;

    *hash_value = ( *hash_value << ONE_EIGHTH ) + *datum;

    if (( i = *hash_value & HIGH_BITS ) != 0 )
        *hash_value =
            ( *hash_value ^ ( i >> THREE_QUARTERS )) &
            ~HIGH_BITS;
}


unsigned int calc_hash(const char *datum, const char *datum_end)
{
    unsigned int hash_value;

    if (datum_end) {
        for( hash_value = 0; datum < datum_end; ++datum )
        {
            update_hash( &hash_value, datum );
            /*
            hash_value = ( hash_value << ONE_EIGHTH ) + *datum;
            if (( i = hash_value & HIGH_BITS ) != 0 )
                hash_value =
                    ( hash_value ^ ( i >> THREE_QUARTERS )) &
                    ~HIGH_BITS;
            */
        }
    } else {
        for ( hash_value = 0; *datum; ++datum )
        {
            update_hash( &hash_value, datum );
            /*
            hash_value = ( hash_value << ONE_EIGHTH ) + *datum;
            if (( i = hash_value & HIGH_BITS ) != 0 )
                hash_value =
                    ( hash_value ^ ( i >> THREE_QUARTERS )) &
                    ~HIGH_BITS;
            */
        }
        /* and the extra null value, so we match if working by length */
        update_hash( &hash_value, datum );
        /*
        hash_value = ( hash_value << ONE_EIGHTH ) + *datum;
        if (( i = hash_value & HIGH_BITS ) != 0 )
            hash_value =
                ( hash_value ^ ( i >> THREE_QUARTERS )) &
                ~HIGH_BITS;
        */
    }

    /* printf("Hash value of %s//%s is %d\n", datum, datum_end, hash_value); */
    return hash_value;
}

