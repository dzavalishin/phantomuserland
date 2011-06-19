// Simple grep.  Only supports ^ . * $ operators.

#include <user/sys_misc.h>
#include <user/sys_fio.h>
#include <stdio.h>


static int list( int fd );
static int get( int fd, const char *key );
static int set( int fd, const char *key, const char *val );

int
main(int argc, char *argv[])
{
  int fd;
  
  if(argc <= 1 || argc > 4)
  {
    printf( "usage: mode file          \t- list keys\n");
    printf( "usage: mode file key      \t- print mode for key\n");
    printf( "usage: mode file key value\t- set mode for key to value\n");
    exit(0);
  }

  if((fd = open(argv[1], 0)) < 0)
  {
      printf( "mode: cannot open %s\n", argv[1]);
      exit(0);
  }

  if(argc == 2)
      return list(fd);

  if(argc == 3)
      return get(fd, argv[2]);

  if(argc == 4)
      return set(fd, argv[2], argv[3]);

  close(fd);
  return 0;
}



static int list( int fd )
{
    char buf[1024];

    int i;
    for( i = 0; ; i++ )
    {
        if( listproperties( fd, i, buf, sizeof(buf) ) )
            return 0;
        printf("%s\n", buf);
    }
}

static int get( int fd, const char *key )
{
    char buf[1024];

    errno_t rc = getproperty( fd, key, buf, sizeof(buf) );

    if( rc == 0 )
    {
        printf("%s\n", buf );
        return 0;
    }

    printf("error %d\n", rc);
    return 1;

}

static int set( int fd, const char *key, const char *val )
{
    errno_t rc = setproperty( fd, key, val );
    if( rc == 0 )
    {
        return 0;
    }

    printf("error %d\n", rc);
    return 1;
}




