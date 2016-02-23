#if CONF_USE_E4C
#define HAVE_SNPRINTF

/**
 *
 * @file        e4c.h
 *
 * exceptions4c header file
 *
 * @version     3.0
 * @author      Copyright (c) 2013 Guillermo Calvo
 *
 * @section e4c_h exceptions4c header file
 *
 * This header file needs to be included in order to be able to use any of the
 * exception handling system keywords:
 *
 *   - `#try`
 *   - `#catch`
 *   - `#finally`
 *   - `#throw`
 *   - `#with`
 *   - `#using`
 *
 * In order to stop defining these keywords, there exists a `E4C_NOKEYWORDS`
 * *compile-time* parameter. When the keywords are not defined, the next set of
 * alternative macros can be used to achieve the same functionality:
 *
 *   - `E4C_TRY`
 *   - `E4C_CATCH`
 *   - `E4C_FINALLY`
 *   - `E4C_THROW`
 *   - `E4C_WITH`
 *   - `E4C_USING`
 *
 * @section license License
 *
 * > This is free software: you can redistribute it and/or modify it under the
 * > terms of the **GNU Lesser General Public License** as published by the
 * > *Free Software Foundation*, either version 3 of the License, or (at your
 * > option) any later version.
 * >
 * > This software is distributed in the hope that it will be useful, but
 * > **WITHOUT ANY WARRANTY**; without even the implied warranty of
 * > **MERCHANTABILITY** or **FITNESS FOR A PARTICULAR PURPOSE**. See the
 * > [GNU Lesser General Public License](http://www.gnu.org/licenses/lgpl.html)
 * > for more details.
 * >
 * > You should have received a copy of the GNU Lesser General Public License
 * > along with this software. If not, see <http://www.gnu.org/licenses/>.
 *
 */


# ifndef EXCEPTIONS4C
# define EXCEPTIONS4C


# define E4C_VERSION_(version)			version(3, 0, 5)


# if !defined(E4C_THREADSAFE) && ( \
		defined(_THREAD_SAFE) \
	||	defined(_REENTRANT) \
	||	defined(PTHREAD_H) \
	||	defined(PTHREAD_BARRIER_SERIAL_THREAD) \
	||	defined(PTHREAD_CANCEL_ASYNCHRONOUS) \
	||	defined(PTHREAD_CANCEL_ENABLE) \
	||	defined(PTHREAD_CANCEL_DEFERRED) \
	||	defined(PTHREAD_CANCEL_DISABLE) \
	||	defined(PTHREAD_CANCELED) \
	||	defined(PTHREAD_COND_INITIALIZER) \
	||	defined(PTHREAD_CREATE_DETACHED) \
	||	defined(PTHREAD_CREATE_JOINABLE) \
	||	defined(PTHREAD_EXPLICIT_SCHED) \
	||	defined(PTHREAD_INHERIT_SCHED) \
	||	defined(PTHREAD_MUTEX_DEFAULT) \
	||	defined(PTHREAD_MUTEX_ERRORCHECK) \
	||	defined(PTHREAD_MUTEX_NORMAL) \
	||	defined(PTHREAD_MUTEX_INITIALIZER) \
	||	defined(PTHREAD_MUTEX_RECURSIVE) \
	||	defined(PTHREAD_ONCE_INIT) \
	||	defined(PTHREAD_PRIO_INHERIT) \
	||	defined(PTHREAD_PRIO_NONE) \
	||	defined(PTHREAD_PRIO_PROTECT) \
	||	defined(PTHREAD_PROCESS_SHARED) \
	||	defined(PTHREAD_PROCESS_PRIVATE) \
	||	defined(PTHREAD_RWLOCK_INITIALIZER) \
	||	defined(PTHREAD_SCOPE_PROCESS) \
	||	defined(PTHREAD_SCOPE_SYSTEM) \
)
#	error "Please define E4C_THREADSAFE at compiler level " \
"in order to enable the multi-thread version of exceptions4c."
# endif


/*@-exportany@*/


/* C99 features */
# if defined(_ISOC99_SOURCE) \
	||	defined(_GNU_SOURCE) \
	||	( defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) )

#	ifndef HAVE_C99_STDBOOL
#		define HAVE_C99_STDBOOL
#	endif

#	ifndef HAVE_C99_VARIADIC_MACROS
#		define HAVE_C99_VARIADIC_MACROS
#	endif

#	ifndef HAVE_C99_FUNC
#		define HAVE_C99_FUNC
#	endif

#	ifndef HAVE_C99_VSNPRINTF
#		define HAVE_C99_VSNPRINTF
#	endif

#	ifndef HAVE_C99_SNPRINTF
#		define HAVE_C99_SNPRINTF
#	endif

# endif


/* POSIX features */
# if defined(_POSIX_C_SOURCE) \
	||	defined(_POSIX_SOURCE) \
	||	defined(_POSIX_VERSION) \
	||	defined(_POSIX2_C_VERSION) \
	||	defined(_XOPEN_SOURCE) \
	||	defined(_XOPEN_VERSION) \
	||	defined(_XOPEN_SOURCE_EXTENDED) \
	||	defined(_GNU_SOURCE)

/*
 * POSIX.1 does not specify whether setjmp and longjmp save or restore the
 * current set of blocked signals. If a program employs signal handling it
 * should use POSIX's sigsetjmp/siglongjmp.
 */
#	ifndef HAVE_POSIX_SIGSETJMP
#		define HAVE_POSIX_SIGSETJMP
#	endif

# endif


# include <stdlib.h>
# include <setjmp.h>


# if defined(HAVE_C99_STDBOOL) || defined(HAVE_STDBOOL_H)
#	include <stdbool.h>
# endif

# if	defined(__bool_true_false_are_defined) \
	||	defined(bool) \
	||	defined(S_SPLINT_S)
#	define E4C_BOOL						bool
#	define E4C_FALSE					false
#	define E4C_TRUE						true
# else
#	define E4C_BOOL						int
#	define E4C_FALSE					0
#	define E4C_TRUE						1
# endif


/*
 * Make sure we can use exceptions4c within C++.
 */
# ifdef __cplusplus
	extern "C" {
# endif


/*
 * The E4C_FUNCTION_NAME_ compile-time parameter
 * could be defined in order to work with some specific compiler.
 */
# ifndef E4C_FUNCTION_NAME_

#	ifdef HAVE_C99_FUNC
#		define E4C_FUNCTION_NAME_		__func__

#	elif defined(__GNUC__)
#		if !defined(__OPTIMIZE__) && (__GNUC__ >= 2)
#			define E4C_FUNCTION_NAME_	__extension__ __FUNCTION__
#		else
#			define E4C_FUNCTION_NAME_	NULL
#		endif

#	else
#		define E4C_FUNCTION_NAME_		NULL
#	endif

# endif


/*
 * The E4C_INVALID_SIGNAL_NUMBER_ compile-time parameter
 * could be defined in order to work with some specific compiler.
 */
# ifndef E4C_INVALID_SIGNAL_NUMBER_

#	define E4C_INVALID_SIGNAL_NUMBER_	-1

# endif


/*
 * The E4C_NO_RETURN_ compile-time parameter
 * could be defined in order to work with some specific compiler.
 */
# ifdef E4C_NO_RETURN_
#	define E4C_UNREACHABLE_RETURN_(value)		( (void)0 )
#	define E4C_UNREACHABLE_VOID_RETURN_			( (void)0 )

# elif defined(__GNUC__)
#	define E4C_NO_RETURN_						__attribute__ ((noreturn))
#	define E4C_UNREACHABLE_RETURN_(value)		( (void)0 )
#	define E4C_UNREACHABLE_VOID_RETURN_			( (void)0 )

# elif defined(S_SPLINT_S)
#	define E4C_NO_RETURN_
#	define E4C_UNREACHABLE_RETURN_(value) \
		/*@-unreachable@*/ /*@-noeffect@*/ \
		( (void)0 ) \
		/*@=unreachable@*/ /*@=noeffect@*/
#	define E4C_UNREACHABLE_VOID_RETURN_ \
		/*@-unreachable@*/ /*@-noeffect@*/ \
		( (void)0 ) \
		/*@=unreachable@*/ /*@=noeffect@*/

# else
#	define E4C_NO_RETURN_
#	define E4C_UNREACHABLE_RETURN_(value)		return(value)
#	define E4C_UNREACHABLE_VOID_RETURN_			return

# endif


# if defined(HAVE_POSIX_SIGSETJMP) || defined(HAVE_SIGSETJMP)
#	define E4C_CONTINUATION_BUFFER_		sigjmp_buf
#	define E4C_CONTINUATION_CREATE_(continuation) \
		sigsetjmp(continuation->buffer, 1)
# else
#	define E4C_CONTINUATION_BUFFER_		jmp_buf
#	define E4C_CONTINUATION_CREATE_(continuation) \
		setjmp(continuation->buffer)
# endif


# ifndef NDEBUG
#	define E4C_INFO_FILE_				__FILE__
#	define E4C_INFO_LINE_				__LINE__
#	define E4C_INFO_FUNC_				E4C_FUNCTION_NAME_
#	define E4C_ASSERT(condition) ( \
		(condition) \
		? (void)0 \
		: E4C_THROW(AssertionException, "Assertion failed: " #condition) \
	)
# else
#	define E4C_INFO_FILE_				NULL
#	define E4C_INFO_LINE_				0
#	define E4C_INFO_FUNC_				NULL
#	define E4C_ASSERT(ignore)			( (void)0 )
# endif

# define E4C_INFO_ \
			E4C_INFO_FILE_, \
			E4C_INFO_LINE_, \
			E4C_INFO_FUNC_


# define E4C_PASTE_(x, y, z)			x ## _ ## y ## _ ## z
# define E4C_MANGLE_(pre, id, post)		E4C_PASTE_(pre, id, post)
# define E4C_AUTO_(id)					E4C_MANGLE_(_implicit, id, __LINE__)


# ifdef E4C_THREADSAFE
#	define E4C_VERSION_THREADSAFE_			1
#	define E4C_VERSION_THREADSAFE_STRING_	" (multi-thread)"
# else
#	define E4C_VERSION_THREADSAFE_			0
#	define E4C_VERSION_THREADSAFE_STRING_	" (single-thread)"
# endif


# define E4C_VERSION_STRING_(major, minor, revision) \
	#major "." #minor "." #revision E4C_VERSION_THREADSAFE_STRING_
# define E4C_VERSION_NUMBER_(major, minor, revision) ( \
	( (long)E4C_VERSION_THREADSAFE_	* 10000000L) +	\
	( (long)major					* 1000000L) +	\
	( (long)minor					* 1000L) +		\
	( (long)revision				* 1L)			\
)
# define E4C_VERSION_MAJOR_(major, minor, revision) ( (int)major )
# define E4C_VERSION_MINOR_(major, minor, revision) ( (int)minor )
# define E4C_VERSION_REVISION_(major, minor, revision) ( (int)revision )


/*
 * These undocumented macros hide implementation details from documentation.
 */

# define E4C_FRAME_LOOP_(stage) \
	if(E4C_CONTINUATION_CREATE_(e4c_frame_first_stage_(stage,E4C_INFO_)) >= 0) \
		while( e4c_frame_next_stage_() )

# define E4C_TRY \
	E4C_FRAME_LOOP_(e4c_acquiring_) \
	if( ( e4c_frame_get_stage_(E4C_INFO_) == e4c_trying_ ) \
		&& e4c_frame_next_stage_() )
	/* simple optimization: e4c_frame_next_stage_ will avoid disposing stage */

# define E4C_CATCH(exception_type) \
	else if( e4c_frame_catch_(&exception_type, E4C_INFO_) )

# define E4C_FINALLY \
	else if( e4c_frame_get_stage_(E4C_INFO_) == e4c_finalizing_ )

# define E4C_THROW(exception_type, message) \
	e4c_exception_throw_verbatim_(&exception_type, E4C_INFO_, message )

# define E4C_WITH(resource, dispose) \
	E4C_FRAME_LOOP_(e4c_beginning_) \
	if( e4c_frame_get_stage_(E4C_INFO_) == e4c_disposing_ ){ \
		dispose( \
			/*@-usedef@*/ (resource) /*@=usedef@*/, \
			(e4c_get_status() == e4c_failed) \
		); \
	}else if( e4c_frame_get_stage_(E4C_INFO_) == e4c_acquiring_ ){
	/*
	 * Splint detects resource being used before it is defined,
	 * but we *really* do define it before using, so we need to
	 * disable this check (usedef) for this specific parameter.
	 */

# define E4C_USE \
	}else if( e4c_frame_get_stage_(E4C_INFO_) == e4c_trying_ )

# define E4C_USING(type, resource, args) \
	E4C_WITH( (resource), e4c_dispose_##type){ \
		(resource) = e4c_acquire_##type args; \
	}E4C_USE

