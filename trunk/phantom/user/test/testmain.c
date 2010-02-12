extern void exit(int);

int
main()
{
	exit(0);
    asm("int $3");
}