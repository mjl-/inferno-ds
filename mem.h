/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */
#define	BI2BY		8			/* bits per byte */
#define BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2V		8			/* bytes per double word */
#define	BY2PG		4096			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define ROUND(s, sz)	(((s)+(sz-1))&~(sz-1))
#define PGROUND(s)	ROUND(s, BY2PG)
#define BIT(n)		(1<<n)
#define BITS(a,b)	((1<<(b+1))-(1<<a))

#define	MAXMACH		1			/* max # cpus system can run */

/*
 * Time
 */
#define	HZ		(100)			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */
#define	MS2TK(t)	((t)/MS2HZ)		/* milliseconds to ticks */

/*
 * More accurate time
 */
 
#define CLOCKFREQ	(0x02000000>>10) 	/* see TIMER_FREQ(Tmrdiv1024) at io.h */
#define MS2TMR(t)	((ulong)(((uvlong)(t)*CLOCKFREQ)/1000)) 
#define US2TMR(t)	((ulong)(((uvlong)(t)*CLOCKFREQ)/1000000))  

/*
 *  Address spaces
 *	nearly everything maps 1-1 with physical addresses
 *	0 to 1Mb is not mapped
 *	cache strategy varies as needed (see mmu.c)
 */

#define KZERO		0x02000000
#define MACHADDR	(KZERO+0x00000000)
#define KTZERO		(KZERO+0x00000130)
#define KSTACK	8192			/* Size of kernel stack */

#define AIVECADDR	0xFFFF0000	/* alternative interrupt vector address (other is 0) */

#define	ICACHESZ	8192
#define	DCACHESZ	4096
#define	CACHELINESZ	32

/*
 * Memory
 */

#define	EWRAMZERO	0x02000000
#define	EWRAMTOP	0x023FFFFF
#define	DWRAMZERO	0x0b000000
#define	DWRAMTOP9	0x0b003FFC
#define	IWRAMZERO9	0x03000000
#define	IWRAMTOP9	0x03007FFF
#define	LCD		0x04000000	/* LCD controller */
#define	SPI		0x040001C0	/* serial peripheral interface controller */
#define	VRAM		0x04000240	/* Vram bank controller */
#define	POWER		0x04000304	/* Power controller */
#define	SUBLCD		0x04001000	/* sub LCD controller */
#define	WIFI		0x04800000
#define	PALMEM		0x05000000
#define	VRAMZERO	0x06000000
#define	VRAMTOP		0x06800000
#define	VRAMLO		0x06000000
#define	VRAMHI		0x0600A000
#define	EXCHAND9	0x027FFD9C	/* exc handler */
#define	INTHAND9	0x00803FFC	/* irq handler set with writedtmcctl */
#define	IRQCHECK9	0x00803FF8	/* notify NDS BIOS of end of int */
#define	SFRZERO		0x04000000
#define	ROMZERO		0x08000000
#define	ROMTOP		0x09FFFFFF
#define	SRAMZERO	0x0A000000
#define	SRAMTOP		0x0A00FFFF
#define	DTCMZERO	0x027C0000	/* NDS DTCM standard address */
#define	DTCMSIZE	(16*1024)

/*
 * ARM7 specific
 */
#define	KSTACK7		512
#define	IWRAMZERO7	0x03800000
#define	IWRAMTOP7	0x0380FFFF
#define	INTHAND7	(SFRZERO - 4)
#define	IRQCHECK7	(SFRZERO - 8)

/*
 * PSR
 */
#define PsrMusr		0x10 	/* mode */
#define PsrMfiq		0x11 
#define PsrMirq		0x12
#define PsrMsvc		0x13
#define PsrMabt		0x17
#define PsrMund		0x1B
#define PsrMsys		0x1F
#define PsrMask		0x1F

#define PsrDfiq		0x00000040	/* disable FIQ interrupts */
#define PsrDirq		0x00000080	/* disable IRQ interrupts */

#define PsrV		0x10000000	/* overflow */
#define PsrC		0x20000000	/* carry/borrow/extend */
#define PsrZ		0x40000000	/* zero */
#define PsrN		0x80000000	/* negative/less than */

/*
 * Coprocessors
 */
 
#define CpMPU		15

/*
 * Memory protection unit:
 */

#define	Pagesz(logsz)	((logsz) << 1)
#define	Pagesz16K	(0x0e << 1)
#define	Pagesz32K	(0x0f << 1)
#define	Pagesz4M	(0x15 << 1)
#define	Pagesz32M	(0x18 << 1)
#define	Pagesz64M	(0x19 << 1)
#define	Pagesz128M	(0x1a << 1)

/*
 * Internal MPU coprocessor registers
 */
 
#define	CpCPUID		0		/* R: */
#define	CpControl	1		/* R/W: */
#define	CpCachebit	2		/* R/W: Cachability Bits for Protection Region*/
#define	CpWBops		3		/* W: Write Buffer operations */
#define	CpAccess	5		/* R/W: Access Permission Protection Region */
#define	CpPerm		6		/* R/W: Protection Unit Instruction Region 0..7 */
#define	CpCacheCtl	7		/* W: Cache Commands */
#define	CpTCM		9		/* R/W: Tightly Coupled Memory */

/*
 * CpControl bits
 */
 
#define	CpCmpu		0x00000001	/* MPU enable */
#define	CpCalign	0x00000002	/* alignment fault enable */
#define	CpCdcache	0x00000004	/* Data cache enable */
#define	CpCwb		0x00000008	/* write buffer turned on */
#define	CpCi32		0x00000010	/* 32-bit programme space */
#define	CpCd32		0x00000020	/* 32-bit data space */
#define	CpClateabt	0x00000040	/* Late abort Mode (pre v4) */
#define	CpCbe		0x00000080	/* big-endian operation */
#define	CpCbpredict	0x00000800	/* branch prediction */
#define	CpCicache	0x00001000	/* Instruction cache enable */
#define	CpCaltivec	0x00002000	/* alternative interrupt vectors */
#define	CpCrrob		0x00004000	/* Round Robin cache replacement */
#define	CpCprearm	0x00008000	/* Pre-ARMv5 Mode */
#define	CpCdtcme	0x00010000	/* DTCM enable */
#define	CpCdtcml	0x00020000	/* DTCM load mode */
#define	CpCitcme	0x00040000	/* ITCM enable */
#define	CpCitcml	0x00080000	/* ITCM load mode */

/*
 * Access permissions
 */

#define	MpuAP(i, v)	((v)<<((i)*4))	/* access permissions */
#define	MpuAPnone	0			/* no access */
#define	MpuAPsrw	1			/* supervisor rw */
#define	MpuAPuro	2			/* supervisor rw + user ro */
#define	MpuAPurw	3			/* supervisor rw + user rw */
#define	MpuAPsro	5			/* supervisor ro */
#define	MpuAPsuro	6			/* supervisor ro + user ro */
