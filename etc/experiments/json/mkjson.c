#include <stdio.h>

static inline char hex_nibble( char c ) { return (c >= 10) ? 'A' + c - 10 : '0' + c; }

void out( char c )
{
    putchar( c );
}


// TODO UTF-8

void jenc(char *in)
{

    while( *in )
    {
        const char c = *in++;

        if(c == 0)
            break;

        switch (c)
        {

        case '\\':
        case '"':
            out( '\\' );
            out( c );
            break;

        case '/':
            out( '\\' );
            out( c );
            break;

        case '\b':
            out( '\\' );
            out( 'b' );
            break;

        case '\t':
            out( '\\' );
            out( 't' );
            break;

        case '\n':
            out( '\\' );
            out( 'n' );
            break;

        case '\f':
            out( '\\' );
            out( 'f' );
            break;

        case '\r':
            out( '\\' );
            out( 'r' );
            break;

        default:
            if (c < ' ')
            {
                out( '\\' );
                out( 'u' );
                hex_nibble( 0x0F & (c>>4) );
                hex_nibble( 0x0F & c );

            }
            else
            {
                out( c );
            }
        }
    }

}


int main( int ac, char **av )
{
    jenc("hello \"world\"");

    return 0;
}

