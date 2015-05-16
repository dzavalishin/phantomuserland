# Introduction #

Syscall is a method of internal class. It is called as usual method (at least all the conversion is hidden).

Syscall is supposed to return one object or return nothing. Syscall can throw an exception.

Syscall is supposed to decrement refcount for its object arguments or save/pass them along.

Q: what about 'this'?

Call parameters: 'this' object, current thread object da pointer.

# Helpers #

  * SYSCALL\_RETURN(obj) - return object
  * SYSCALL\_RETURN\_NOTHING - just return (null object will be returned, in fact)
  * SYSCALL\_THROW(obj) - return by throwing an exception
  * SYSCALL\_THROW\_STRING(str) - shortcut, accepts C string as arg
  * POP\_ARG - returns next arg. pop in reverse order!
  * POP\_ISTACK - istack has just one arg - number of objects passed
  * POP\_INT() - pops object and converts it to C integer, with type check
  * POP\_STRING() - pops object and converts it to string da ptr, with type check
  * CHECK\_PARAM\_COUNT(n\_param, must\_have) - throws a message if !=
  * ASSERT\_STRING(obj) - throws, if not a string object
  * ASSERT\_INT(obj) - throws, if not an int object
  * DEBUG\_INFO - in debug mode prints call info

# Can change #

  * SYSCALL\_PUT\_THIS\_THREAD\_ASLEEP() - after returning from syscall current thread will be blocked
  * SYSCALL\_PUT\_THREAD\_ASLEEP(thread) - thread will be put asleep AFTER SYSCALL ONLY! In fact this one can't be used!
  * SYSCALL\_WAKE\_THIS\_THREAD\_UP() - impossible to call :)
  * SYSCALL\_WAKE\_THREAD\_UP(thread) - given thread will be unblocked NOW.

# Example #

```
static int si_string_8_substring(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_string *meda = pvm_object_da( me, string );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 2);

    struct pvm_object _len = POP_ARG;
    ASSERT_INT(_len);
    int parmlen = pvm_get_int(_len);

    struct pvm_object _index = POP_ARG;
    ASSERT_INT(_index);
    int index = pvm_get_int(_index);


    if( index < 0 || index >= meda->length )
        SYSCALL_THROW_STRING( "string.substring index is out of bounds" );

    int len = meda->length - index;
    if( parmlen < len ) len = parmlen;

    if( len < 0 )
        SYSCALL_THROW_STRING( "string.substring length is negative" );

    SYSCALL_RETURN(pvm_create_string_object_binary( meda->data + index, len ));
}

```