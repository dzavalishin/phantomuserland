extern int main(int ac, char **av, char **env);
void __start()
{
	main( 0, 0, 0 );
}

// automatically called by GCC from main in SOME (!) cases
void __main()
{
}
