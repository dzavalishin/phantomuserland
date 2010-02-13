extern void exit(int);
extern void sleepmsec(int);
extern void vsyslog(int level, char *string );

int
main(int ac, char **av, char **env)
{
	while(1)
	{
		vsyslog( 0, "module test is running" );
		sleepmsec(4000);
	}

	exit(0);
    asm("int $3");
}