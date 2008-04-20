#include "../mem.h"

TEXT _main(SB), $-4
/*	MOVW		$Mach0(SB), R13 */
//	ADD		$(KSTACK-4), R13	/* leave 4 bytes for link */

	MOVW		$0, R0			/* start with IME=0 */
	MOVW		$(INTbase), R1	
	MOVW		R0, (R1)

	MOVW		$setR12(SB), R12 	/* static base (SB) */

	MOVW		$(PsrMirq), R1		/* Switch to IRQ mode */
	MOVW		R1, CPSR
	MOVW		$(IWRAMTOP7 - 0x60), R13

	MOVW		$(PsrMsys), R1		/* Switch to System mode */
	MOVW		R1, CPSR
	MOVW		$(IWRAMTOP7 - 0x160), R13

	BL		main(SB)		/* jump to kernel */
dead:
	B		dead
	BL		_div(SB)			/* hack to get _div etc loaded */

TEXT swiDelay(SB), $-4
	SWI	0x030000
	RET

TEXT swiWaitForVBlank(SB), $-4
	SWI	0x050000
	RET

TEXT swiHalt(SB), $-4
	SWI	0x060000
	RET

TEXT swiDivide(SB), $-4
	SWI	0x090000
	RET

TEXT swiRemainder(SB), $-4
	SWI	0x090000
	MOVW	R1, R0
	RET

/* fixme will need to figure out more here */
TEXT swiDivMod(SB), $-4
	MOVM.DB.W	[R2-R3], (R13)
	SWI 0x090000
	MOVM.DB.W	(R13), [R2-R3]
	MOVW	R0, (R2)
	MOVW	R1, (R3)
	RET

TEXT swiCRC16(SB), $-4
	SWI	0x0E0000
	RET
