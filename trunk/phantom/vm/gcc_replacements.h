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


void *malloc( int size );
void free( void *mem );


void exit(int retcode );
