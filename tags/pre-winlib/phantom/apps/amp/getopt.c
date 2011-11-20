#ifndef _NO_PROTO
#define _NO_PROTO
#endif

#include <stdio.h>
#include <compat/tabos.h>

#ifndef _
/* This is for other GNU distributions with internationalized messages.
 When compiling libc, the _ macro is predefined.  */
#ifdef HAVE_LIBINTL_H
# include <libintl.h>
# define _(msgid)	gettext (msgid)
#else
# define _(msgid)	(msgid)
#endif
#endif

#include "getopt.h"

char *optarg = NULL;
int optind = 1;
int __getopt_initialized = 0;
static char *nextchar;
int opterr = 1;
int optopt = '?';
static enum {
    REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
} ordering;
static char *posixly_correct;

static char *my_index (str, chr)
    const char *str;
int chr;
{
    while (*str)
    {
        if (*str == chr)
            return (char *) str;
        str++;
    }
    return 0;
}

static int first_nonopt;
static int last_nonopt;

static void
    exchange (argv)
    char **argv;
{
    int bottom = first_nonopt;
    int middle = last_nonopt;
    int top = optind;
    char *tem;

    /* Exchange the shorter segment with the far end of the longer segment.
     That puts the shorter segment into the right place.
     It leaves the longer segment in the right place overall,
     but it consists of two parts that need to be swapped next.  */

    while (top > middle && middle > bottom)
    {
        if (top - middle > middle - bottom)
        {
            /* Bottom segment is the short one.  */
            int len = middle - bottom;
            register int i;

            /* Swap it with the top part of the top segment.  */
            for (i = 0; i < len; i++)
            {
                tem = argv[bottom + i];
                argv[bottom + i] = argv[top - (middle - bottom) + i];
                argv[top - (middle - bottom) + i] = tem;
            }
            /* Exclude the moved bottom segment from further swapping.  */
            top -= len;
        }
        else
        {
            /* Top segment is the short one.  */
            int len = top - middle;
            register int i;

            /* Swap it with the bottom part of the bottom segment.  */
            for (i = 0; i < len; i++)
            {
                tem = argv[bottom + i];
                argv[bottom + i] = argv[middle + i];
                argv[middle + i] = tem;
            }
            /* Exclude the moved top segment from further swapping.  */
            bottom += len;
        }
    }

    /* Update records for the slots the non-options now occupy.  */

    first_nonopt += (optind - last_nonopt);
    last_nonopt = optind;
}

static const char *
_getopt_initialize (argc, argv, optstring)
int argc;
char *const *argv;
const char *optstring;
{
    /* Start processing options with ARGV-element 1 (since ARGV-element 0
     is the program name); the sequence of previously skipped
     non-option ARGV-elements is empty.  */

    first_nonopt = last_nonopt = optind = 1;

    nextchar = NULL;

    posixly_correct = NULL; //getenv ("POSIXLY_CORRECT");

    /* Determine how to handle the ordering of options and nonoptions.  */

    if (optstring[0] == '-')
    {
        ordering = RETURN_IN_ORDER;
        ++optstring;
    }
    else if (optstring[0] == '+')
    {
        ordering = REQUIRE_ORDER;
        ++optstring;
    }
    else if (posixly_correct != NULL)
        ordering = REQUIRE_ORDER;
    else
        ordering = PERMUTE;

    return optstring;
}

