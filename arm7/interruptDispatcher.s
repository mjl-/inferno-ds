#ifdef ARM7
	.text
#endif

#ifdef ARM9
	.section	.itcm,"ax",%progbits
#endif
	.extern	irqTable
	.code 32
	.global	IntrMain
IntrMain:
	mov	r3, #0x4000000		@ REG_BASE
	ldr	r1, [r3, #0x208]	@ r1 = IME
	str	r3, [r3, #0x208]	@ disable IME
	mrs	r0, spsr
	stmfd	sp!, {r0-r1,r3}		@ {spsr, IME, REG_BASE}
	ldr	r1, [r3,#0x210]		@ REG_IE
	ldr	r2, [r3,#0x214]		@ REG_IF
	and	r1,r1,r2
	ldr	r0,=__irq_flags		@ defined by linker script
	ldr	r2,[r0]
	orr	r2,r2,r1
	str	r2,[r0]
	ldr	r2,=irqTable
findIRQ:
	ldr r0, [r2, #4]
	cmp	r0,#0
	beq	no_handler
	ands	r0, r0, r1
	bne	jump_intr
	add	r2, r2, #8
	b	findIRQ
no_handler:
	str	r1, [r3, #0x0214]	@ IF Clear
	ldmfd   sp!, {r0-r1,r3}		@ {spsr, IME, REG_BASE}
	str	r1, [r3, #0x208]	@ restore REG_IME
	mov	pc,lr
jump_intr:
	ldr	r1, [r2]		@ user IRQ handler address
	cmp	r1, #0
	bne	got_handler
	mov	r1, r0
	b	no_handler
got_handler:
	mrs	r2, cpsr
	bic	r2, r2, #0xdf		@ \__
	orr	r2, r2, #0x1f		@ /  --> Enable IRQ & FIQ. Set CPU mode to System.
	msr	cpsr,r2
	str	r0, [r3, #0x0214]	@ IF Clear
	
	push	{lr}
	adr	lr, IntrRet
	bx	r1
IntrRet:
	pop	{lr}
	mov	r3, #0x4000000		@ REG_BASE
	str	r3, [r3, #0x208]	@ disable IME
	mrs	r3, cpsr
	bic	r3, r3, #0xdf		@ \__
	orr	r3, r3, #0x92		@ /  --> Disable IRQ. Enable FIQ. Set CPU mode to IRQ.
	msr	cpsr, r3
	ldmfd   sp!, {r0-r1,r3}		@ {spsr, IME, REG_BASE}
	str	r1, [r3, #0x208]	@ restore REG_IME
	msr	spsr, r0		@ restore spsr
	mov	pc,lr
	.pool
	.end

