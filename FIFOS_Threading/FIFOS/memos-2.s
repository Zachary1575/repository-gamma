.global _start
.code32

_start:
	jmp real_start
	
	.align 4
	.long 0x1BADB002
	.long 0x00000003
	.long 0xE4524FFB
	
real_start:
	# Before we do anything, we need to push EAX and EBX onto the stack
	# So that we can use the multiboot structure
	push %eax
	push %ebx
	
	call kernel_main
	hlt