# define E4C_REUSING_CONTEXT(status, on_failure) \
	\
	volatile E4C_BOOL		E4C_AUTO_(BEGIN)	= !e4c_context_is_ready(); \
	volatile E4C_BOOL		E4C_AUTO_(DONE)		= E4C_FALSE; \
	\
	if( E4C_AUTO_(BEGIN) ){ \
		e4c_context_begin(E4C_FALSE); \
		E4C_TRY{ \
			goto E4C_AUTO_(PAYLOAD); \
			E4C_AUTO_(CLEANUP): \
			E4C_AUTO_(DONE) = E4C_TRUE; \
		}E4C_CATCH(RuntimeException){ \
			(status) = (on_failure); \
		} \
		e4c_context_end(); \
		E4C_AUTO_(DONE)		= E4C_TRUE; \
		E4C_AUTO_(BEGIN)	= E4C_FALSE; \
	} \
	\
	E4C_AUTO_(PAYLOAD): \
	for(; !E4C_AUTO_(DONE) || E4C_AUTO_(BEGIN); E4C_AUTO_(DONE) = E4C_TRUE) \
		if( E4C_AUTO_(DONE) ){ \
			goto E4C_AUTO_(CLEANUP); \
		}else

# define E4C_USING_CONTEXT(handle_signals) \
	\
	for( \
		e4c_context_begin(handle_signals); \
		e4c_context_is_ready(); \
		e4c_context_end() \
	)

# ifdef HAVE_C99_VARIADIC_MACROS
#	define E4C_THROWF(exception_type, format, ...) \
		e4c_exception_throw_format_( \
			&exception_type, E4C_INFO_, format, __VA_ARGS__ \
		)
# endif

# define E4C_RETHROW(message) \
	e4c_exception_throw_verbatim_( \
		( \
			e4c_get_exception() == NULL \
			? &NullPointerException \
			: e4c_get_exception()->type \
		), \
		E4C_INFO_, message \
	)

# ifdef HAVE_C99_VARIADIC_MACROS
#	define E4C_RETHROWF(format, ...) \
		e4c_exception_throw_format_( \
			( e4c_get_exception() == NULL ? NULL : e4c_get_exception()->type), \
			E4C_INFO_, format, __VA_ARGS__ \
		)
# endif

# define E4C_RETRY(max_retry_attempts) \
	e4c_frame_repeat_(max_retry_attempts, e4c_acquiring_, E4C_INFO_)

# define E4C_REACQUIRE(max_reacquire_attempts) \
	e4c_frame_repeat_(max_reacquire_attempts, e4c_beginning_, E4C_INFO_)


/**
 * @name Exception handling keywords
 *
 * This set of keywords express the semantics of exception handling.
 *
 * @{
 */

/**
 * Introduces a block of code aware of exceptions
 *
 * A `#try` statement executes a block of code. If an exception is thrown and
 * there is a `#catch` block that can handle it, then control will be
 * transferred to it. If there is a `#finally` block, then it will be executed,
 * no matter whether the `try` block completes normally or abruptly, and no
 * matter whether a `catch` block is first given control.
 *
 * The block of code immediately after the keyword `try` is called **the `try`
 * block** of the `try` statement. The block of code immediately after the
 * keyword `finally` is called **the `finally` block** of the `try` statement.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   stack_t * stack = stack_new();
 *   try{
 *       // the try block
 *       int value = stack_pop(stack);
 *       stack_push(stack, 16);
 *       stack_push(stack, 32);
 *   }catch(StackOverflowException){
 *       // a catch block
 *       printf("Could not push.");
 *   }catch(StackUnderflowException){
 *       // another catch block
 *       printf("Could not pop.");
 *   }finally{
 *       // the finally block
 *       stack_delete(stack);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * One `try` block may precede many `catch` blocks (also called *exception
 * handlers*). A `catch` block **must** have exactly one parameter, which is the
 * exception type it is capable of handling. Within the `catch` block, the
 * exception can be accessed through the function `#e4c_get_exception`.
 * Exception handlers are considered in left-to-right order: the earliest
 * possible `catch` block handles the exception. If no `catch` block can handle
 * the thrown exception, it will be *propagated*.
 *
 * Sometimes it may come in handy to `#retry` an entire `try` block; for
 * instance, once the exception has been caught and the error condition has been
 * solved.
 *
 * A `try` block has an associated [status](@ref e4c_status) according to the
 * way it has been executed:
 *
 *   - It *succeeds* when the execution reaches the end of the block without
 *     any exceptions.
 *   - It *recovers* when an exception is thrown but a `catch` block handles it.
 *   - It *fails* when an exception is thrown and it's not caught.
 *
 * The status of the current `try` block can be retrieved through the function
 * `#e4c_get_status`.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to using
 *     the keyword `try`. Such programming error will lead to an abrupt exit of
 *     the program (or thread).
 *   - A `try` block **must** precede, at least, another block of code,
 *     introduced by either `catch` or `finally`.
 *     - A `try` block may precede several `catch` blocks.
 *     - A `try` block can precede, at most, one `finally` block.
 *   - A `try` block **must not** be exited through any of: `goto`, `break`,
 *     `continue` or `return` (but it is legal to `#throw` an exception).
 *
 * @post
 *   - A `finally` block will be executed after the `try` block and any `catch`
 *     block that might be executed, no matter whether the `try` block
 *     *succeeds*, *recovers* or *fails*.
 *
 * @see     #catch
 * @see     #finally
 * @see     #retry
 * @see     #e4c_status
 * @see     #e4c_get_status
 */
# ifndef E4C_NOKEYWORDS
# define try E4C_TRY
# endif

/**
 * Introduces a block of code capable of handling a specific type of exceptions
 *
 * @param   exception_type
 *          The type of exceptions to be handled
 *
 * `#catch` blocks are optional code blocks that **must** be preceded by `#try`,
 * `#with`... `#use` or `#using` blocks. Several `catch` blocks can be placed
 * next to one another.
 *
 * When an exception is thrown, the system looks for a `catch` block to handle
 * it. The first capable block (in order of appearance) will be executed. The
 * exception is said to be *caught* and the `try` block is in *recovered*
 * (status)[@ ref e4c_status].
 *
 * The caught exception can be accessed through the function #e4c_get_exception.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   try{
 *       ...
 *   }catch(RuntimeException){
 *       const e4c_exception * exception = e4c_get_exception();
 *       printf("Error: %s", exception->message);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * The actual type of the exception can be checked against other exception types
 * through the function #e4c_is_instance_of.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   try{
 *       ...
 *   }catch(RuntimeException){
 *       const e4c_exception * exception = e4c_get_exception();
 *       if( e4c_is_instance_of(exception, &SignalException) ){
 *           // the exception type is SignalException or any subtype
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * The type might also be compared directly against another specific exception
 * type.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   try{
 *       ...
 *   }catch(RuntimeException){
 *       const e4c_exception * exception = e4c_get_exception();
 *       if(exception->type == &NotEnoughMemoryException){
 *           // the exception type is precisely NotEnoughMemoryException
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * After the `catch` block completes, the `#finally` block (if any) is executed.
 * Then the program continues by the next line following the set of `try`...
 * `catch`... `finally` blocks.
 *
 * However, if an exception is thrown in a `catch` block, then the `finally`
 * block will be executed right away and the system will look for an outter
 * `catch` block to handle it.
 *
 * Only one of all the `catch` blocks will be executed for each `try` block,
 * even though the executed `catch` block throws another exception. The only
 * possible way to execute more than one `catch` block would be by `#retry`ing
 * the entire `try` block.
 *
 * @pre
 *   - A `catch` block **must** be preceded by one of these blocks:
 *     - A `try` block
 *     - A `with`... `#use` block
 *     - A `using` block
 *     - Another `catch` block
 *   - A `catch` block **must not** be exited through any of: `goto`, `break`,
 *     `continue` or `return` (but it is legal to `#throw` an exception).
 *
 * @see     #try
 * @see     #e4c_exception_type
 * @see     #e4c_get_exception
 * @see     #e4c_exception
 * @see     #e4c_is_instance_of
 */
# ifndef E4C_NOKEYWORDS
# define catch(exception_type) E4C_CATCH(exception_type)
# endif

/**
 * Introduces a block of code responsible for cleaning up the previous
 * exception-aware block
 *
 * `#finally` blocks are optional code blocks that **must** be preceded by
 * `#try`, `#with`... `#use` or `#using` blocks. It is allowed to place, at
 * most, one `finally` block for each one of these.
 *
 * The `finally` block can determine the completeness of the *exception-aware*
 * block through the function `#e4c_get_status`. The thrown exception (if any)
 * can also be accessed through the function `#e4c_get_exception`.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   try{
 *      ...
 *   }finally{
 *      switch( e4c_get_status() ){
 *
 *          case e4c_succeeded:
 *              ...
 *              break;
 *
 *          case e4c_recovered:
 *              ...
 *              break;
 *
 *          case e4c_failed:
 *              ...
 *              break;
 *      }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * The finally block will be executed only **once**. The only possible way to
 * execute it again would be by `#retry`ing the entire `try` block.
 *
 * @pre
 *   - A `finally` block **must** be preceded by a `try`, `with`... `use`,
 *     `using` or `catch` block.
 *   - A `finally` block **must not** be exited through any of: `goto`, `break`,
 *     `continue` or `return` (but it is legal to `#throw` an exception).
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to using
 *     the keyword `finally`. Such programming error will lead to an abrupt exit
 *     of the program (or thread).
 *
 * @see     #e4c_exception
 * @see     #e4c_get_exception
 * @see     #e4c_get_status
 * @see     #e4c_status
 */
# ifndef E4C_NOKEYWORDS
# define finally E4C_FINALLY
# endif

/**
 * Repeats the previous `#try` (or `#use`) block entirely
 *
 * @param   max_retry_attempts
 *          The maximum number of attempts to retry
 *
 * This macro discards any thrown exception (if any) and repeats the previous
 * `try` or `use` block, up to a specified maximum number of attempts. It is
 * intended to be used within `#catch` or `#finally` blocks as a quick way to
 * fix an error condition and *try again*.
 *
 * If the block has already been *tried* at least the specified number of times,
 * then the program continues by the next line following `retry` clause.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   const char * file_path = config_get_user_defined_file_path();
 *
 *   try{
 *       config = read_config(file_path);
 *   }catch(ConfigException){
 *       file_path = config_get_default_file_path();
 *       retry(1);
 *       rethrow("Wrong defaults.");
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @warning
 * If the specified maximum number of attempts is *zero*, then the block can
 * eventually be attempted an unlimited number of times. Care should be taken in
 * order not to create an *infinite loop*.
 *
 * This macro won't return control unless the block has already been attempted,
 * at least, the specified maximum number of times.
 *
 * @note
 * Once a `catch` code block is being executed, the current exception is
 * considered caught. If you want the exception to be propagated when the
 * maximum number of attempts has been reached, then you need to `#rethrow` it.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   int dividend = 100;
 *   int divisor = 0;
 *   int result = 0;
 *
 *   try{
 *       result = dividend / divisor;
 *       do_something(result);
 *   }catch(RuntimeException){
 *       divisor = 1;
 *       retry(1);
 *       rethrow("Error (not a division by zero).");
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @note
 * At a `finally` block, the current exception (if any) will be propagated when
 * the `retry` does not take place, so you don't need to `rethrow` it again.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   int dividend = 100;
 *   int divisor = 0;
 *   int result = 0;
 *
 *   try{
 *       result = dividend / divisor;
 *       do_something(result);
 *   }finally{
 *       if( e4c_get_status() == e4c_failed ){
 *           divisor = 1;
 *           retry(1);
 *           // when we get here, the exception will be propagated
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to using
 *     the keyword `retry`. Such programming error will lead to an abrupt exit
 *     of the program (or thread).
 *   - The `retry` keyword **must** be used from a `catch` or `finally` block.
 * @post
 *   - Control does not return to the `retry` point, unless the `try` (or `use`)
 *     block has already been attempted, at least, the specified number of times.
 *
 * @see     #reacquire
 * @see     #try
 * @see     #use
 */
# ifndef E4C_NOKEYWORDS
#	define retry(max_retry_attempts) E4C_RETRY(max_retry_attempts)
# endif

/**
 * Signals an exceptional situation represented by an exception object
 *
 * @param   exception_type
 *          The type of exception to be thrown
 * @param   message
 *          The *ad-hoc* message describing the exception. If `NULL`, then the
 *          default message for the specified exception type will be used
 *
 * Creates a new instance of the specified type of exception and then throws it.
 * The message is copied into the thrown exception, so it may be freely
 * deallocated. If `NULL` is passed, then the default message for that type of
 * exception will be used.
 *
 * When an exception is thrown, the exception handling framework looks for the
 * appropriate `#catch` block that can handle the exception. The system unwinds
 * the call chain of the program and executes the `#finally` blocks it finds.
 *
 * When no `catch` block is able to handle an exception, the system eventually
 * gets to the main function of the program. This situation is called an
 * **uncaught exception**.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to using
 *     the keyword `throw`. Such programming error will lead to an abrupt exit
 *     of the program (or thread).
 * @post
 *   - Control does not return to the `throw` point.
 *
 * @see     #throwf
 * @see     #rethrow
 * @see     #e4c_exception_type
 * @see     #e4c_exception
 * @see     #e4c_uncaught_handler
 * @see     #e4c_get_exception
 */
