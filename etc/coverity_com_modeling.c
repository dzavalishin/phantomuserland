void pvm_exec_panic( const char *reason )
{
	__coverity_panic__();
}

void test_fail(int rc)
{
	__coverity_panic__();
}


void test_fail_msg(int rc, const char *msg)
{
	__coverity_panic__();
}
