class Test
{

int testCode(int b) 
{
    if(b > 2)
	return b+343;
    else
	return 22;
}


Test testClass()
{
	Test t = new Test();
	int a = t.testCode(2);

	return t;
}


}