#include <stdio.h>

// made to reverse Kolibri fonts right to left :)

static unsigned char font_reverse_c(unsigned char ic)
{
    unsigned char oc = 0;

    int bitc = 8;
    while(bitc--)
    {
        oc <<= 1;
        unsigned char bit = ic & 1;
        if(bit)             oc |= 1;

        ic >>= 1;
    }
    return oc;
}


int main()
{

    while(1)
    {
        unsigned char ic;
        if( read( 0, &ic, 1 ) < 1 )
            break;
        unsigned char oc = font_reverse_c(ic);
        write( 1, &oc, 1 );
    }

}
