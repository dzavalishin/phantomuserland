#include <stdio.h>
#include "getopt.h"

int getopt_long( int argc, char *const *argv, const char *options,
    const struct option *long_options, int *opt_index ) {
    return _getopt_internal( argc, argv, options, long_options, opt_index, 0 );
}

int getopt_long_only( int argc, char *const *argv, const char *options,
    const struct option *long_options, int *opt_index ) {
    return _getopt_internal( argc, argv, options, long_options, opt_index, 1 );
}

#ifdef TEST

#include <stdio.h>

int main( int argc, char **argv ) {
    int c;
    int digit_optind = 0;

    while ( 1 ) {
	int this_option_optind = optind ? optind : 1;
	int option_index = 0;
	static struct option long_options[] = {
	    { "add", 1, 0, 0 },
	    { "append", 0, 0, 0 },
	    { "delete", 1, 0, 0 },
	    { "verbose", 0, 0, 0 },
	    { "create", 0, 0, 0 },
	    { "file", 1, 0, 0 },
	    { 0, 0, 0, 0 }
        };

	c = getopt_long( argc, argv, "abc:d:0123456789", long_options,
	    &option_index);
	if ( c == -1 ) {
	    break;
	}

      switch (c)
	{
	case 0:
	  Print ("option %s", long_options[option_index].name);
	  if (optarg)
	    Print (" with arg %s", optarg);
	  Print ("\n");
	  break;

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

	case 'd':
	  Print ("option d with value `%s'\n", optarg);
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

  exit (0);
}

#endif /* TEST */
