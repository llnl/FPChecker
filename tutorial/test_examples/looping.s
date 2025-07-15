	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	d9, d8, [sp, #16]               ; 16-byte Folded Spill
	stp	x20, x19, [sp, #32]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset b8, -40
	.cfi_offset b9, -48
	mov	x19, x1
	ldr	x0, [x1, #8]
	bl	_atof
	fmov	d8, d0
	ldr	x0, [x19, #16]
	bl	_atoi
	cmp	w0, #1
	b.lt	LBB0_3
; %bb.1:
	fcvt	s0, d8
	cmp	w0, #3
	b.hi	LBB0_4
; %bb.2:
	mov	w8, #0                          ; =0x0
	movi	d1, #0000000000000000
	b	LBB0_7
LBB0_3:
	movi	d0, #0000000000000000
	b	LBB0_10
LBB0_4:
	and	w8, w0, #0x7ffffffc
	movi	d1, #0000000000000000
	mov	x9, x8
LBB0_5:                                 ; =>This Inner Loop Header: Depth=1
	fadd	s1, s1, s0
	fadd	s1, s1, s0
	fadd	s1, s1, s0
	fadd	s1, s1, s0
	subs	w9, w9, #4
	b.ne	LBB0_5
; %bb.6:
	cmp	w0, w8
	b.eq	LBB0_9
LBB0_7:
	sub	w8, w0, w8
LBB0_8:                                 ; =>This Inner Loop Header: Depth=1
	fadd	s1, s1, s0
	subs	w8, w8, #1
	b.ne	LBB0_8
LBB0_9:
	fcvt	d0, s1
LBB0_10:
	str	d0, [sp]
Lloh0:
	adrp	x0, l_.str@PAGE
Lloh1:
	add	x0, x0, l_.str@PAGEOFF
	bl	_printf
	mov	w0, #0                          ; =0x0
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #16]               ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.loh AdrpAdd	Lloh0, Lloh1
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"Sum: %.10f\n"

.subsections_via_symbols