# ifndef E4C_NOKEYWORDS
# define throw(exception_type, message) \
	E4C_THROW(exception_type, message)
# endif

/**
 * Throws again the currently thrown exception, with a new message
 *
 * @param   message
 *          The new message describing the exception. It should be more specific
 *          than the current one
 *
 * This macro creates a new instance of the thrown exception, with a more
 * specific message.
 *
 * `rethrow` is intended to be used within a `#catch` block; the purpose is to
 * refine the message of the currently caught exception. The previous exception
 * (and its message) will be stored as the *cause* of the newly thrown
 * exception.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   try{
 *       image = read_file(file_path);
 *   }catch(FileOpenException){
 *       rethrow("Image not found.");
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * The semantics of this keyword are the same as for `throw`.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to using
 *     the keyword `rethrow`. Such programming error will lead to an abrupt exit
 *     of the program (or thread).
 * @post
 *   - Control does not return to the `rethrow` point.
 *
 * @see     #throw
 * @see     #rethrowf
 */
# ifndef E4C_NOKEYWORDS
#	define rethrow(message) E4C_RETHROW(message)
# endif

/** @} */

/**
 * @name Dispose pattern keywords
 *
 * This set of keywords express the semantics of automatic resource acquisition
 * and disposal.
 *
 * @{
 */

/**
 * Opens a block of code with automatic disposal of a resource
 *
 * @param   resource
 *          The resource to be disposed
 * @param   dispose
 *          The name of the disposal function (or macro)
 *
 * The combination of keywords `with`... `#use` encapsules the *Dispose
 * Pattern*. This pattern consists of two separate blocks and an implicit call
 * to a given function:
 *
 *   - the `with` block is responsible for the resource acquisition
 *   - the `use` block makes use of the resource
 *   - the disposal function will be called implicitly
 *
 * A `with` block **must** be followed by a `use` block. In addition, the `use`
 * block may be followed by several `#catch` blocks and/or one `#finally` block.
 *
 * The `with` keyword guarantees that the disposal function will be called **if,
 * and only if**, the acquisition block is *completed* without any errors (i.e.
 * the acquisition block does not `#throw` any exceptions).
 *
 * If the `with` block does not complete, neither the disposal function nor the
 * `use` block will be executed.
 *
 * The disposal function is called right after the `use` block. If an exception
 * is thrown, the `catch` or `finally` blocks (if any) will take place **after**
 * the disposal of the resource.
 *
 * When called, the disposal function will receive two arguments:
 *
 *   - The resource
 *   - A boolean flag indicating if the `use` block *failed* (i. e. did not
 *     *complete*)
 *
 * This way, different actions can be taken depending on the success or failure
 * of the block. For example, commiting or rolling back a *transaction*
 * resource.
 *
 * Legacy functions can be reused by defining macros. For example, a file
 * resource needs to be closed regardless of the errors occurred. Since the
 * function `fclose` only takes one parameter, we could define the next macro:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   # define e4c_dispose_file(file , failed) fclose(file)
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Here is the typical usage of `with`... `use`:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   with(foo, e4c_dispose_foo){
 *       foo = e4c_acquire_foo(foobar);
 *       validate_foo(foo, bar);
 *       ...
 *   }use{
 *       do_something(foo);
 *       ...
 *   }catch(FooAcquisitionFailed){
 *       recover1(foobar);
 *       ...
 *   }catch(SomethingElseFailed){
 *       recover2(foo);
 *       ...
 *   }finally{
 *       cleanup();
 *       ...
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Nonetheless, *one-liners* fit nicely too:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   with(bar, e4c_dispose_bar) bar = e4c_acquire_bar(baz, 123); use foo(bar);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * There is a way to lighten up even more this pattern by defining convenience
 * macros, customized for a specific kind of resources. For example,
 * `#e4c_using_file` or `#e4c_using_memory`.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to using
 *     the keyword `with`. Such programming error will lead to an abrupt exit of
 *     the program (or thread).
 *   - A `with` block **must not** be exited through any of: `goto`, `break`,
 *     `continue` or `return` (but it is legal to `throw` an exception).
 *   - A `with` block **must** always be followed by a `use` block.
 *
 * @see     #use
 * @see     #using
 */
# ifndef E4C_NOKEYWORDS
# define with(resource, dispose) E4C_WITH(resource, dispose)
# endif

/**
 * Closes a block of code with automatic disposal of a resource
 *
 * A `#use` block **must** always be preceded by a `with` block. These two
 * keywords are designed so the compiler will complain about *dangling*
 * `with`... `use` blocks.
 *
 * A code block introduced by the `use` keyword will only be executed *if, and
 * only if*, the acquisition of the resource *completes* without exceptions.
 *
 * The disposal function will be executed, either if the `use` block completes
 * or not.
 *
 * @pre
 *   - A `use` block **must** be preceded by a `with` block.
 *   - A `use` block **must not** be exited through any of: `goto`, `break`,
 *     `continue` or `return`  (but it is legal to `#throw` an exception).
 *
 * @see     #with
 */
# ifndef E4C_NOKEYWORDS
# define use E4C_USE
# endif

/**
 * Introduces a block of code with automatic acquisition and disposal of a
 * resource
 *
 * @param   type
 *          The type of the resource
 * @param   resource
 *          The resource to be acquired, used and then disposed
 * @param   args
 *          A list of arguments to be passed to the acquisition function
 *
 * The specified resource will be *acquired*, *used* and then *disposed*. The
 * automatic acquisition and disposal is achieved by calling the *implicit*
 * functions **e4c_acquire_<em>type</em>** and **e4c_dispose_<em>type</em>**:
 *
 *   - `TYPE e4c_acquire_TYPE(ARGS)`
 *   - `void e4c_dispose_TYPE(TYPE RESOURCE, E4C_BOOL failed)`
 *
 * These two symbols **must** exist, in the form of either *functions* or
 * *macros*.
 *
 * The semantics of the automatic acquisition and disposal are the same as for
 * blocks introduced by `#with`... `#use`. For example, a `using` block can also
 * precede `#catch` and `#finally` blocks.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to using
 *     the keyword `using`. Such programming error will lead to an abrupt exit
 *     of the program (or thread).
 *   - A `using` block **must not** be exited through any of: `goto`, `break`,
 *     `continue` or `return`  (but it is legal to `#throw` an exception).
 *
 * @see     #with
 */
# ifndef E4C_NOKEYWORDS
# define using(type, resource, args) E4C_USING(type, resource, args)
# endif

/**
 * Repeats the previous `#with` block entirely
 *
 * @param   max_reacquire_attempts
 *          The maximum number of attempts to reacquire
 *
 * This macro discards any thrown exception (if any) and repeats the previous
 * `with` block, up to a specified maximum number of attempts. If the
 * acquisition completes, then the `use` block will be executed.
 *
 * It is intended to be used within `#catch` or `#finally` blocks, next to a
 * `#with`... `#use` or `#using` block, when the resource acquisition fails,
 * as a quick way to fix an error condition and try to acquire the resource
 * again.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   image_type * image;
 *   const char * image_path = image_get_user_avatar();
 *
 *   with(image, e4c_image_dispose){
 *       image = e4c_image_acquire(image_path);
 *   }use{
 *       image_show(image);
 *   }catch(ImageNotFoundException){
 *       image_path = image_get_default_avatar();
 *       reacquire(1);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @warning
 * If the specified maximum number of attempts is *zero*, then the `with` block
 * can eventually be attempted an unlimited number of times. Care should be
 * taken in order not to create an *infinite loop*.
 *
 * Once the resource has been acquired, the `use` block can also be repeated
 * *alone* through the keyword `#retry`:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   image_type * image;
 *   const char * image_path = image_get_user_avatar();
 *   display_type * display = display_get_user_screen();
 *
 *   with(image, e4c_image_dispose){
 *       image = e4c_image_acquire(image_path);
 *   }use{
 *       image_show(image, display);
 *   }catch(ImageNotFoundException){
 *       image_path = image_get_default_avatar();
 *       reacquire(1);
 *   }catch(DisplayException){
 *       display = display_get_default_screen();
 *       retry(1);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @pre
 *   - The `reacquire` keyword **must** be used from a `catch` or `finally`
 *     block, preceded by `with`... `use` or `using` blocks.
 *   - A program (or thread) **must** begin an exception context prior to using
 *     the keyword `reacquire`. Such programming error will lead to an abrupt
 *     exit of the program (or thread).
 * @post
 *   - This macro won't return control unless the `with` block has already been
 *     attempted, at least, the specified maximum number of times.
 *
 * @see     #retry
 * @see     #with
 * @see     #use
 */
# ifndef E4C_NOKEYWORDS
#	define reacquire(max_reacquire_attempts) \
		E4C_REACQUIRE(max_reacquire_attempts)
# endif

/** @} */

/**
 * @name Integration macros
 *
 * These macros are designed to ease the integration of external libraries which
 * make use of the exception handling system.
 *
 * @{
 */

/**
 * Provides the library version number
 *
 * The library version number is a `long` value which expresses:
 *
 *   - library thread mode (either *single-thread* or *multi-thread*)
 *   - library *major* version number
 *   - library *minor* version number
 *   - library *revision* number
 *
 * The multi-thread (or *thread-safe*) mode can be enabled by compiling the
 * library with the `E4C_THREADSAFE` *compile-time* parameter.
 *
 * The formula to encode these version numbers into a single `long` value is:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   THREADSAFE * 10000000 + MAJOR * 1000000 + MINOR * 1000 + REVISION
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * These numbers can be obtained separately through the next macros:
 *
 *   - `#E4C_VERSION_THREADSAFE`
 *   - `#E4C_VERSION_MAJOR`
 *   - `#E4C_VERSION_MINOR`
 *   - `#E4C_VERSION_REVISION`
 *
 * The library version number can be also obtained as a string literal in the
 * format "MAJOR.MINOR.REVISION (THREADSAFE)" through the macro
 * `#E4C_VERSION_STRING`.
 *
 * @note
 * This version number can be considered as the *compile-time* library version
 * number, as opposed to the *run-time* library version number (associated with
 * the actual, compiled library). This *run-time* version number can be obtained
 * through the function `#e4c_library_version`.
 *
 * @remark
 * The library **must** be compiled with the corresponding header (i.e. library
 * version number should be equal to header version number).
 *
 * @see     #e4c_library_version
 * @see     #E4C_VERSION_THREADSAFE
 * @see     #E4C_VERSION_MAJOR
 * @see     #E4C_VERSION_MINOR
 * @see     #E4C_VERSION_REVISION
 * @see     #E4C_VERSION_STRING
 */
# define E4C_VERSION_NUMBER \
	\
	E4C_VERSION_(E4C_VERSION_NUMBER_)

/**
 * Provides the library thread mode (either single-thread or multi-thread)
 *
 * When the library is compiled with the `E4C_THREADSAFE` *compile-time*
 * parameter, `#E4C_VERSION_THREADSAFE` will yield the `int` value `1` (meaning
 * *multi-thread* mode), otherwise it will yield the `int` value `0` (meaning
 * *single-thread* mode).
 *
 * @see     #E4C_VERSION_NUMBER
 */
# define E4C_VERSION_THREADSAFE \
	\
	E4C_VERSION_THREADSAFE_

/**
 * Provides the library major version number
 *
 * The library major version number is an `int` value which is incremented from
 * one release to another when there are **significant changes in
 * functionality**.
 *
 * @see     #E4C_VERSION_NUMBER
 */
# define E4C_VERSION_MAJOR \
	\
	E4C_VERSION_(E4C_VERSION_MAJOR_)

/**
 * Provides the library minor version number
 *
 * The library minor version number is an `int` value which is incremented from
 * one release to another when **only minor features or significant fixes have
 * been added**.
 *
 * @see     #E4C_VERSION_NUMBER
 */
# define E4C_VERSION_MINOR \
	\
	E4C_VERSION_(E4C_VERSION_MINOR_)

/**
 * Provides the library revision number
 *
 * The library revision number is an `int` value which is incremented from one
 * release to another when **minor bugs are fixed**.
 *
 * @see     #E4C_VERSION_NUMBER
 */
# define E4C_VERSION_REVISION \
	\
	E4C_VERSION_(E4C_VERSION_REVISION_)

/**
 * Provides the library version number as a string literal
 *
 * The format of the string literal is: "MAJOR.MINOR.REVISION (THREADSAFE)".
 *
 * @see     #E4C_VERSION_NUMBER
 */
# define E4C_VERSION_STRING \
	\
	E4C_VERSION_(E4C_VERSION_STRING_)

/**
 * Provides the maximum length (in bytes) of an exception message
 */
# ifndef E4C_EXCEPTION_MESSAGE_SIZE
#	define E4C_EXCEPTION_MESSAGE_SIZE 128
# endif