int
_getopt_internal (argc, argv, optstring, longopts, longind, long_only)
int argc;
char *const *argv;
const char *optstring;
const struct option *longopts;
int *longind;
int long_only;
{
    optarg = NULL;

    if (!__getopt_initialized || optind == 0)
    {
        optstring = _getopt_initialize (argc, argv, optstring);
        optind = 1;		/* Don't scan ARGV[0], the program name.  */
        __getopt_initialized = 1;
    }

    /* Test whether ARGV[optind] points to a non-option argument.
     Either it does not have option syntax, or there is an environment flag
     from the shell indicating it is not an option.  The later information
     is only used when the used in the GNU libc.  */
#define NONOPTION_P (argv[optind][0] != '-' || argv[optind][1] == '\0')

    if (nextchar == NULL || *nextchar == '\0')
    {
        /* Advance to the next ARGV-element.  */

        /* Give FIRST_NONOPT & LAST_NONOPT rational values if OPTIND has been
         moved back by the user (who may also have changed the arguments).  */
        if (last_nonopt > optind)
            last_nonopt = optind;
        if (first_nonopt > optind)
            first_nonopt = optind;

        if (ordering == PERMUTE)
        {
            /* If we have just processed some options following some non-options,
             exchange them so that the options come first.  */

            if (first_nonopt != last_nonopt && last_nonopt != optind)
                exchange ((char **) argv);
            else if (last_nonopt != optind)
                first_nonopt = optind;

            /* Skip any additional non-options
             and extend the range of non-options previously skipped.  */

            while (optind < argc && NONOPTION_P)
                optind++;
            last_nonopt = optind;
        }

        /* The special ARGV-element `--' means premature end of options.
         Skip it like a null option,
         then exchange with previous non-options as if it were an option,
         then skip everything else like a non-option.  */

        if (optind != argc && !StringCompare (argv[optind], "--"))
        {
            optind++;

            if (first_nonopt != last_nonopt && last_nonopt != optind)
                exchange ((char **) argv);
            else if (first_nonopt == last_nonopt)
                first_nonopt = optind;
            last_nonopt = argc;

            optind = argc;
        }

        /* If we have done all the ARGV-elements, stop the scan
         and back over any non-options that we skipped and permuted.  */

        if ( optind == argc ) {
            /* Set the next-arg-index to point at the non-options
             * that we previously skipped, so the caller will digest them.  */
            if (first_nonopt != last_nonopt) {
                optind = first_nonopt;
            }
            return -1;
        }

        /* If we have come to a non-option and did not permute it,
         either stop the scan or describe it to the caller and pass it by.  */

        if (NONOPTION_P)
        {
            if (ordering == REQUIRE_ORDER)
                return -1;
            optarg = argv[optind++];
            return 1;
        }

        /* We have found another option-ARGV-element.
         Skip the initial punctuation.  */

        nextchar = (argv[optind] + 1
                    + (longopts != NULL && argv[optind][1] == '-'));
    }

    /* Decode the current option-ARGV-element.  */

    /* Check whether the ARGV-element is a long option.

    If long_only and the ARGV-element has the form "-f", where f is
    a valid short option, don't consider it an abbreviated form of
    a long option that starts with f.  Otherwise there would be no
    way to give the -f short option.

    On the other hand, if there's a long option "fubar" and
    the ARGV-element is "-fu", do consider that an abbreviation of
    the long option, just like "--fu", and not "-f" with arg "u".

    This distinction seems to be the most useful approach.  */

    if (longopts != NULL
        && (argv[optind][1] == '-'
            || (long_only && (argv[optind][2] || !my_index (optstring, argv[optind][1])))))
    {
        char *nameend;
        const struct option *p;
        const struct option *pfound = NULL;
        int exact = 0;
        int ambig = 0;
        int indfound = -1;
        int option_index;

        for (nameend = nextchar; *nameend && *nameend != '='; nameend++)
            /* Do nothing.  */ ;

        /* Test all long options for either exact match
         or abbreviated matches.  */
        for (p = longopts, option_index = 0; p->name; p++, option_index++)
            if (!StringNumCompare (p->name, nextchar, nameend - nextchar))
            {
                if ((unsigned int) (nameend - nextchar)
                    == (unsigned int) StringLength (p->name))
                {
                    /* Exact match found.  */
                    pfound = p;
                    indfound = option_index;
                    exact = 1;
                    break;
                }
                else if (pfound == NULL)
                {
                    /* First nonexact match found.  */
                    pfound = p;
                    indfound = option_index;
                }
                else
                    /* Second or later nonexact match found.  */
                    ambig = 1;
            }

        if (ambig && !exact)
        {
            if (opterr)
                printf( _("%s: option `%s' is ambiguous\n"),
                        argv[0], argv[optind]);
            nextchar += StringLength (nextchar);
            optind++;
            optopt = 0;
            return '?';
        }

        if (pfound != NULL)
        {
            option_index = indfound;
            optind++;
            if (*nameend)
            {
                /* Don't test has_arg with >, because some C compilers don't
                 allow it to be used on enums.  */
                if (pfound->has_arg)
                    optarg = nameend + 1;
                else
                {
                    if (opterr)
                    {
                        if (argv[optind - 1][1] == '-')
                            /* --option */
                            printf (
                                    _("%s: option `--%s' doesn't allow an argument\n"),
                                    argv[0], pfound->name);
                        else
                            /* +option or -option */
                            printf (
                                    _("%s: option `%c%s' doesn't allow an argument\n"),
                                    argv[0], argv[optind - 1][0], pfound->name);
                    }
                    nextchar += StringLength (nextchar);

                    optopt = pfound->val;
                    return '?';
                }
            }
            else if (pfound->has_arg == 1)
            {
                if (optind < argc)
                    optarg = argv[optind++];
                else
                {
                    if (opterr)
                        printf (
                                _("%s: option `%s' requires an argument\n"),
                                argv[0], argv[optind - 1]);
                    nextchar += StringLength (nextchar);
                    optopt = pfound->val;
                    return optstring[0] == ':' ? ':' : '?';
                }
            }
            nextchar += StringLength (nextchar);
            if (longind != NULL)
                *longind = option_index;
            if (pfound->flag)
            {
                *(pfound->flag) = pfound->val;
                return 0;
            }
            return pfound->val;
        }

        /* Can't find it as a long option.  If this is not getopt_long_only,
         or the option starts with '--' or is not a valid short
         option, then it's an error.
         Otherwise interpret it as a short option.  */
        if (!long_only || argv[optind][1] == '-'
            || my_index (optstring, *nextchar) == NULL)
        {
            if (opterr)
            {
                if (argv[optind][1] == '-')
                    /* --option */
                    printf( _("%s: unrecognized option `--%s'\n"),
                            argv[0], nextchar);
                else
                    /* +option or -option */
                    printf( _("%s: unrecognized option `%c%s'\n"),
                            argv[0], argv[optind][0], nextchar);
            }
            nextchar = (char *) "";
            optind++;
            optopt = 0;
            return '?';
        }
    }

    /* Look at and handle the next short option-character.  */

    {
        char c = *nextchar++;
        char *temp = my_index (optstring, c);

        /* Increment `optind' when we start to process its last character.  */
        if (*nextchar == '\0')
            ++optind;

        if (temp == NULL || c == ':')
        {
            if (opterr)
            {
                if (posixly_correct)
                    /* 1003.2 specifies the format of this message.  */
                    printf( _("%s: illegal option -- %c\n"),
                            argv[0], c);
                else
                    printf( _("%s: invalid option -- %c\n"),
                            argv[0], c);
            }
            optopt = c;
            return '?';
        }
        /* Convenience. Treat POSIX -W foo same as long option --foo */
        if (temp[0] == 'W' && temp[1] == ';')
        {
            char *nameend;
            const struct option *p;
            const struct option *pfound = NULL;
            int exact = 0;
            int ambig = 0;
            int indfound = 0;
            int option_index;

            /* This is an option that requires an argument.  */
            if (*nextchar != '\0')
            {
                optarg = nextchar;
                /* If we end this ARGV-element by taking the rest as an arg,
                 we must advance to the next element now.  */
                optind++;
            }
            else if (optind == argc)
            {
                if (opterr)
                {
                    /* 1003.2 specifies the format of this message.  */
                    printf( _("%s: option requires an argument -- %c\n"),
                            argv[0], c);
                }
                optopt = c;
                if (optstring[0] == ':')
                    c = ':';
                else
                    c = '?';
                return c;
            }
            else
                /* We already incremented `optind' once;
                 increment it again when taking next ARGV-elt as argument.  */
                optarg = argv[optind++];

            /* optarg is now the argument, see if it's in the
             table of longopts.  */

            for (nextchar = nameend = optarg; *nameend && *nameend != '='; nameend++)
                /* Do nothing.  */ ;

            /* Test all long options for either exact match
             or abbreviated matches.  */
            for (p = longopts, option_index = 0; p->name; p++, option_index++)
                if (!StringNumCompare (p->name, nextchar, nameend - nextchar))
                {
                    if ((unsigned int) (nameend - nextchar) == StringLength (p->name))
                    {
                        /* Exact match found.  */
                        pfound = p;
                        indfound = option_index;
                        exact = 1;
                        break;
                    }
                    else if (pfound == NULL)
                    {
                        /* First nonexact match found.  */
                        pfound = p;
                        indfound = option_index;
                    }
                    else
                        /* Second or later nonexact match found.  */
                        ambig = 1;
                }
            if (ambig && !exact)
            {
                if (opterr)
                    printf( _("%s: option `-W %s' is ambiguous\n"),
                            argv[0], argv[optind]);
                nextchar += StringLength (nextchar);
                optind++;
                return '?';
            }
            if (pfound != NULL)
            {
                option_index = indfound;
                if (*nameend)
                {
                    /* Don't test has_arg with >, because some C compilers don't
                     allow it to be used on enums.  */
                    if (pfound->has_arg)
                        optarg = nameend + 1;
                    else
                    {
                        if (opterr)
                            printf( _("\
                                      %s: option `-W %s' doesn't allow an argument\n"),
                                    argv[0], pfound->name);

                        nextchar += StringLength (nextchar);
                        return '?';
                    }
                }
                else if (pfound->has_arg == 1)
                {
                    if (optind < argc)
                        optarg = argv[optind++];
                    else
                    {
                        if (opterr)
                            printf(
                                   _("%s: option `%s' requires an argument\n"),
                                   argv[0], argv[optind - 1]);
                        nextchar += StringLength (nextchar);
                        return optstring[0] == ':' ? ':' : '?';
                    }
                }
                nextchar += StringLength (nextchar);
                if (longind != NULL)
                    *longind = option_index;
                if (pfound->flag)
                {
                    *(pfound->flag) = pfound->val;
                    return 0;
                }
                return pfound->val;
            }
            nextchar = NULL;
            return 'W';	/* Let the application handle it.   */
        }
        if (temp[1] == ':')
        {
            if (temp[2] == ':')
            {
                /* This is an option that accepts an argument optionally.  */
                if (*nextchar != '\0')
                {
                    optarg = nextchar;
                    optind++;
                }
                else
                    optarg = NULL;
                nextchar = NULL;
            }
            else
            {
                /* This is an option that requires an argument.  */
                if (*nextchar != '\0')
                {
                    optarg = nextchar;
                    /* If we end this ARGV-element by taking the rest as an arg,
                     we must advance to the next element now.  */
                    optind++;
                }
                else if (optind == argc)
                {
                    if (opterr)
                    {
                        /* 1003.2 specifies the format of this message.  */
                        printf(
                               _("%s: option requires an argument -- %c\n"),
                               argv[0], c);
                    }
                    optopt = c;
                    if (optstring[0] == ':')
                        c = ':';
                    else
                        c = '?';
                }
                else
                    /* We already incremented `optind' once;
                     increment it again when taking next ARGV-elt as argument.  */
                    optarg = argv[optind++];
                nextchar = NULL;
            }
        }
        return c;
    }
}

