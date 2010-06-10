#include <mach/asm.h>

Entry(bios_int_10)
	int		$0x10
	int		$0x21
Entry(bios_int_10_end)



#if 0

#define VESA_X_TARGET 640
#define VESA_Y_TARGET 480
#define VESA_BIT_DEPTH_TARGET 16

.code16
.globl _vesa_setup_start
_vesa_setup_start:
	cld
        call find_vesa
        // mark finish of asm code
        mov             $0xAA, %bx
        int 		$0x21
	//ltr 0 // will cause a trap for sure

find_vesa:        
	// put the VBEInfo struct at DS:0
	mov		%ds, %dx
	mov		%dx, %es
	mov		$0x4f00, %ax
	mov		$0, %di
	int		$0x10

	// check the return code
	cmp		$0x4f, %al
	jne		done_vesa_bad
	cmp		$0x00, %ah
	jne		done_vesa_bad

	// check the signature on the data structure
	mov		%es:0, %eax
	cmp		$0x41534556, %eax   // 'VESA'
	je		vesa_sig_ok
	cmp		$0x32454256, %eax   // 'VBE2'
	jne		done_vesa_bad

        jmp 		done_vesa_good

vesa_sig_ok:
	// scan through each mode and grab the info on them
	les		%es:14, %bx	// calculate the pointer to the mode list
	mov		$0x200, %di	// push the buffer up a little to be past the VBEInfo struct

mode_loop:
	mov		%es:(%bx), %cx	// grab the next mode in the list
	cmp		$0xffff, %cx
	je		done_vesa_bad
	and		$0x01ff, %cx
	mov		$0x4f01, %ax
	int		$0x10

	// if it matches what we're looking for, go for it
	mov		%es:(%di), %ax
	test	$0x1, %ax        // test the supported bit
	jz		next_mode
	test	$0x08, %ax       // test the linear frame mode bit
	jz		next_mode
	mov		%es:18(%di), %ax
	cmp		$VESA_X_TARGET, %ax       // x
	jne		next_mode
	mov		%es:20(%di), %ax
	cmp		$VESA_Y_TARGET, %ax       // y
	jne		next_mode
	mov		%es:25(%di), %al
	cmp		$VESA_BIT_DEPTH_TARGET, %al         // bit_depth
	jne		next_mode

	// looks good, switch into it
	//mov		$0x4f02, %ax
	//mov		%cx, %bx
	//or		$0x4000, %bx     // add the linear mode bit
	//int		$0x10
	//cmp		$0x004f, %ax
	//je		done_vesa_good
        jmp 		done_vesa_good

next_mode:
	// get ready to try the next mode
	inc		%bx
	inc		%bx
	jmp		mode_loop

done_vesa_good:
	mov		$0x1, %ax
	ret

done_vesa_bad:
	xor		%ax, %ax
	ret


.globl _vesa_setup_end
_vesa_setup_end:

#endif


#if 0


// BX is mode num
.globl _vesa_mode_set_start
_vesa_mode_set_start:

	mov		$0x4f02, %ax
	or		$0x4000, %bx     // add the linear mode bit
	int		$0x10
	cmp		$0x004f, %ax
        int 		$0x21

.globl _vesa_mode_set_end
_vesa_mode_set_end:







.globl _vesa_complete_orig_setup_start
_vesa_complete_orig_setup_start:
	cld
        call find_vesa
        // mark finish of asm code
        mov             $0xAA, %bx
        int 		$0x21
	//ltr 0 // will cause a trap for sure

find_vesa:        
	// put the VBEInfo struct at DS:0
	mov		%ds, %dx
	mov		%dx, %es
	mov		$0x4f00, %ax
	mov		$0, %di
	int		$0x10

	// check the return code
	cmp		$0x4f, %al
	jne		done_vesa_bad
	cmp		$0x00, %ah
	jne		done_vesa_bad

	// check the signature on the data structure
	mov		%es:0, %eax
	cmp		$0x41534556, %eax   // 'VESA'
	je		vesa_sig_ok
	cmp		$0x32454256, %eax   // 'VBE2'
	jne		done_vesa_bad

vesa_sig_ok:
	// scan through each mode and grab the info on them
	les		%es:14, %bx	// calculate the pointer to the mode list
	mov		$0x200, %di	// push the buffer up a little to be past the VBEInfo struct

mode_loop:
	mov		%es:(%bx), %cx	// grab the next mode in the list
	cmp		$0xffff, %cx
	je		done_vesa_bad
	and		$0x01ff, %cx
	mov		$0x4f01, %ax
	int		$0x10

	// if it matches what we're looking for, go for it
	mov		%es:(%di), %ax
	test	$0x1, %ax        // test the supported bit
	jz		next_mode
	test	$0x08, %ax       // test the linear frame mode bit
	jz		next_mode
	mov		%es:18(%di), %ax
	cmp		$VESA_X_TARGET, %ax       // x
	jne		next_mode
	mov		%es:20(%di), %ax
	cmp		$VESA_Y_TARGET, %ax       // y
	jne		next_mode
	mov		%es:25(%di), %al
	cmp		$VESA_BIT_DEPTH_TARGET, %al         // bit_depth
	jne		next_mode

	// looks good, switch into it
	mov		$0x4f02, %ax
	mov		%cx, %bx
	or		$0x4000, %bx     // add the linear mode bit
	int		$0x10
	cmp		$0x004f, %ax
	je		done_vesa_good

next_mode:
	// get ready to try the next mode
	inc		%bx
	inc		%bx
	jmp		mode_loop

done_vesa_good:
	mov		$0x1, %ax
	ret

done_vesa_bad:
	xor		%ax, %ax
	ret


.globl _vesa_complete_orig_setup_end
_vesa_complete_orig_setup_end:
#endif