/**
 * Reuses an existing exception context, otherwise, begins a new one and then
 * ends it.
 *
 * @param   status
 *          The name of a previously defined variable, or lvalue, which will be
 *          assigned the specified failure value
 * @param   on_failure
 *          A constant value or expression that will be assigned to the
 *          specified *lvalue* in case of failure
 *
 * This macro lets library developers use the exception framework, regardless of
 * whether the library clients are unaware of the exception handling system. In
 * a nutshell, function libraries can use `#try`, `#catch`, `#throw`, etc.
 * whether the client previously began an exception context or not.
 *
 * You **must not** use this macro unless you are implementing some
 * functionality which is to be called from another program, potentially unaware
 * of exceptions.
 *
 * When the block completes, the system returns to its previous status (if it
 * was required to open a new exception context, then it will be automatically
 * closed).
 *
 * This way, when an external function encounters an error, it may either throw
 * an exception (when the caller is aware of the exception system), or otherwise
 * return an error code (when the caller did not open an exception context).
 *
 * `e4c_reusing_context` takes two parameters:
 *
 *   - `status` (*lvalue*)
 *   - `on_failure` (*rvalue*)
 *
 * `status` **must** will be assigned `on_failure` **if, and only if**, an
 * exception is thrown inside the block. Needless to say, `on_failure` may be an
 * expression assignable to `status`.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   int library_public_function(void * pointer, int number){
 *
 *       // We don't know where this function is going to be called from, so:
 *       //   - We cannot use "try", "throw", etc. right here, because the
 *       //     exception context COULD be uninitialized at this very moment.
 *       //   - We cannot call "e4c_context_begin" either, because the
 *       //     exception context COULD be already initialized.
 *       // If we have to make use of the exception handling system, we need
 *       // to "reuse" the existing exception context or "use" a new one.
 *
 *       volatile int status = STATUS_OK;
 *
 *       e4c_reusing_context(status, STATUS_ERROR){
 *
 *           // Now we can safely use "try", "throw", etc.
 *           if(pointer == NULL){
 *               throw(NullPointerException);
 *           }
 *
 *           library_private_function(pointer, number);
 *       }
 *
 *       return(status);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * `status` will be left unmodified if the client (i.e. the function caller)
 * is *exception-aware*, or the block *completes* without an error (i.e. no
 * exception is thrown), so it **must** be properly initialized before returning
 * it.
 *
 * Please note that `status` doesn't have to be just a dichotomy (success or
 * failure). It can be a fine-grained value describing what exactly went wrong.
 * You can pass any expression to `e4c_reusing_context` as `on_failure`; it will
 * be evaluated when an exception is thrown:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   int library_public_function(void * pointer, int number){
 *
 *       volatile int status = STATUS_OK;
 *       volatile bool flag = true;
 *
 *       e4c_reusing_context(status,(flag?STATUS_NULL_POINTER:STATUS_ERROR)){
 *
 *           if(pointer == NULL){
 *               throw(NullPointerException);
 *           }
 *
 *           flag = false;
 *
 *           library_private_function(pointer, number);
 *       }
 *
 *       return(status);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * However, Most of the times you probably want to yield a different status
 * value depending on the specific exception being thrown. This can be easily
 * accomplished by making use of the macro `#E4C_ON_FAILURE`.
 *
 * Next, the semantics of `e4c_reusing_context` are explained, step by step:
 *
 *   - If there is an exception context at the time the block starts:
 *     1. The existing exception context will be reused.
 *     2. The code block will take place.
 *     3. If any exception is thrown during the execution of the block:
 *       * It will be **propagated** upwards to the caller.
 *       * The control will be transferred to the nearest surrounding block of
 *         code which is able to handle that exception.
 *   - If there is no exception context at the time the block starts:
 *     1. A new exception context will be begun; note that the signal handling
 *        system **WILL NOT** be set up.
 *     2. The code block will take place.
 *     3. If any exception is thrown during the execution of the block:
 *       * It will be **caught**.
 *       * `status` will be asigned the value of the expression `on_failure`.
 *
 * If you need to perform any cleanup, you should place it *inside* a
 * `#finally` block.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   ...
 *   e4c_reusing_context(status, STATUS_ERROR){
 *
 *       void * buffer = NULL;
 *       try{
 *           buffer = malloc(1024);
 *           ...
 *       }finally{
 *           free(buffer);
 *       }
 *   }
 *   ...
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * If you need to rely on the signal handling system, you may call
 * `#e4c_context_set_signal_mappings` explicitly. You should take into account
 * that you could be *hijacking* your client's original signal mappings, so you
 * should also call `#e4c_context_get_signal_mappings` in order to restore the
 * previous signal mappings when you are done.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   const e4c_signal_mapping new_mappings[] = {
 *       E4C_SIGNAL_MAPPING(SIGABRT, Exception1),
 *       E4C_SIGNAL_MAPPING(SIGINT, Exception2),
 *       E4C_IGNORE_SIGNAL(SIGTERM),
 *       ...
 *       E4C_NULL_SIGNAL_MAPPING
 *   };
 *   ...
 *   e4c_reusing_context(status, STATUS_ERROR){
 *
 *       const e4c_signal_mapping * old_mappings = e4c_context_get_signal_mappings();
 *
 *       e4c_context_set_signal_mappings(new_mappings);
 *
 *       try{
 *           ...
 *       }finally{
 *           e4c_context_set_signal_mappings(old_mappings);
 *       }
 *   }
 *   ...
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This macro only begins a new exception context **if there is no one, already
 * begun, to be used** whereas `#e4c_using_context` always attempts to begin a
 * new one.
 *
 * @pre
 *   - A block introduced by `e4c_reusing_context` **must not** be exited
 *     through any of: `goto`, `break`, `continue` or `return` (but it is legal
 *     to `#throw` an exception).
 * @post
 *   - A block introduced by `e4c_reusing_context` is guaranteed to take place
 *     *inside* an exception context.
 *
 * @see     #e4c_context_begin
 * @see     #e4c_context_end
 * @see     #e4c_context_is_ready
 * @see     #e4c_using_context
 * @see     #e4c_exception
 * @see     #E4C_ON_FAILURE
 */
# define e4c_reusing_context(status, on_failure) \
	E4C_REUSING_CONTEXT(status, on_failure)

/**
 * Provides a means of parsing an exception to obtain a status value
 *
 * @param   handler
 *          The name of the parser function to be called
 *
 * This is a handy way to call a function when a #e4c_reusing_context block
 * fails. This function will be passed the current thrown exception; it is
 * expected to parse it and return a proper `status` value.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   static int parse_exception(const e4c_exception * exception){
 *
 *       if(exception->type == &NotEnoughMemoryException){
 *           return(STATUS_MEMORY_ERROR);
 *       }else if( is_instance_of(exception, &MyException) ){
 *           return(STATUS_MY_ERROR);
 *       }
 *
 *       return(STATUS_ERROR);
 *   }
 *
 *   int library_public_function(void * pointer, int number){
 *
 *       volatile int status = STATUS_OK;
 *
 *       e4c_reusing_context(status, E4C_ON_FAILURE(parse_exception)){
 *
 *           if(pointer == NULL){
 *               throw(NullPointerException);
 *           }
 *
 *           library_private_function(pointer, number);
 *       }
 *
 *       return(status);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @see     #e4c_reusing_context
 * @see     #e4c_get_exception
 * @see     #e4c_exception
 */
# define E4C_ON_FAILURE(handler) handler( e4c_get_exception() )

/**
 * Marks a function which never returns
 *
 * This macro helps both developer and compiler to assume that the marked
 * function will not return the control to its caller (unless by throwing an
 * exception).
 *
 * @note
 * It does not make sense for these functions to have a return type other than
 * `void`.
 *
 * For example, a function `f1` that **always** throws an exception, could be
 * marked with this macro:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   void f1(int foo) E4C_NO_RETURN;
 *   // ...
 *   void f1(int foo){
 *       if(foo == 1){
 *           throw(MyException1, "foo is one.");
 *       }
 *       throw(MyException2, "foo is not one.");
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Then, if another function tested a condition and then called `f1`, it
 * wouldn't need to return anything witnin the `if` branch, nor consider the
 * `else` branch of the test:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   int f2(int bar, int foo){
 *
 *       if(bar == 0){
 *           f1(foo);
 *           // return(-1);
 *       }// else
 *
 *       return(123);
 *
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * If the compiler supports this macro, it could optimize the program and avoid
 * spurious warnings of uninitialized variables.
 *
 * @see     #E4C_UNREACHABLE_RETURN
 */
# define E4C_NO_RETURN \
	\
	E4C_NO_RETURN_

/**
 * Simulates a function return
 *
 * @param   value
 *          The value that would be returned if the statement took place.
 *
 * This macro ensures portability on compilers which don't support functions
 * that never return.
 *
 * It may be used after calling a function marked as `#E4C_NO_RETURN`, so that
 * the compiler will not complain about *control reaching end of non-void
 * function*. For example:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   void f1(int foo) E4C_NO_RETURN;
 *
 *   int f3(int bar, int foo){
 *
 *       if(bar != 0){
 *           return(123);
 *       }
 *
 *       f1(123);
 *
 *       E4C_UNREACHABLE_RETURN(-1);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This macro will become an actual `return`  statement if the compiler does not
 * support `E4C_NO_RETURN`, even though it will never be reached (because the
 * called function won't actually return control).
 *
 * @see     #E4C_NO_RETURN
 * @see     #E4C_UNREACHABLE_VOID_RETURN
 */
# define E4C_UNREACHABLE_RETURN(value) \
	\
	E4C_UNREACHABLE_RETURN_(value)

/**
 * Simulates a function void return
 *
 * This macro ensures portability on static source code analyzers which don't
 * support functions that never return.
 *
 * It may be used after calling a function marked as `#E4C_NO_RETURN`, so that
 * the analyzer will not complain about spurious errors. For example, if we
 * didn't use `E4C_UNREACHABLE_VOID_RETURN` here, some analyzers might complain
 * about *possible null pointer dereference* at line `foo = *bar`, because they
 * are not aware that function call `f1(321);` will never return control:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   void f1(int foo) E4C_NO_RETURN;
 *
 *   void f3(int * bar){
 *
 *       int foo;
 *
 *       if(bar == NULL){
 *           f1(321);
 *           E4C_UNREACHABLE_VOID_RETURN;
 *       }
 *
 *       foo = *bar;
 *       printf("value: %d", foo);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This macro will become an actual `return`  statement if the compiler does not
 * support `E4C_NO_RETURN`, even though it will never be reached (because the
 * called function won't actually return control).
 *
 * @see     #E4C_NO_RETURN
 * @see     #E4C_UNREACHABLE_RETURN
 */
# define E4C_UNREACHABLE_VOID_RETURN \
	\
	E4C_UNREACHABLE_VOID_RETURN_

/** @} */

/**
 * @name Other convenience macros
 *
 * These macros provide a handy way to: begin (and end) implicitly a new
 * exception context, express *assertions*, define and declare exceptions, and
 * define arrays of signal mappings.
 *
 * @{
 */

/**
 * Introduces a block of code which will use a new exception context.
 *
 * @param   handle_signals
 *          If `true`, the signal handling system will be set up with the
 *          default mapping.
 *
 * This macro begins a new exception context to be used by the code block right
 * next to it. When the code completes, `#e4c_context_end` will be called
 * implicitly.
 *
 * This macro is very convenient when the beginning and the ending of the
 * current exception context are next to each other. For example, there is no
 * semantic difference between this two blocks of code:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   // block 1
 *   e4c_context_begin(E4C_TRUE);
 *   // ...
 *   e4c_context_end();
 *
 *   // block 2
 *   e4c_using_context(E4C_TRUE){
 *       // ...
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This macro **always** attempts to begin a new exception context, whereas
 * `#e4c_reusing_context` only does if there is no exception context, already
 * begun, to be used.
 *
 * This macro **should be used whenever possible**, rather than doing the
 * explicit, manual calls to `#e4c_context_begin` and `#e4c_context_end`,
 * because it is less prone to errors.
 *
 * @pre
 *   - A block introduced by `e4c_using_context` **must not** be exited through
 *     any of: `goto`, `break`, `continue` or `return` (but it is legal to
 *     `#throw` an exception).
 * @post
 *   - A block introduced by `#e4c_using_context` is guaranteed to take place
 *     *inside* an exception context.
 *
 * @see     #e4c_context_begin
 * @see     #e4c_context_end
 * @see     #e4c_reusing_context
 */
# define e4c_using_context(handle_signals) \
	E4C_USING_CONTEXT(handle_signals)

