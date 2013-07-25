	.file	"ocr-stat-test_out.bc"
	.text
	.globl	mainEdt
	.align	16, 0x90
	.type	mainEdt,@function
mainEdt:                                # @mainEdt
	.cfi_startproc
# BB#0:                                 # %entry
	pushq	%rbp
.Ltmp3:
	.cfi_def_cfa_offset 16
.Ltmp4:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
.Ltmp5:
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	subq	$88, %rsp
.Ltmp6:
	.cfi_offset %rbx, -40
.Ltmp7:
	.cfi_offset %r14, -32
.Ltmp8:
	.cfi_offset %r15, -24
	movl	%edi, -28(%rbp)
	movq	%rsi, -40(%rbp)
	movl	%edx, -44(%rbp)
	movq	%rcx, -56(%rbp)
	movl	$.L.str, %edi
	xorl	%eax, %eax
	callq	printf
	leaq	-72(%rbp), %rdi
	leaq	-64(%rbp), %rsi
	movq	-80(%rbp), %r8
	movl	$100, %edx
	movl	$57005, %ecx            # imm = 0xDEAD
	xorl	%r9d, %r9d
	callq	ocrDbCreate
	movl	$10, %edi
	callq	malloc
	movq	%rax, -96(%rbp)
	movl	$0, -84(%rbp)
	movl	$1, %r14d
	jmp	.LBB0_1
	.align	16, 0x90
.LBB0_2:                                # %for.inc
                                        #   in Loop: Header=BB0_1 Depth=1
	movslq	-84(%rbp), %r15
	movq	-96(%rbp), %rbx
	leaq	(%rbx,%r15), %rdi
	movq	%r14, %rsi
	callq	store_callback
	movb	%r15b, (%rbx,%r15)
	incl	-84(%rbp)
.LBB0_1:                                # %for.cond
                                        # =>This Inner Loop Header: Depth=1
	cmpl	$9, -84(%rbp)
	jle	.LBB0_2
# BB#3:                                 # %for.end
	movq	-64(%rbp), %rbx
	movq	%rbx, -104(%rbp)
	movq	%rbx, %rdi
	movl	$4, %esi
	callq	store_callback
	movl	$109, (%rbx)
	movq	-104(%rbp), %rbx
	leaq	4(%rbx), %rdi
	movl	$4, %esi
	callq	store_callback
	movl	$101, 4(%rbx)
	movq	-104(%rbp), %rbx
	movq	%rbx, %rdi
	movl	$4, %esi
	callq	load_callback
	movl	(%rbx), %eax
	movl	%eax, -108(%rbp)
	xorl	%eax, %eax
	callq	ocrShutdown
	xorl	%eax, %eax
	addq	$88, %rsp
	popq	%rbx
	popq	%r14
	popq	%r15
	popq	%rbp
	ret
.Ltmp9:
	.size	mainEdt, .Ltmp9-mainEdt
	.cfi_endproc

	.type	.L.str,@object          # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	 "Hello !\n"
	.size	.L.str, 9


	.section	".note.GNU-stack","",@progbits