int
getopt (argc, argv, optstring)
int argc;
char *const *argv;
const char *optstring;
{
    return _getopt_internal (argc, argv, optstring,
                             (const struct option *) 0,
                             (int *) 0,
                             0);
}

#ifdef TEST

/* Compile with -DTEST to make an executable for use in testing
 the above definition of `getopt'.  */

int
main (argc, argv)
int argc;
char **argv;
{
    int c;
    int digit_optind = 0;

    while (1)
    {
        int this_option_optind = optind ? optind : 1;

        c = getopt (argc, argv, "abc:d:0123456789");
        if (c == -1)
            break;

        switch (c)
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (digit_optind != 0 && digit_optind != this_option_optind)
                Print ("digits occur in two different argv-elements.\n");
            digit_optind = this_option_optind;
            Print ("option %c\n", c);
            break;

        case 'a':
            Print ("option a\n");
            break;

        case 'b':
            Print ("option b\n");
            break;

        case 'c':
            Print ("option c with value `%s'\n", optarg);
            break;

        case '?':
            break;

        default:
            Print ("?? getopt returned character code 0%o ??\n", c);
        }
    }

    if (optind < argc)
    {
        Print ("non-option ARGV-elements: ");
        while (optind < argc)
            Print ("%s ", argv[optind++]);
        Print ("\n");
    }

    ThreadExit (0);
}

#endif /* TEST */