/**
 * Expresses a program assertion
 *
 * @param   condition
 *          A predicate that must evaluate to `true`
 *
 * An assertion is a mechanism to express that the developer *thinks* that a
 * specific condition is always met at some point of the program.
 *
 * `assert` is a convenient way to insert debugging assertions into a program.
 * The `NDEBUG` *compile-time* parameter determines whether the assumptions will
 * be actually verified by the program at *run-time*.
 *
 * In presence of `NDEBUG`, the assertion statements will be ignored and
 * therefore will have no effects on the program, not even evaluating the
 * condition. Therefore expressions passed to `assert` **must not contain
 * side-effects**, since they will not take place when debugging is disabled.
 *
 * In absence of `NDEBUG`, the assertion statements will verify that the
 * condition is met every time the program reaches that point of the program.
 *
 * If the assertion does not hold at any time, then an `#AssertionException`
 * will be thrown to indicate the programming error. This exception cannot be
 * caught whatsoever. The program (or current thread) will be terminated.
 *
 * The main advantage of using this assertion mechanism (as opposed to the
 * macros provided by the standard header file `assert.h`) is that all of the
 * pending `#finally` blocks will be executed, before actually exiting the
 * program or thread.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to using
 *     the keyword `assert`. Such programming error will lead to an abrupt exit
 *     of the program (or thread).
 *
 * @see     #AssertionException
 */
# ifndef E4C_NOKEYWORDS
# ifdef assert
	/* macro assert is already defined (probably assert.h was included) */
#	error "Please define E4C_NOKEYWORDS at compiler level " \
"in order to prevent exceptions4c from defining the assert macro."
# endif
/*@notfunction@*/
#	define assert(condition) \
		E4C_ASSERT(condition)
# endif

/**
 * Throws an exception with a formatted message
 *
 * @param   exception_type
 *          The type of exception to be thrown
 * @param   format
 *          The string containing the specifications that determine the output
 *          format for the variadic arguments
 * @param   ...
 *          The variadic arguments that will be formatted according to the
 *          format control
 *
 * This is a handy way to compose a formatted message and `#throw` an exception
 * *on the fly*:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   int bytes = 1024;
 *   void * buffer = malloc(bytes);
 *   if(buffer == NULL){
 *       throwf(NotEnoughMemoryException, "Could not allocate %d bytes.", bytes);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This macro relies on two features that were introduced in the **ISO/IEC
 * 9899:1999** (also known as *C99*) revision of the C programming language
 * standard in 1999:
 *
 *   - Variadic macros
 *   - Buffer-safe function `vsnprintf`
 *
 * In order not to create compatibility issues, this macro will only be defined
 * when the `__STDC_VERSION__` *compile-time* parameter is defined as a `long`
 * value *greater than or equal to* `199901L`, or when
 * `HAVE_C99_VARIADIC_MACROS` is defined.
 *
 * The semantics of this keyword are the same as for `throw`.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to using
 *     the keyword `throwf`. Such programming error will lead to an abrupt exit
 *     of the program (or thread).
 *   - At least one argument **must** be passed right after the format string.
 *     The message will be composed through the function `vsnprintf` with the
 *     specified format and variadic arguments. For further information on
 *     formatting rules, you may look up the specifications for the function
 *     `vsnprintf`.
 * @post
 *   - Control does not return to the `throwf` point.
 *
 * @see     #throw
 * @see     #rethrowf
 */
# if !defined(E4C_NOKEYWORDS) && defined(HAVE_C99_VARIADIC_MACROS)
#	define throwf(exception_type, format, ...) \
		\
		E4C_THROWF( (exception_type), (format), __VA_ARGS__ )
# endif

/**
 * Throws again the currently thrown exception, with a new, formatted message
 *
 * @param   format
 *          The string containing the specifications that determine the output
 *          format for the variadic arguments.
 * @param   ...
 *          The variadic arguments that will be formatted according to the
 *          format control.
 *
 * This is a handy way to create (and then `#throw`) a new instance of the
 * currently thrown exception, with a more specific, formatted message.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   try{
 *       image = read_file(file_path);
 *   }catch(FileOpenException){
 *       rethrowf("Image '%s' not found.", title);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This macro relies on two features that were introduced in the **ISO/IEC
 * 9899:1999** (also known as *C99*) revision of the C programming language
 * standard in 1999:
 *
 *   - Variadic macros
 *   - Buffer-safe function `vsnprintf`
 *
 * In order not to create compatibility issues, this macro will only be defined
 * when the `__STDC_VERSION__` *compile-time* parameter is defined as a `long`
 * value *greater than or equal to* `199901L`, or when
 * `HAVE_C99_VARIADIC_MACROS` is defined.
 *
 * The semantics of this keyword are the same as for `throw`.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to using
 *     the keyword `rethrowf`. Such programming error will lead to an abrupt
 *     exit of the program (or thread).
 *   - At least one argument **must** be passed right after the format string.
 *     The message will be composed through the function `vsnprintf` with the
 *     specified format and variadic arguments. For further information on
 *     formatting rules, you may look up the specifications for the function
 *     `vsnprintf`.
 * @post
 *   - Control does not return to the `rethrowf` point.
 *
 * @see     #rethrow
 * @see     #throwf
 */
# if !defined(E4C_NOKEYWORDS) && defined(HAVE_C99_VARIADIC_MACROS)
#	define rethrowf(format, ...) \
		\
		E4C_RETHROWF( (format), __VA_ARGS__ )
# endif


/**
 * Declares an exception type
 *
 * @param   name
 *          Name of the exception type
 *
 * This macro introduces the name of an `extern`, `const` [exception type]
 * (@ref e4c_exception_type) which will be available to be thrown or caught:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   E4C_DECLARE_EXCEPTION(StackException);
 *   E4C_DECLARE_EXCEPTION(StackOverflowException);
 *   E4C_DECLARE_EXCEPTION(StackUnderflowException);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This macro is intended to be used inside header files.
 *
 * @note
 * When you *declare* exception types, no storage is allocated. In order to
 * actually *define* them, you need to use the macro `#E4C_DEFINE_EXCEPTION`.
 *
 * @see     #e4c_exception_type
 * @see     #E4C_DEFINE_EXCEPTION
 */
# define E4C_DECLARE_EXCEPTION(name) \
	\
	extern const e4c_exception_type name

/**
 * Defines an exception type
 *
 * @param   name
 *          Name of the new exception type
 * @param   default_message
 *          Default message of the new exception type
 * @param   supertype
 *          Supertype of the new exception type
 *
 * This macro allocates a new, `const` [exception type]
 * (@ref e4c_exception_type).
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   E4C_DEFINE_EXCEPTION(StackException, "Stack exception", RuntimeException);
 *   E4C_DEFINE_EXCEPTION(StackOverflowException, "Stack overflow", StackException);
 *   E4C_DEFINE_EXCEPTION(StackUnderflowException, "Stack underflow", StackException);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This macro is intended to be used inside sorce code files. The defined
 * exception types can be *declared* in a header file through the macro
 * `#E4C_DECLARE_EXCEPTION`.
 *
 * @see     #e4c_exception_type
 * @see     #RuntimeException
 * @see     #E4C_DECLARE_EXCEPTION
 */
# define E4C_DEFINE_EXCEPTION(name, default_message, supertype) \
	\
	const e4c_exception_type name = { \
		#name, \
		default_message, \
		&supertype \
	}

/**
 * Maps a specific signal number to a given exception type
 *
 * @param   signal_number
 *          Numeric value of the signal to be converted
 * @param   exception_type
 *          Exception type representing the signal
 *
 * This macro represents a [signal mapping](@ref e4c_signal_mapping) literal. It
 * comes in handy for initializing arrays of signal mappings.
 *
 * @see     #e4c_signal_mapping
 * @see     #e4c_context_set_signal_mappings
 * @see     #e4c_context_get_signal_mappings
 * @see     #E4C_IGNORE_SIGNAL
 * @see     #E4C_NULL_SIGNAL_MAPPING
 * @see     #E4C_DECLARE_EXCEPTION
 */
# define E4C_SIGNAL_MAPPING(signal_number, exception_type) \
	\
	{signal_number, &exception_type}

/**
 * Ignores a specific signal number
 *
 * @param   signal_number
 *          Numeric value of the signal to be ignored
 *
 * This macro represents a [signal mapping](@ref e4c_signal_mapping) literal.
 * It comes in handy for initializing arrays of signal mappings.
 *
 * @see     #e4c_signal_mapping
 * @see     #e4c_context_set_signal_mappings
 * @see     #e4c_context_get_signal_mappings
 * @see     #E4C_SIGNAL_MAPPING
 * @see     #E4C_NULL_SIGNAL_MAPPING
 * @see     #E4C_DECLARE_EXCEPTION
 */
# define E4C_IGNORE_SIGNAL(signal_number) \
	\
	{signal_number, NULL}

/**
 * Represents a null signal mapping literal
 *
 * This macro represents a *null* [signal mapping](@ref e4c_signal_mapping)
 * literal. It comes in handy for terminating arrays of `#e4c_signal_mapping`.
 *
 * @see     #e4c_signal_mapping
 * @see     #e4c_context_set_signal_mappings
 * @see     #e4c_context_get_signal_mappings
 * @see     #E4C_SIGNAL_MAPPING
 * @see     #E4C_IGNORE_SIGNAL
 * @see     #E4C_DECLARE_EXCEPTION
 */
# define E4C_NULL_SIGNAL_MAPPING \
	\
	{E4C_INVALID_SIGNAL_NUMBER_, NULL}

/** @} */


/**
 * Represents an exception type in the exception handling system
 *
 * The types of the exceptions a program will use are **defined** in source code
 * files through the macro `#E4C_DEFINE_EXCEPTION`. In addition, they are
 * **declared** in header files through the macro `#E4C_DECLARE_EXCEPTION`.
 *
 * When defining types of exceptions, they are given a *name*, a *default
 * message* and a *supertype* to organize them into a *pseudo-hierarchy*:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   E4C_DEFINE_EXCEPTION(SimpleException, "Simple exception", RuntimeException);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Exceptions are defined as global objects. There is a set of predefined
 * exceptions built into the framework; `#RuntimeException` is the *root* of the
 * exceptions *pseudo-hierarchy*:
 *
 *   - `#RuntimeException`
 *     - `#NotEnoughMemoryException`
 *     - `#AssertionException`
 *     - `#IllegalArgumentException`
 *     - `#InputOutputException`
 *     - `#SignalException`
 *       - `#SignalAlarmException`
 *       - `#SignalChildException`
 *       - `#SignalTrapException`
 *       - `#ErrorSignalException`
 *         - `#IllegalInstructionException`
 *         - `#ArithmeticException`
 *         - `#BrokenPipeException`
 *         - `#BadPointerException`
 *           * `#NullPointerException`
 *       - `#ControlSignalException`
 *         - `#StopException`
 *         - `#KillException`
 *         - `#HangUpException`
 *         - `#TerminationException`
 *         - `#AbortException`
 *         - `#CPUTimeException`
 *         - `#UserControlSignalException`
 *           - `#UserQuitException`
 *           - `#UserInterruptionException`
 *           - `#UserBreakException`
 *       - `#ProgramSignalException`
 *         - `#ProgramSignal1Exception`
 *         - `#ProgramSignal2Exception`
 *
 * @see     #e4c_exception
 * @see     #E4C_DEFINE_EXCEPTION
 * @see     #E4C_DECLARE_EXCEPTION
 * @see     #throw
 * @see     #catch
 */
typedef struct e4c_exception_type_ e4c_exception_type;
struct e4c_exception_type_{

	/** The name of this exception type */
	/*@observer@*/ /*@notnull@*/
	const char *					name;

	/** The default message of this exception type */
	/*@observer@*/
	const char						default_message[E4C_EXCEPTION_MESSAGE_SIZE];

	/** The supertype of this exception type */
	/*@shared@*/ /*@notnull@*/
	const e4c_exception_type *		supertype;
};

/**
 * Represents an instance of an exception type
 *
 * Exceptions are a means of breaking out of the normal flow of control of a
 * code block in order to handle errors or other exceptional conditions. An
 * exception should be [thrown](@ref throw) at the point where the error is
 * detected; it may be [handled](@ref catch) by the surrounding code block or by
 * any code block that directly or indirectly invoked the code block where the
 * error occurred.
 *
 * Exceptions provide information regarding the exceptional situation, such as:
 *
 *   - The exception `name`
 *   - An *ad-hoc* `message` (as opposed to the *default* one)
 *   - The exact point of the program where it was thrown (source code `file`,
 *     `line` and `function` name, if available)
 *   - The value of the standard error code `errno` at the time the exception
 *     was thrown
 *   - The `cause` of the exception, which is the previous exception (if any),
 *     when the exception was thrown
 *   - The specific `type` of the exception, convenient when handling an
 *     abstract type of exceptions from a `#catch` block
 *   - Optional, *user-defined*, `custom_data`, which can be initialized and
 *     finalized throught context *handlers*
 *
 * @note
 * **Any** exception can be caught by a block introduced by
 * `catch(RuntimeException)`, **except for `#AssertionException`**.
 *
 * @see     #e4c_exception_type
 * @see     #throw
 * @see     #catch
 * @see     #e4c_get_exception
 * @see     #e4c_context_set_handlers
 * @see     #RuntimeException
 * @see     #AssertionException
 */
typedef struct e4c_exception_ e4c_exception;
struct e4c_exception_{

	/* This field is undocumented on purpose and reserved for internal use */
	int								_;

