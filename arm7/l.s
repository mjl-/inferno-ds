#include "mem.h"

TEXT _main(SB), $-4
	MOVW		$setR12(SB), R12 	/* static base (SB) */
/*	MOVW		$Mach0(SB), R13 */
	ADD		$(KSTACK-4), R13	/* leave 4 bytes for link */

	MOVW		$(PsrDirq|PsrDfiq|PsrMsvc), R1	/* Switch to SVC mode */
	MOVW		R1, CPSR


	BL		main(SB)		/* jump to kernel */
TEXT swiDelay(SB), $-4
	SWI	0x030000
	RET
TEXT swiWaitForVBlank(SB), $-4
	SWI	0x050000
	RET
/* fixme will need to figure out more here */
TEXT swiDivMod(SB), $-4
	SWI 0x090000
	RET
TEXT IntrMain(SB), $-4
	RET
/*
	mov	r3, #0x4000000		@ REG_BASE

	ldr	r1, [r3, #0x208]	@ r1 = IME
	str	r3, [r3, #0x208]	@ disable IME
	mrs	r0, spsr
	stmfd	sp!, {r0-r1,r3,lr}	@ {spsr, IME, REG_BASE, lr_irq}

	ldr	r1, [r3,#0x210]		@ REG_IE
	ldr	r2, [r3,#0x214]		@ REG_IF
	and	r1,r1,r2

	ldr	r0,=__irq_flags		@ defined by linker script

	ldr	r2,[r0]
	orr	r2,r2,r1
	str	r2,[r0]

	ldr	r2,=irqTable
	
	*/