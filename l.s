#include "mem.h"

/*
 * 		Entered from the boot loader with
 *		supervisor mode, interrupts disabled;
 */

TEXT _startup(SB), $-4
	MOVW		$setR12(SB), R12 	/* static base (SB) */
	MOVW		$Mach0(SB), R13
	ADD		$(KSTACK-4), R13	/* leave 4 bytes for link */
	

	MOVW		$(PsrDirq|PsrDfiq|PsrMsvc), R1	/* Switch to SVC mode */
	MOVW		R1, CPSR


	BL		main(SB)		/* jump to kernel */

dead:
	B	dead

GLOBL 		Mach0(SB), $KSTACK

TEXT setr13(SB), $-4
	MOVW		4(FP), R1

	MOVW		CPSR, R2
	BIC		$PsrMask, R2, R3
	ORR		R0, R3
	MOVW		R3, CPSR

	MOVW		R13, R0
	MOVW		R1, R13

	MOVW		R2, CPSR
	RET

TEXT _vundcall(SB), $-4			
_vund:
	MOVM.DB		[R0-R3], (R13)
	MOVW		$PsrMund, R0
	B		_vswitch

TEXT _vsvccall(SB), $-4				
_vsvc:
	MOVW.W		R14, -4(R13)
	MOVW		CPSR, R14
	MOVW.W		R14, -4(R13)
	BIC		$PsrMask, R14
	ORR		$(PsrDirq|PsrDfiq|PsrMsvc), R14
	MOVW		R14, CPSR
	MOVW		$PsrMsvc, R14
	MOVW.W		R14, -4(R13)
	B		_vsaveu

TEXT _vpabcall(SB), $-4			
_vpab:
	MOVM.DB		[R0-R3], (R13)
	MOVW		$PsrMabt, R0
	B		_vswitch

TEXT _vdabcall(SB), $-4	
_vdab:
	MOVM.DB		[R0-R3], (R13)
	MOVW		$(PsrMabt+1), R0
	B		_vswitch

TEXT _vfiqcall(SB), $-4				/* IRQ */
_vfiq:		/* FIQ */
	MOVM.DB		[R0-R3], (R13)
	MOVW		$PsrMfiq, R0
	B		_vswitch

TEXT _virqcall(SB), $-4				/* IRQ */
_virq:
	MOVM.DB		[R0-R3], (R13)
	MOVW		$PsrMirq, R0

_vswitch:					/* switch to svc mode */
	MOVW		SPSR, R1
	MOVW		R14, R2
	MOVW		R13, R3

	MOVW		CPSR, R14
	BIC		$PsrMask, R14
	ORR		$(PsrDirq|PsrDfiq|PsrMsvc), R14
	MOVW		R14, CPSR

	MOVM.DB.W 	[R0-R2], (R13)
	MOVM.DB	  	(R3), [R0-R3]

_vsaveu:						/* Save Registers */
	MOVW.W		R14, -4(R13)			/* save link */
/*	MCR		CpMMU, 0, R0, C(0), C(0), 0 */	

	SUB		$8, R13
	MOVM.DB.W 	[R0-R12], (R13)

	MOVW		R0, R0				/* gratuitous noop */

	MOVW		$setR12(SB), R12		/* static base (SB) */
	MOVW		R13, R0				/* argument is ureg */
	SUB		$8, R13				/* space for arg+lnk*/
	BL		trap(SB)


_vrfe:							/* Restore Regs */
	MOVW		CPSR, R0			/* splhi on return */
	ORR		$(PsrDirq|PsrDfiq), R0, R1
	MOVW		R1, CPSR
	ADD		$(8+4*15), R13		/* [r0-R14]+argument+link */
	MOVW		(R13), R14			/* restore link */
	MOVW		8(R13), R0
	MOVW		R0, SPSR
	MOVM.DB.S 	(R13), [R0-R14]		/* restore user registers */
	MOVW		R0, R0				/* gratuitous nop */
	ADD		$12, R13		/* skip saved link+type+SPSR*/
	RFE					/* MOVM.IA.S.W (R13), [R15] */
	