	/** The name of this exception */
	/*@observer@*/ /*@notnull@*/
	const char *					name;

	/** The message of this exception */
	/*@exposed@*/
	char							message[E4C_EXCEPTION_MESSAGE_SIZE];

	/** The path of the source code file from which the exception was thrown */
	/*@observer@*/ /*@null@*/
	const char *					file;

	/** The number of line from which the exception was thrown */
	int								line;

	/** The function from which the exception was thrown */
	/*@observer@*/ /*@null@*/
	const char *					function;

	/** The value of errno at the time the exception was thrown */
	int								error_number;

	/** The type of this exception */
	/*@shared@*/ /*@notnull@*/
	const e4c_exception_type *		type;

	/** The cause of this exception */
	/*@only@*/ /*@null@*/
	e4c_exception *					cause;

	/** Custom data associated to this exception */
	/*@shared@*/ /*@null@*/
	void *							custom_data;
};

/**
 * Represents a map between a signal and an exception
 *
 * A signal is an asynchronous notification sent by the operating system to a
 * process in order to notify it of an event that occurred. Most of the signals
 * will, by default, crash the program as soon as they are raised.
 * **exceptions4c** can convert signals to exceptions, so they can be easily
 * handled.
 *
 * For example, a *suspicious* or *dangerous* part of the program could be
 * wrapped up with `#try` blocks and then `#catch` *segmentation faults* or
 * *divisions by zero*. Then the program would clean up and continue normally:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   e4c_using_context(E4C_TRUE){
 *       int * pointer = NULL;
 *       try{
 *           int oops = *pointer;
 *       }catch(BadPointerException){
 *           printf("No problem ;-)");
 *       }finally{
 *           // clean up...
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * In order to perform the conversion, **exceptions4c** *maps* signals to
 * exceptions.
 *
 * The simplest way to get this working is by calling the function
 * `#e4c_context_begin`. This function will set up the [default mappings]
 * (@ref e4c_default_signal_mappings) for the available signals in the platform,
 * when passed `handle_signals=true`.
 *
 * If you need to be more specific about which signals get converted to
 * exceptions, you can define an array of [signal mappings]
 * (@ref e4c_signal_mapping) and then pass it to the function
 * `#e4c_context_set_signal_mappings`.
 *
 * An array of signal mappings is defined through three macros:
 *
 *   - `#E4C_SIGNAL_MAPPING`
 *   - `#E4C_IGNORE_SIGNAL`
 *   - `#E4C_NULL_SIGNAL_MAPPING`
 *
 * While `E4C_SIGNAL_MAPPING` tells the system to convert a specific signal to a
 * given exception, `E4C_IGNORE_SIGNAL` allows you to disregard the signal and
 * continue (even if unmeaningful).
 *
 * Every array of signal mappings **needs** to be terminated with the
 * `E4C_NULL_SIGNAL_MAPPING` element, so the system finds out how many mappings
 * are there in a given array.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   const e4c_signal_mapping my_signal_mappings[] = {
 *       E4C_SIGNAL_MAPPING(SIGABRT, Exception1),
 *       E4C_SIGNAL_MAPPING(SIGINT, Exception2),
 *       E4C_IGNORE_SIGNAL(SIGTERM),
 *       ...
 *       E4C_NULL_SIGNAL_MAPPING
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Once the array is properly defined, it can be passed to the function
 * `e4c_context_set_signal_mappings`. This way, only the specified signals will
 * be handled as exceptions, and they will be converted to the specified
 * exceptions.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   e4c_using_context(E4C_FALSE){
 *
 *       e4c_context_set_signal_mappings(my_signal_mappings);
 *       ...
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * These are some of the signals you can handle:
 *
 *   - `SIGFPE`  when dividing by *zero*.
 *   - `SIGSEGV` when dereferencing an invalid pointer.
 *   - `SIGINT`  when a user interrupts the process.
 *   - `SIGTERM` when a process is requested to be terminated.
 *
 * @see     #e4c_context_begin
 * @see     #e4c_context_set_signal_mappings
 * @see     #e4c_context_get_signal_mappings
 * @see     #E4C_SIGNAL_MAPPING
 * @see     #E4C_IGNORE_SIGNAL
 * @see     #e4c_default_signal_mappings
 */
typedef struct e4c_signal_mapping_ e4c_signal_mapping;
struct e4c_signal_mapping_{

	/** The signal to be converted */
	int									signal_number;

	/** The exception representing the signal */
	/*@dependent@*/ /*@null@*/
	const e4c_exception_type * const	exception_type;

};

/**
 * Represents the completeness of a code block aware of exceptions
 *
 * The symbolic values representing the status of a block help to distinguish
 * between different possible situations inside a `#finally` block. For example,
 * different cleanup actions can be taken, depending on the status of the block.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   try{
 *      ...
 *   }finally{
 *      switch( e4c_get_status() ){
 *
 *          case e4c_succeeded:
 *              ...
 *              break;
 *
 *          case e4c_recovered:
 *              ...
 *              break;
 *
 *          case e4c_failed:
 *              ...
 *              break;
 *      }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @see     #e4c_get_status
 * @see     #finally
 */
enum e4c_status_{

	/** There were no exceptions */
	e4c_succeeded,

	/** There was an exception, but it was caught */
	e4c_recovered,

	/** There was an exception and it wasn't caught */
	e4c_failed
};
typedef enum e4c_status_ e4c_status;

/**
 * Represents a function which will be executed in the event of an uncaught
 * exception.
 *
 * @param   exception
 *          The uncaught exception
 *
 * This handler can be set through the function `#e4c_context_set_handlers`:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   void my_uncaught_handler(const e4c_exception * exception){
 *
 *       printf("Error: %s (%s)\n", exception->name, exception->message);
 *   }
 *
 *   int main(int argc, char * argv[]){
 *
 *       e4c_using_context(E4C_TRUE){
 *
 *           e4c_context_set_handlers(my_uncaught_handler, NULL, NULL, NULL);
 *           // ...
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * There exists a convenience function `#e4c_print_exception` which is used as
 * the default *uncaught handler*, unless otherwise specified. It simply prints
 * information regarding the exception to `stderr`.
 *
 * @warning
 * An uncaught handler is not allowed to try and recover the current exception
 * context. Moreover, the program (or current thread) will terminate right after
 * the function returns.
 *
 * @see     #e4c_context_set_handlers
 * @see     #e4c_initialize_handler
 * @see     #e4c_finalize_handler
 * @see     #e4c_print_exception
 */
typedef void (*e4c_uncaught_handler)(const e4c_exception * exception)
/*@globals
	fileSystem,
	internalState
@*/
/*@modifies
	fileSystem,
	internalState
*/
;

/**
 * Represents a function which will be executed whenever a new exception is
 * thrown.
 *
 * @param   exception
 *          The newly thrown exception
 *
 * When this handler is set, it will be called any time a new exception is
 * created. The `void` pointer returned by this function will be assigned to
 * the exception's *custom_data*. This data can be accessed later on, for
 * example, from a `#catch` block, or an *uncaught handler*, for any
 * specific purpose.
 *
 * This handler can be set through the function `#e4c_context_set_handlers`:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   void * my_initialize_handler(const e4c_exception * e){
 *
 *       if( e4c_is_instance_of(e, &SignalException) ){
 *           printf("Program received signal %s (%d)!\n", e->file, e->line);
 *       }
 *
 *       return(NULL);
 *   }
 *
 *   int main(int argc, char * argv[]){
 *
 *       e4c_using_context(E4C_TRUE){
 *
 *           e4c_context_set_handlers(NULL, NULL, my_initialize_handler, NULL);
 *           // ...
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * By the time this handler is called, the exception already has been assigned
 * the initial value specified for `custom_data`, so the handler may make use
 * of it:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   void * log_handler(const e4c_exception * e){
 *
 *       printf("LOG: Exception thrown at module '%s'\n", e->custom_data);
 *
 *       return(NULL);
 *   }
 *
 *   int main(int argc, char * argv[]){
 *
 *       e4c_using_context(E4C_TRUE){
 *
 *           e4c_context_set_handlers(NULL, "FOO", log_handler, NULL);
 *           // ...
 *       }
 *
 *       e4c_using_context(E4C_TRUE){
 *
 *           e4c_context_set_handlers(NULL, "BAR", log_handler, NULL);
 *           // ...
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @see     #e4c_context_set_handlers
 * @see     #e4c_exception
 * @see     #e4c_finalize_handler
 * @see     #e4c_uncaught_handler
 * @see     #e4c_print_exception
 */
typedef void * (*e4c_initialize_handler)(const e4c_exception * exception)
/*@globals
	fileSystem,
	internalState
@*/
/*@modifies
	fileSystem,
	internalState
*/
;

/**
 * Represents a function which will be executed whenever an exception is
 * destroyed.
 *
 * @param   custom_data
 *          The "custom data" of the exception to be discarded
 *
 * When this handler is set, it will be called any time an exception is
 * discarded. It will be passed the *custom_data* of the exception, so it may
 * dispose resources previously acquired by the *initialize handler*.
 *
 * This handler can be set through the function `#e4c_context_set_handlers`:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   void * initialize_data(const e4c_exception * exception){
 *
 *       const char * custom_data = malloc(1024);
 *
 *       if(custom_data != NULL){
 *           if( e4c_is_instance_of(exception, &SignalException) ){
 *               strcpy(custom_data, "SIGNAL ERROR");
 *           }else{
 *               strcpy(custom_data, "RUNTIME ERROR");
 *           }
 *       }
 *
 *       return(custom_data);
 *   }
 *
 *   void finalize_data(void * custom_data){
 *
 *       free(custom_data);
 *   }
 *
 *   int main(int argc, char * argv[]){
 *
 *       e4c_using_context(E4C_TRUE){
 *
 *           e4c_context_set_handlers(NULL, NULL, initialize_data, finalize_data);
 *           ...
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @see     #e4c_context_set_handlers
 * @see     #e4c_exception
 * @see     #e4c_finalize_handler
 * @see     #e4c_uncaught_handler
 * @see     #e4c_print_exception
 */
typedef void (*e4c_finalize_handler)(void * custom_data)
/*@globals
	fileSystem,
	internalState
@*/
/*@modifies
	fileSystem,
	internalState
*/
;

/*
 * Next types are undocumented on purpose, in order to hide implementation
 * details, subject to change.
 */
enum e4c_frame_stage_{
	e4c_beginning_,
	e4c_acquiring_,
	e4c_trying_,
	e4c_disposing_,
	e4c_catching_,
	e4c_finalizing_,
	e4c_done_
};

struct e4c_continuation_{
	/*@partial@*/ /*@dependent@*/
	E4C_CONTINUATION_BUFFER_		buffer;
};

/**
 * @name Predefined signal mappings
 *
 * There is a predefined set of signal mappings. Signal mappings are used to
 * convert signals into exceptions.
 *
 * Common signals are mapped to its corresponding exception, for example:
 *
 *   - `SIGABRT` is mapped to `#AbortException`
 *   - `SIGFPE`  is mapped to `#ArithmeticException`
 *   - `SIGSEGV` is mapped to `#BadPointerException`
 *   - `SIGTERM` is mapped to `#TerminationException`
 *   - ...and so on
 *
 * @see     #e4c_signal_mapping
 * @see     #e4c_context_begin
 * @see     #e4c_context_set_signal_mappings
 * @see     #e4c_context_get_signal_mappings
 * @{
 */

/**
 * The array of predefined signal mappings.
 */
/*@unused@*/ /*@notnull@*/
extern const e4c_signal_mapping * const e4c_default_signal_mappings;

/** @} */

/**
 * @name Predefined exceptions
 *
 * Built-in exceptions represent usual error conditions that might occur during
 * the course of any program.
 *
 * They are organized into a *pseudo-hierarchy*; `#RuntimeException` is the
 * common *supertype* of all exceptions.
 *
 * @{
 */

