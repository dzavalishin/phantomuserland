/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/* Code modified by Justin Smith 2003/06/26 */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>

#if 0
static unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (!base) {
		base = 10;
		if (*cp == '0') {
			base = 8;
			cp++;
			if ((*cp == 'x') && isxdigit(cp[1])) {
				cp++;
				base = 16;
			}
		}
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

static long simple_strtol(const char *cp,char **endp,unsigned int base)
{
	if(*cp=='-')
		return -simple_strtoul(cp+1,endp,base);
	return simple_strtoul(cp,endp,base);
}
#endif
static int skip_atoi(const char **s)
{
	int i=0;

	while (isdigit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define LARGE	64		/* use 'ABCDEF' instead of 'abcdef' */

static unsigned long long do_div(unsigned long long *n, unsigned base) 
{ 
	unsigned long long res;
	res = (*n) %  (unsigned long long)base;

	*n = (*n) / (unsigned long long)base;

	return res;
}

static char * number(char * str, long long num, unsigned base, int size, int precision
	,int type)
{
	char c,sign,tmp[66];
	const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
	int i;

	if (type & LARGE)
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN) {
		if (num < 0) {
			sign = '-';
			num = -num;
			size--;
		} else if (type & PLUS) {
			sign = '+';
			size--;
		} else if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}
	if (type & SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}
	i = 0;
	if (num == 0)
		tmp[i++]='0';
	else while (num != 0)
		tmp[i++] = digits[do_div((unsigned long long *)&num,base)];
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type&(ZEROPAD+LEFT)))
		while(size-->0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (type & SPECIAL) {
		if (base==8)
			*str++ = '0';
		else if (base==16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	}
	if (!(type & LEFT))
		while (size-- > 0)
			*str++ = c;
	while (i < precision--)
		*str++ = '0';
	while (i-- > 0)
		*str++ = tmp[i];
	while (size-- > 0)
		*str++ = ' ';
	return str;
}

/** %e, %f, %g are not implemented */
int _v_printf(int (*_write)(void*, const void *, ssize_t ), void* arg, const char *fmt, va_list args)
{
    int len;
	unsigned long long num;
	int i, base;

    char* str;
    const char *s;

	int flags;		/* flags to number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
				           number of chars for from string */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */

    int err;
    off_t out_len = 0;
    int pos = 0;
    char buf[ 1024 ];

#define WRITE(C)    buf[pos++] = (C); \
                    if(pos == 1024 ) { \
                        err = _write(arg, buf, 1024 );\
                        if(err < 0) return err;\
                        out_len += pos; \
                        pos = 0; \
                    }
#define ALLOC_STR(MAX) (char *)malloc(((precision > field_width) ? ((precision > (MAX) ) ? precision : (MAX) ) : (field_width > (MAX) ) ? field_width : (MAX) )*sizeof(char));

    /* Loop through each character of the format*/
    for (; *fmt ; ++fmt) 
    {
        /* Copy over non-format chars */
		if (*fmt != '%') 
        {
			WRITE( *fmt )
            continue;
		}

		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */
			switch (*fmt) {
				case '-': flags |= LEFT; goto repeat;
				case '+': flags |= PLUS; goto repeat;
				case ' ': flags |= SPACE; goto repeat;
				case '#': flags |= SPECIAL; goto repeat;
				case '0': flags |= ZEROPAD; goto repeat;
				}
		
		/* get field width */
		field_width = -1;
		if (isdigit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			++fmt;
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;	
			if (isdigit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}

		/* default base */
		base = 10;

		switch (*fmt) {
		case 'c':
			if (!(flags & LEFT))
				while (--field_width > 0)
                {
					WRITE( ' ' )
                }
			WRITE( (unsigned char) va_arg(args, int) )
			while (--field_width > 0)
            {
                WRITE( ' ' )
            }
			continue;

		case 's':
			s = va_arg(args, char *);
			if (!s)
				s = "<NULL>";

			len = strnlen(s, precision);

			if (!(flags & LEFT))
				while (len < field_width--)
                {
                    WRITE( ' ' )
                }
			for (i = 0; i < len; ++i)
            {
                WRITE( *s++ )
            }
			while (len < field_width--)
            {
                WRITE( ' ' )
            }
			continue;

		case 'p':
			if (field_width == -1) {
				field_width = 2*sizeof(void *);
				flags |= ZEROPAD;
			}
			str = ALLOC_STR( 12 ); 
            str[0]= '0';
			str[1]= 'x';
			s = number(str+2,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
            len = s - str;
            s = str;
            for(i = 0; i < len; i++)
            {
				WRITE( *s++ )
            }
            free(str);
			continue;

/* From GNU documentation:
"The %n conversion is unlike any of the other output conversions. It uses an argument which must be a 
pointer to an int, but instead of printing anything it stores the number of characters printed so far 
by this call at that location. The h and l type modifiers are permitted to specify that the argument 
is of type short int * or long int * instead of int *, but no flags, field width, or precision are permitted."
http://www.gnu.org/manual/glibc-2.2.5/html_node/Other-Output-Conversions.html#Other%20Output%20Conversions
*/

		case 'n':
			if (qualifier == 'l') {
				long * ip = va_arg(args, long *);
				*ip = out_len + pos;
			} else {
				int * ip = va_arg(args, int *);
				*ip = out_len + pos;
			}
			continue;

		/* integer number formats - set up the flags and "break" */
		case 'o':
			base = 8;
			break;

		case 'X':
			flags |= LARGE;
		case 'x':
			base = 16;
			break;

		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			break;

		default:
			if (*fmt != '%')
            {
                WRITE( '%' )
            }
			if (*fmt)
            {
                WRITE( *fmt )
            }
			else
				--fmt;
			continue;
		}
		if (qualifier == 'L')
			num = va_arg(args, unsigned long long);
		else if (qualifier == 'l') {
			num = va_arg(args, unsigned long);
			if (flags & SIGN)
				num = (long) num;
		} else if (qualifier == 'h') {
			num = (unsigned short) va_arg(args, int);
			if (flags & SIGN)
				num = (short) num;
		} else if (flags & SIGN)
			num = va_arg(args, int);
		else
			num = va_arg(args, unsigned int);
        
        str = ALLOC_STR( 65 )
        s = number(str, num, base, field_width, precision, flags);
        len = s - str;
        s = str;
        for(i = 0; i < len; i++)
        {
            WRITE( *s++ )
        }
        free(str);
	}
    out_len += pos;
    err = _write(arg, buf, pos);
    if(err < 0) return err;
	return out_len;
#undef ALLOC_STR
#undef WRITE
}

