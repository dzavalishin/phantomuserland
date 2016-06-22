
void die_rc( const char *msg, int rc )
{
    printf("%s, rc = %d\n", msg, rc );
    exit( 0 );
}


void test(void)
{
}

int main( int ac; char**av )
{

    errno_t 		rc;

    rc = cpfs_init();
    if( rc ) die_rc( "Init", rc );


    test();

    rc = cpfs_stop(void);
    if( rc ) die_rc( "Stop", rc );


    return 0;
}