/**
 * This is the root of the exception *pseudo-hierarchy*
 *
 * `#RuntimeException` is the common *supertype* of all exceptions.
 *
 * @par     Direct known subexceptions:
 *          #NotEnoughMemoryException,
 *          #AssertionException,
 *          #IllegalArgumentException,
 *          #InputOutputException,
 *          #SignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(RuntimeException);

/**
 * This exception is thrown when the system runs out of memory
 *
 * `#NotEnoughMemoryException` is thrown when there is not enough memory to
 * continue the execution of the program.
 *
 * @par     Extends:
 *          #RuntimeException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(NotEnoughMemoryException);

/**
 * This exception is thrown when a function is passed an illegal or
 * inappropriate argument
 *
 * `#IllegalArgumentException` is thrown by a function when it detects that some
 * of the function parameters (passed by the caller) is unacceptable.
 *
 * @par     Extends:
 *          #RuntimeException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(IllegalArgumentException);

/**
 * This exception is thrown when an assertion does not hold
 *
 * `#AssertionException` is part of the assertion facility of the library. It is
 * thrown when the *compile-time* parameter `NDEBUG` is present and the
 * conditon of an assertion evaluates to `false`.
 *
 * @remark
 * This exception cannot be caught whatsoever. The program (or current thread)
 * will be terminated, after the execution of all pending `#finally` blocks.
 *
 * @see     #assert
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(AssertionException);

/**
 * This exception is thrown when an input/output error occurs
 *
 * `#InputOutputException` is the general type of exceptions produced by failed
 * or interrupted I/O operations.
 *
 * @par     Extends:
 *          #RuntimeException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(InputOutputException);

/**
 * This exception is the common supertype of all signal exceptions
 *
 * Signal exceptions are thrown when some signal is sent to the process.
 *
 * A signal can be generated by calling `raise`.
 *
 * @par     Extends:
 *          #RuntimeException
 *
 * @par     Direct known subexceptions:
 *          #SignalException,
 *          #SignalAlarmException,
 *          #SignalChildException,
 *          #SignalTrapException,
 *          #ErrorSignalException,
 *          #ControlSignalException,
 *          #ProgramSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(SignalException);

/**
 * This exception is thrown when a time limit has elapsed
 *
 * `#SignalAlarmException` represents `SIGALRM`, the signal sent to a process
 * when a time limit has elapsed.
 *
 * @par     Extends:
 *          #SignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(SignalAlarmException);

/**
 * This exception is thrown when a child process terminates
 *
 * `#SignalChildException` represents `SIGCHLD`, the signal sent to a process
 * when a child process terminates.
 *
 * @par     Extends:
 *          #SignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(SignalChildException);

/**
 * This exception is thrown when a condition arises that a debugger has
 * requested to be informed of
 *
 * `#SignalTrapException` represents `SIGTRAP`, the signal sent to a process
 * when a condition arises that a debugger has requested to be informed of.
 *
 * @par     Extends:
 *          #SignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(SignalTrapException);

/**
 * This exception is the common supertype of all error signal exceptions
 *
 * Error signal exceptions are thrown when some error prevents the program to
 * keep executing its normal flow.
 *
 * @par     Extends:
 *          #SignalException
 *
 * @par     Direct known subexceptions:
 *          #IllegalInstructionException,
 *          #BadPointerException,
 *          #ArithmeticException,
 *          #BrokenPipeException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(ErrorSignalException);

/**
 * This exception is thrown when the process attempts to execute an illegal
 * instruction
 *
 * `#IllegalInstructionException` represents `SIGILL`, the signal sent to a
 * process when it attempts to execute a malformed, unknown, or privileged
 * instruction.
 *
 * @par     Extends:
 *          #ErrorSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(IllegalInstructionException);

/**
 * This exception is thrown when the process performs an erroneous arithmetic
 * operation
 *
 * `#ArithmeticException` represents `SIGFPE`, the signal sent to a process
 * when it performs an erroneous arithmetic operation.
 *
 * @par     Extends:
 *          #ErrorSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(ArithmeticException);

/**
 * This exception is thrown when the process attempts to write to a broken pipe
 *
 * `#BrokenPipeException` represents `SIGPIPE`, the signal sent to a process
 * when it attempts to write to a pipe without a process connected to the other
 * end.
 *
 * @par     Extends:
 *          #ErrorSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(BrokenPipeException);

/**
 * This exception is thrown when the process tries to dereference an invalid
 * pointer
 *
 * `#BadPointerException` represents `SIGSEGV`, the signal sent to a process
 * when it makes an invalid memory reference, or segmentation fault.
 *
 * @par     Extends:
 *          #ErrorSignalException
 *
 * @par     Direct known subexceptions:
 *          #NullPointerException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(BadPointerException);

/**
 * This exception is thrown when an unexpected null pointer is found
 *
 * `#NullPointerException` is thrown when some part of the program gets a
 * pointer which was expected or required to contain a valid memory address.
 *
 * A *null* pointer exception is a special case of a *bad* pointer exception.
 * The difference between them is that `#NullPointerException` needs to be
 * thrown *explicitly* by some function, while `#BadPointerException` is
 * thrown *implicitly* by the signal handling system (if enabled).
 *
 * @note
 * Sometimes, a null pointer exception can also be considered as a special case
 * of an *illegal argument* exception.
 *
 * @par     Extends:
 *          #BadPointerException
 *
 * @see     #IllegalArgumentException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(NullPointerException);

/**
 * This exception is the common supertype of all control signal exceptions
 *
 * Control signal exceptions are thrown when the process needs to be controlled
 * by the user or some other process.
 *
 * @par     Extends:
 *          #SignalException
 *
 * @par     Direct known subexceptions:
 *          #StopException,
 *          #KillException,
 *          #HangUpException,
 *          #TerminationException,
 *          #AbortException,
 *          #CPUTimeException,
 *          #UserControlSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(ControlSignalException);

/**
 * This exception is thrown to stop the process for later resumption
 *
 * `#StopException` represents `SIGSTOP`, the signal sent to a process to stop
 * it for later resumption.
 *
 * @remark
 * Since `SIGSTOP` is usually unblock-able, it won't be handled and converted
 * to this exception automatically on some platforms.
 *
 * @par     Extends:
 *          #ControlSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(StopException);

/**
 * This exception is thrown to terminate the process immediately
 *
 * `#KillException` represents `SIGKILL`, the signal sent to a process to
 * cause it to terminate immediately.
 *
 * @remark
 * Since `SIGKILL` is usually unblock-able, it won't be handled and converted
 * to this exception automatically on some platforms.
 *
 * @par     Extends:
 *          #ControlSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(KillException);

/**
 * This exception is thrown when the process' terminal is closed
 *
 * `#HangUpException` represents `SIGHUP`, the signal sent to a process when
 * its controlling terminal is closed.
 *
 * @par     Extends:
 *          #ControlSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(HangUpException);

/**
 * This exception is thrown to request the termination of the process
 *
 * `#TerminationException` represents `SIGTERM`, the signal sent to a process
 * to request its termination.
 *
 * @par     Extends:
 *          #ControlSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(TerminationException);

/**
 * This exception is thrown to abort the process
 *
 * `#AbortException` represents `SIGABRT`, the signal sent by computer
 * programs to abort the process.
 *
 * @par     Extends:
 *          #ControlSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(AbortException);

/**
 * This exception is thrown when the process has used up the CPU for too long
 *
 * `#CPUTimeException` represents `SIGXCPU`, the signal sent to a process when
 * it has used up the CPU for a duration that exceeds a certain predetermined
 * user-settable value.
 *
 * @par     Extends:
 *          #ControlSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(CPUTimeException);

/**
 * This exception is the common supertype of all control signal exceptions
 * caused by the user
 *
 * User control signal exceptions are thrown when the process needs to be
 * controlled by the user.
 *
 * @par     Extends:
 *          #ControlSignalException
 *
 * @par     Direct known subexceptions:
 *          #UserQuitException,
 *          #UserInterruptionException,
 *          #UserBreakException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(UserControlSignalException);

/**
 * This exception is thrown when the user requests to quit the process
 *
 * `#UserQuitException` represents `SIGQUIT`, the signal sent to a process by
 * its controlling terminal when the user requests that the process dump core.
 *
 * @par     Extends:
 *          #UserControlSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(UserQuitException);

/**
 * This exception is thrown when the user requests to interrupt the process
 *
 * `#UserInterruptionException` represents `SIGINT`, the signal sent to a
 * process by its controlling terminal when a user wishes to interrupt it.
 *
 * @par     Extends:
 *          #UserControlSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(UserInterruptionException);

/**
 * This exception is thrown when a user wishes to break the process
 *
 * `#UserBreakException` represents `SIGBREAK`, the signal sent to a process
 * by its controlling terminal when a user wishes to break it.
 *
 * @par     Extends:
 *          #UserControlSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(UserBreakException);

/**
 * This exception is the common supertype of all user-defined signal exceptions
 *
 * User-defined signal exceptions are thrown to indicate user-defined
 * conditions.
 *
 * @par     Extends:
 *          #SignalException
 *
 * @par     Direct known subexceptions:
 *          #ProgramSignal1Exception,
 *          #ProgramSignal2Exception
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(ProgramSignalException);

/**
 * This exception is thrown when user-defined conditions occur
 *
 * `#ProgramSignal1Exception` represents `SIGUSR1`, the signal sent to a
 * process to indicate user-defined conditions.
 *
 * @par     Extends:
 *          #ProgramSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(ProgramSignal1Exception);

/**
 * This exception is thrown when user-defined conditions occur
 *
 * `#ProgramSignal2Exception` represents `SIGUSR1`, the signal sent to a
 * process to indicate user-defined conditions.
 *
 * @par     Extends:
 *          #ProgramSignalException
 */
/*@unused@*/
E4C_DECLARE_EXCEPTION(ProgramSignal2Exception);

/** @} */

/**
 * @name Exception context handling functions
 *
 * These functions enclose the scope of the exception handling system and
 * retrieve the current exception context.
 *
 * @{
 */

/**
 * Checks if the current exception context is ready
 *
 * @return  Whether the current exception context of the program (or current
 *          thread) is ready to be used.
 *
 * This function returns whether there is an actual exception context ready to
 * be used by the program or current thread.
 *
 * @see     #e4c_context_begin
 * @see     #e4c_context_end
 * @see     #e4c_using_context
 * @see     #e4c_reusing_context
 */
/*@unused@*/ extern
E4C_BOOL
e4c_context_is_ready(
	void
)
# ifdef E4C_THREADSAFE
/*@globals
	fileSystem,
	internalState
*/
/*@modifies
	fileSystem,
	internalState
*/
# else
/*@globals
	internalState
@*/
# endif
;

/**
 * Begins an exception context
 *
 * @param   handle_signals
 *          If `true`, the signal handling system will be set up with the
 *          default mapping.
 *
 * This function begins the current exception context to be used by the program
 * (or current thread), until `#e4c_context_end` is called.
 *
 * The signal handling system can be initialized automatically with the
 * [default signal mappings](@ref e4c_default_signal_mappings) by passing
 * `handle_signals=true`. This is equivalent to:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   e4c_context_set_signal_mappings(e4c_default_signal_mappings);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @warning
 * Note that the behavior of the standard `signal` function might be undefined
 * for a multithreaded program, so use the signal handling system with caution.
 *
 * The convenience function `#e4c_print_exception` will be used as the default
 * *uncaught handler*. It will be called in the event of an uncaught exception,
 * before exiting the program or thread. This handler can be set through the
 * function `#e4c_context_set_handlers`.
 *
 * @pre
 *   - Once `e4c_context_begin` is called, the program (or thread) **must** call
 *     `e4c_context_end` before exiting.
 *   - Calling `e4c_context_begin` *twice in a row* is considered a programming
 *     error, and therefore the program (or thread) will exit abruptly.
 *     Nevertheless, `#e4c_context_begin` can be called several times *if, and
 *     only if*, `e4c_context_end` is called in between.
 *
 * @see     #e4c_context_end
 * @see     #e4c_context_is_ready
 * @see     #e4c_using_context
 * @see     #e4c_reusing_context
 * @see     #e4c_uncaught_handler
 * @see     #e4c_print_exception
 * @see     #e4c_context_set_handlers
 */
/*@unused@*/ extern
void
e4c_context_begin(
	E4C_BOOL					handle_signals
)
/*@globals
	fileSystem,
	internalState,

	e4c_default_signal_mappings,

	NotEnoughMemoryException
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/**
 * Ends the current exception context
 *
 * This function ends the current exception context.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to
 *     calling `e4c_context_end`. Such programming error will lead to an abrupt
 *     exit of the program (or thread).
 *
 * @see     #e4c_context_begin
 * @see     #e4c_context_is_ready
 * @see     #e4c_using_context
 * @see     #e4c_reusing_context
 */
/*@unused@*/ extern
void
e4c_context_end(
	void
)
/*@globals
	fileSystem,
	internalState
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/**
 * Sets the optional handlers of an exception context
 *
 * @param   uncaught_handler
 *          The function to be executed in the event of an uncaught exception
 * @param   custom_data
 *          The initial value assigned to the custom_data of a new exception
 * @param   initialize_handler
 *          The function to be executed whenever a new exception is thrown
 * @param   finalize_handler
 *          The function to be executed whenever an exception is destroyed
 *
 * These handlers are a means of customizing the behavior of the exception
 * system. For example, you can specify what needs to be done when a thrown
 * exception is not caught (and thus, the program or thread is about to end) by
 * calling `e4c_context_set_handlers` with your own [uncaught handler]
 * (@ref e4c_uncaught_handler).
 *
 * You can also control the [custom data](@ref e4c_exception::custom_data)
 * attached to any new exception by specifying any or all of these:
 *
 *   - The *initial value* to be assigned to the `custom_data`
 *   - The function to *initialize* the `custom_data`
 *   - The function to *finalize* the `custom_data`
 *
 * When these handlers are defined, they will be called anytime an exception is
 * uncaught, created or destroyed. You can use them to meet your specific needs.
 * For example, you could...
 *
 *   - ...send an e-mail whenever an exception is uncaught
 *   - ...log any thrown exception to a file
 *   - ...capture the call stack in order to print it later on
 *   - ...go for something completely different ;)
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to
 *     calling `e4c_context_set_handlers`. Such programming error will lead to
 *     an abrupt exit of the program (or thread).
 *
 * @see     #e4c_uncaught_handler
 * @see     #e4c_initialize_handler
 * @see     #e4c_finalize_handler
 * @see     #e4c_exception
 * @see     #e4c_print_exception
 */