TEXT splhi(SB), $-4					
	MOVW		CPSR, R0
	ORR		$(PsrDirq), R0, R1
	MOVW		R1, CPSR
	RET

TEXT spllo(SB), $-4
	MOVW		CPSR, R0
	BIC		$(PsrDirq|PsrDfiq), R0, R1
	MOVW		R1, CPSR
	RET

TEXT splx(SB), $-4
	MOVW	$(MACHADDR), R6
	MOVW	R14, (R6)	/* m->splpc */

TEXT splxpc(SB), $-4
	MOVW		R0, R1
	MOVW		CPSR, R0
	MOVW		R1, CPSR
	RET

TEXT islo(SB), $-4
	MOVW		CPSR, R0
	AND		$(PsrDirq), R0
	EOR		$(PsrDirq), R0
	RET

TEXT splfhi(SB), $-4					
	MOVW		CPSR, R0
	ORR		$(PsrDfiq|PsrDirq), R0, R1
	MOVW		R1, CPSR
	RET

TEXT splflo(SB), $-4
	MOVW		CPSR, R0
	BIC		$(PsrDfiq), R0, R1
	MOVW		R1, CPSR
	RET

TEXT cpsrr(SB), $-4
	MOVW		CPSR, R0
	RET

TEXT spsrr(SB), $-4
	MOVW		SPSR, R0
	RET

TEXT getcallerpc(SB), $-4
	MOVW		0(R13), R0
	RET

TEXT _tas(SB), $-4
	MOVW		R0, R1
	MOVW		$0xDEADDEAD, R2
	SWPW		R2, (R1), R0
	RET

TEXT setlabel(SB), $-4
	MOVW		R13, 0(R0)		/* sp */
	MOVW		R14, 4(R0)		/* pc */
	MOVW		$0, R0
	RET

TEXT gotolabel(SB), $-4
	MOVW		0(R0), R13		/* sp */
	MOVW		4(R0), R14		/* pc */
	MOVW		$1, R0
	BX			(R14)

TEXT outs(SB), $-4
	MOVW	4(FP),R1
	WORD	$0xe1c010b0	/* STR H R1,[R0+0] */
	RET

TEXT ins(SB), $-4
	WORD	$0xe1d000b0	/* LDRHU R0,[R0+0] */
	RET

/* for devboot */
TEXT	gotopc(SB), $-4
/*
	MOVW	R0, R1
	MOVW	bootparam(SB), R0
	MOVW	R1, PC
*/
	RET


TEXT _halt(SB), $-4
	SWI 0x02

TEXT _reset(SB), $-4
	SWI 0x0

/* need to allow kernel to pass args on what to clear */	
TEXT	_clearregs(SB), $-4
	MOVW $0x4, R0
	SWI 0x1

TEXT _stop(SB), $-4
	MOVW $0xEAEAEA,R0

TEXT swiDelay(SB), $-4
	SWI	0x030000
	RET

TEXT	waitvblank(SB), $-4
	SWI	0x050000
	RET

/* dsemu only: used to print to log */
TEXT consputs(SB),$-4
	SWI	0xff0000
	RET

TEXT getdtcm(SB), $-4
	MRC		15,0,R0,C9,C1,0
	RET

TEXT getdtcmctl(SB), $-4
	MRC		15,0,R0,C9,C1,0
	RET

TEXT writedtcmctl(SB), $-4
	MCR		15,0,R0,C9,C1,0
	RET
TEXT loop(SB), $-4
wait:
	B wait
	RET

/*
 * Memory protection unit: go(es)to mem.h
 */
 
#define PAGE_16K	(0x0e << 1)
#define PAGE_32K	(0x0f << 1)
#define PAGE_4M	(0x15 << 1)
#define PAGE_64M	(0x19 << 1)
#define PAGE_128M	(0x1a << 1)

#define ITCM_EN		(1<<18)
#define DTCM_EN		(1<<16)
#define ICACHE_EN		(1<<12)
#define DCACHE_EN		(1<<2)
#define PROTECT_EN		(1<<0)

