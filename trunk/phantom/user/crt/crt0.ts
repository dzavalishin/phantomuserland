// TODO replace C version with asm


	movl   $0, %ebp
        push   %ebp
	mov    %esp,%ebp
	
        sub    $0x18,%esp
	movl   $0x0,0x8(%esp)
	movl   $0x0,0x4(%esp)
	movl   $0x0,(%esp)
        
	call   EXT(main)
	
        push   %eax
        call   EXT(exit)

	// Exit is broken?        
	int     $3