/*@unused@*/ extern
void
e4c_context_set_handlers(
	/*@dependent@*/ /*@null@*/
	e4c_uncaught_handler uncaught_handler,
	/*@dependent@*/ /*@null@*/
	void * custom_data,
	/*@dependent@*/ /*@null@*/
	e4c_initialize_handler initialize_handler,
	/*@dependent@*/ /*@null@*/
	e4c_finalize_handler finalize_handler
)/*@globals
	fileSystem,
	internalState
@*/
/*@modifies
	fileSystem,
	internalState
@*/;

/**
 * Assigns the specified signal mappings to the exception context
 *
 * @param   mappings
 *          The array of mappings
 *
 * This function assigns an array of mappings between the signals to be handled
 * and the corresponding exception to be thrown.
 *
 * @warning
 * Note that the behavior of the standard `signal` function might be undefined
 * for a multithreaded program, so use the signal handling system with caution.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to
 *     calling `e4c_context_set_signal_mappings`. Such programming error will
 *     lead to an abrupt exit of the program (or thread).
 *   - `mappings` **must** be terminated by `#E4C_NULL_SIGNAL_MAPPING`.
 *
 * @see     #e4c_context_get_signal_mappings
 * @see     #e4c_signal_mapping
 * @see     #e4c_default_signal_mappings
 */
/*@unused@*/ extern
void
e4c_context_set_signal_mappings(
	/*@dependent@*/ /*@null@*/
	const e4c_signal_mapping * mappings
)
/*@globals
	fileSystem,
	internalState
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/**
 * Retrieves the signal mappings for the current exception context
 *
 * @return  The current array of mappings
 *
 * This function retrieves the current array of mappings between the signals to
 * be handled and the corresponding exception to be thrown.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to
 *     calling `e4c_context_get_signal_mappings`. Such programming error will
 *     lead to an abrupt exit of the program (or thread).
 *
 * @see     #e4c_context_set_signal_mappings
 * @see     #e4c_signal_mapping
 * @see     #e4c_default_signal_mappings
 */
/*@unused@*/ extern
/*@observer@*/ /*@null@*/
const e4c_signal_mapping *
e4c_context_get_signal_mappings(
	void
)
/*@globals
	fileSystem,
	internalState
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/**
 * Returns the completeness status of the executing code block
 *
 * @return  The completeness status of the executing code block
 *
 * Exception-aware code blocks have a completeness status regarding the
 * exception handling system. This status determines whether an exception was
 * actually thrown or not, and whether the exception was caught or not.
 *
 * The status of the current block can be obtained any time, provided that the
 * exception context has begun at the time of the function call. However, it is
 * sensible to call this function only during the execution of `#finally`
 * blocks.
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to
 *     calling `e4c_get_status`. Such programming error will lead to an abrupt
 *     exit of the program (or thread).
 *
 * @see     #e4c_status
 * @see     #finally
 */
/*@unused@*/ extern
e4c_status
e4c_get_status(
	void
)
/*@globals
	fileSystem,
	internalState
@*/
/*@modifies
	fileSystem,
	internalState
@*/;

/**
 * Returns the exception that was thrown
 *
 * @return  The exception that was thrown in the current exception context (if
 *          any) otherwise `NULL`
 *
 * This function returns a pointer to the exception that was thrown in the
 * surrounding *exception-aware* block, if any; otherwise `NULL`.
 *
 * The function `#e4c_is_instance_of` will determine if the thrown exception is
 * an instance of any of the defined exception types. The `type` of the thrown
 * exception can also be compared for an exact type match.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   try{
 *      ...
 *   }catch(RuntimeException){
 *      const e4c_exception * exception = e4c_get_exception();
 *      if( e4c_is_instance_of(exception, &SignalException) ){
 *          ...
 *      }else if(exception->type == &NotEnoughMemoryException){
 *          ...
 *      }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * The thrown exception can be obtained any time, provided that the exception
 * context has begun at the time of the function call. However, it is sensible
 * to call this function only during the execution of `#finally` or `#catch`
 * blocks.
 *
 * Moreover, a pointer to the thrown exception obtained *inside* a `#finally`
 * or `#catch` block **must not** be used *outside* these blocks.
 *
 * The exception system objects are dinamically allocated and deallocated, as
 * the program enters or exits `#try`... `#catch`... `#finally` blocks. While
 * it would be legal to *copy* the thrown exception and access to its `name`
 * and `message` outside these blocks, care should be taken in order not to
 * dereference the `cause` of the exception, unless it is a **deep copy**
 * (as opposed to a **shallow copy**).
 *
 * @pre
 *   - A program (or thread) **must** begin an exception context prior to
 *     calling `e4c_get_exception`. Such programming error will lead to an
 *     abrupt exit of the program (or thread).
 *
 * @see     #e4c_exception
 * @see     #e4c_is_instance_of
 * @see     #throw
 * @see     #catch
 * @see     #finally
 */
/*@unused@*/ extern
/*@observer@*/ /*@relnull@*/
const e4c_exception *
e4c_get_exception(
	void
)
/*@globals
	fileSystem,
	internalState
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/** @} */

/**
 * @name Other integration and convenience functions
 *
 * @{
 */

/**
 * Gets the library version number
 *
 * @return  The version number associated with the library
 *
 * This function provides the same information as the `#E4C_VERSION_NUMBER`
 * macro, but the returned version number is associated with the actual,
 * compiled library.
 *
 * @note
 * This version number can be considered as the *run-time* library version
 * number, as opposed to the *compile-time* library version number (specified by
 * the header file).
 *
 * @remark
 * The library **must** be compiled with the corresponding header (i.e. library
 * version number should be equal to header version number).
 *
 * @see     #E4C_VERSION_NUMBER
 */
/*@unused@*/ extern
long
e4c_library_version(
	void
)
/*@*/
;

/**
 * Returns whether an exception instance is of a given type
 *
 * @param   instance
 *          The thrown exception
 * @param   exception_type
 *          A previously defined type of exceptions
 * @return  Whether the specified exception is an instance of the given type
 *
 * `#e4c_is_instance_of` can be used to determine if a thrown exception **is an
 * instance of** a given exception type.
 *
 * This function is intended to be used in a `#catch` block, or in a `#finally`
 * block provided that some exception was actually thrown (i.e.
 * `#e4c_get_status` returs `#e4c_failed` or `#e4c_recovered`).
 *
 * This function will return `false` if either `instance` or `type` are
 * `NULL`.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *   try{
 *      ...
 *   }catch(RuntimeException){
 *      const e4c_exception * exception = e4c_get_exception();
 *      if( e4c_is_instance_of(exception, &SignalException) ){
 *          ...
 *      }else if(exception->type == &NotEnoughMemoryException){
 *          ...
 *      }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @pre
 *   - `instance` **must not** be `NULL`
 *   - `type` **must not** be `NULL`
 *
 * @see     #e4c_exception
 * @see     #e4c_exception_type
 * @see     #e4c_get_exception
 */
/*@unused@*/ extern
E4C_BOOL
e4c_is_instance_of(
	/*@temp@*/ /*@notnull@*/
	const e4c_exception *		instance,
	/*@temp@*/ /*@notnull@*/
	const e4c_exception_type *	exception_type
)
/*@*/
;

/**
 * Prints a fatal error message regarding the specified exception
 *
 * @param   exception
 *          The uncaught exception
 *
 * This is a convenience function for showing an error message through the
 * standard error output. It can be passed to `#e4c_context_set_handlers` as
 * the handler for uncaught exceptions. Will be used by default, unless
 * otherwise set up.
 *
 * In absence of `NDEBUG`, this function prints as much information regarding
 * the exception as it is available, whereas in presence of `NDEBUG`, only the
 * `name` and `message` of the exception are printed.
 *
 * @pre
 *   - `exception` **must not** be `NULL`
 * @throws  #NullPointerException
 *          If `exception` is `NULL`
 *
 * @see     #e4c_uncaught_handler
 * @see     #e4c_context_begin
 * @see     #e4c_using_context
 */
/*@unused@*/ extern
void
e4c_print_exception(
	/*@temp@*/ /*@notnull@*/
	const e4c_exception *		exception
)
/*@globals
	fileSystem,
	internalState,

	NotEnoughMemoryException,
	NullPointerException
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/**
 * Prints an ASCII graph representing an exception type's hierarchy
 *
 * @param   exception_type
 *          An exception type
 *
 * This is a convenience function for showing an ASCII graph representing an
 * exception type's hierarchy through the standard error output.
 *
 * For example, the output for `#ProgramSignal2Exception` would be:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *   Exception hierarchy
 *   ________________________________________________________________
 *
 *       RuntimeException
 *        |
 *        +--SignalException
 *            |
 *            +--ProgramSignalException
 *                |
 *                +--ProgramSignal2Exception
 *   ________________________________________________________________
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @pre
 *   - `exception_type` **must not** be `NULL`
 * @throws  #NullPointerException
 *          If `exception_type` is `NULL`
 *
 * @see     #e4c_exception_type
 */
/*@unused@*/ extern
void
e4c_print_exception_type(
	/*@shared@*/ /*@notnull@*/
	const e4c_exception_type *	exception_type
)
/*@globals
	fileSystem,
	internalState,

	NotEnoughMemoryException,
	NullPointerException
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/** @} */

/*
 * Next functions are undocumented on purpose, because they shouldn't be used
 * directly (but through the "keywords").
 */

/*@unused@*/ extern
/*@notnull@*/ /*@temp@*/
struct e4c_continuation_ *
e4c_frame_first_stage_(
	enum e4c_frame_stage_		stage,
	/*@observer@*/ /*@null@*/
	const char *				file,
	int							line,
	/*@observer@*/ /*@null@*/
	const char *				function
)
/*@globals
	fileSystem,
	internalState,

	NotEnoughMemoryException
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/*@unused@*/ extern
E4C_BOOL
e4c_frame_next_stage_(
	void
)
/*@globals
	fileSystem,
	internalState,

	AssertionException
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/*@unused@*/ extern
enum e4c_frame_stage_
e4c_frame_get_stage_(
	/*@observer@*/ /*@null@*/
	const char *				file,
	int							line,
	/*@observer@*/ /*@null@*/
	const char *				function
)
/*@globals
	fileSystem,
	internalState
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/*@unused@*/ extern
E4C_BOOL
e4c_frame_catch_(
	/*@temp@*/ /*@null@*/
	const e4c_exception_type *	exception_type,
	/*@observer@*/ /*@null@*/
	const char *				file,
	int							line,
	/*@observer@*/ /*@null@*/
	const char *				function
)
/*@globals
	fileSystem,
	internalState,

	NullPointerException
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/*@unused@*/ /*@maynotreturn@*/ extern
void
e4c_frame_repeat_(
	int							max_repeat_attempts,
	enum e4c_frame_stage_		stage,
	/*@observer@*/ /*@null@*/
	const char *				file,
	int							line,
	/*@observer@*/ /*@null@*/
	const char *				function
)
/*@globals
	fileSystem,
	internalState,

	AssertionException
@*/
/*@modifies
	fileSystem,
	internalState
@*/
;

/*@unused@*/ /*@noreturn@*/ extern
void
e4c_exception_throw_verbatim_(
	/*@shared@*/ /*@notnull@*/
	const e4c_exception_type *	exception_type,
	/*@observer@*/ /*@null@*/
	const char *				file,
	int							line,
	/*@observer@*/ /*@null@*/
	const char *				function,
	/*@observer@*/ /*@temp@*/ /*@null@*/
	const char *				message
)
/*@globals
	fileSystem,
	internalState,

	NotEnoughMemoryException,
	NullPointerException
@*/
/*@modifies
	fileSystem,
	internalState
@*/
E4C_NO_RETURN;

# if defined(HAVE_C99_VSNPRINTF) || defined(HAVE_VSNPRINTF)

/*@unused@*/ /*@noreturn@*/ extern
void
e4c_exception_throw_format_(
	/*@shared@*/ /*@notnull@*/
	const e4c_exception_type *	exception_type,
	/*@observer@*/ /*@null@*/
	const char *				file,
	int							line,
	/*@observer@*/ /*@null@*/
	const char *				function,
	/*@observer@*/ /*@temp@*/ /*@notnull@*/ /*@printflike@*/
	const char *				format,
	...
)
/*@globals
	fileSystem,
	internalState,

	NotEnoughMemoryException,
	NullPointerException
@*/
/*@modifies
	fileSystem,
	internalState
@*/
E4C_NO_RETURN;

# endif

/*
 * End of the extern "C" block.
 */
#ifdef __cplusplus
	}
#endif

/*@=exportany@*/


# endif

#endif // CONF_USE_E4C

