void phantom_switch_context(
                            void *from,
                            void *to,
                            int *unlock );


void test()
{

phantom_switch_context(
                            (void *)0,
                            (void *)1,
                            (int *)2);

}