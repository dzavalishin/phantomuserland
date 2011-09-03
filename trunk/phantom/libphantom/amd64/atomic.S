/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

//#define FUNCTION(x) .global x; .type x,@function; x
#define FUNCTION(x) .global x; x

.text

/* int atomic_add(int *val, int incr) */
FUNCTION(atomic_add):
	lock
	xadd	%rsi,(%rdi)
	ret

/* int atomic_and(int *val, int incr) */
FUNCTION(atomic_and):
	mov		(%rdi),%rax
	mov		%rsi,%rcx
	and		%rax,%rcx

	lock
	cmpxchg	%rcx,(%rdi)

	jnz		atomic_and

	ret

/* int atomic_or(int *val, int incr) */
FUNCTION(atomic_or):
	mov		(%rdi),%rax
	mov		%rsi,%rcx
	or		%rax,%rcx

	lock
	cmpxchg	%rcx,(%rdx)

	jnz		atomic_or

	ret

/* int atomic_set(int *val, int set_to) */
FUNCTION(atomic_set):
	mov		%rsi,%rax
	xchg	%rax,(%rdi)
	ret

/* int test_and_set(int *val, int set_to, int test_val) */
FUNCTION(test_and_set):
	mov		%rdx,%rax

	lock
	cmpxchg	%rsi,(%rdi)

	ret
#if 0
/* void x86_64_switch_stack_and_call(addr_t stack, void (*func)(void *), void *arg); */
FUNCTION(x86_64_switch_stack_and_call):
	mov		%rdi,%rsp		// switch the stack
	mov		%rdx,%rdi		// copy the arg
	call	*%rsi			// call the target function
_loop:
	jmp		_loop

#endif
