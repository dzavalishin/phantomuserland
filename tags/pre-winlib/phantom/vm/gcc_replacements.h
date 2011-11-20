// hack - local bare bones header to live without compiler's includes

struct _FILE;

typedef struct _FILE FILE;


#ifndef SEEK_SET
#define	SEEK_SET	0	/* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define	SEEK_END	2	/* set file offset to EOF plus offset */
#endif


#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int		size_t;
typedef int			ssize_t;
#endif //_SIZE_T


//void *malloc( int size );
void *malloc(size_t size);

void free( void *mem );

FILE * fopen( const char *fn, const char *mode );
int fread( void *ptr, int, int, FILE *f );
int fwrite( const void *ptr, int, int, FILE *f );
int fclose( FILE *f );

int fseek( FILE *f, long offset, int whence );
long ftell( FILE *f );


void exit(int retcode );