TEXT mprotinit(SB), $-4
	/* turn the power on for M3 */
	MOVW	$0x04000304, R0
	MOVW	$0x8203, R1
	MOVW	R1, (R0)
	
	/* enable arm9 iwram */
	MOVW	$0x04000247, R0
	MOVW	$0, R1
	MOVW	R1, (R0)

	/* disable DTCM and protection unit */
	MOVW	0x00002078, R1		
//	MCR	15, 0, R1, C1, C0
	
	/* allocate GBA+Main Memory to ARM9 */
	/* (Setting wait_cr here allows init of card specific registers here) */
	/* bit  7: GBA  Slot allocated to ARM9 */
	/* bit 11: DS   Slot allocated to ARM9 */
	/* bit 15: Main Memory priority to ARM9 */
	MOVW	$0x04000204, R0 			/* wait_cr */
	MOVW	(R0), R1
	BIC	$0x0080, R1, R1
	BIC	$0x8800, R1, R1
	MOVW	R1, (R0)
	
	/* Protection unit Setup added by Sasq */
	/* Disable cache */
	MOVW	0, R0
	MCR	15, 0, R0, C7, C5, 0		/*  Instruction cache */
	MCR	15, 0, R0, C7, C6, 0		/* Data cache */
	
	/* Wait for write buffer to empty */
	MCR	15, 0, R0, C7, C10, 4

	MOVW	$(DWRAMZERO), R0
	ORR	$0x0a,R0,R0
	MCR	15, 0, R0, C9, C1,0		/* DTCM base = __dtcm_start, size = 16 KB */

	MOVW	$0x20, R0
	MCR	15, 0, R0, C9, C1,1		/* ITCM base = 0 , size = 32 MB */

	/* Setup memory regions similar to Release Version */

	/* Region 0 - IO registers */
	MOVW	$(PAGE_64M | 0x04000000 | 1), R0
	MCR	15, 0, R0, C6, C0, 0

	/* Region 1 - Main Memory */
	MOVW	$(PAGE_4M | 0x02000000 | 1), R0
	MCR	15, 0, R0, C6, C1, 0

	/* Region 2 - iwram */
	MOVW	$(PAGE_32K | 0x037F8000 | 1), R0
	MCR	15, 0, R0, C6, C2, 0

	/* Region 3 - DS Accessory (GBA Cart) */
	MOVW	$(PAGE_128M | 0x08000000 | 1), R0
	MCR	15, 0, R0, C6, C3, 0

	/* Region 4 - DTCM */
	MOVW	$(PAGE_16K | DWRAMZERO | 1), R0
	MCR	15, 0, R0, C6, C4, 0

	/* Region 5 - ITCM */
	MOVW	$(PAGE_32K | IWRAMZERO9 | 1), R0
	MCR	15, 0, R0, C6, C5, 0

	/* Region 6 - System ROM */
	MOVW	$(PAGE_32K | 0xFFFF0000 | 1), R0
	MCR	15, 0, R0, C6, C6, 0

	/* Region 7 - non cacheable main ram */
	MOVW	$(PAGE_4M  | 0x02400000 | 1), R0
	MCR	15, 0, R0, C6, C7, 0

	/* Write buffer enable */
	MOVW	$0x2, R0
	MCR	15, 0, R0, C3, C0, 0

	/* DCache & ICache enable */
	MOVW	$0x42, R0
	MCR	15, 0, R0, C2, C0, 0
	MCR	15, 0, R0, C2, C0, 1

	/* IAccess */
	MOVW	0x36636333, R0
	MCR	15, 0, R0, C5, C0, 3

	/* DAccess */
	MOVW	0x36333333, R0
	MCR	15, 0, R0, C5, C0, 2

        /* Enable ICache, DCache, ITCM & DTCM */
//	MCR	15, 0, R0, C1, C0, 0
	MOVW	$(ITCM_EN | DTCM_EN | ICACHE_EN | DCACHE_EN | PROTECT_EN), R1
	ORR	R1, R0, R0
//	MCR	15, 0, R0, C1, C0, 0

	RET
