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
 
#define CLOCKFREQ	1678000
#define MS2TMR(t)	((ulong)(((uvlong)(t)*CLOCKFREQ)/1000)) 
#define US2TMR(t)	((ulong)(((uvlong)(t)*CLOCKFREQ)/1000000))  

/*
 *  Address spaces
 *	nearly everything maps 1-1 with physical addresses
 *	0 to 1Mb is not mapped
 *	cache strategy varies as needed (see mmu.c)
 */

#define KZERO		0x02000000
#define MACHADDR	(KZERO+0x00002000)
#define KTTB		(KZERO+0x00004000)
#define KTZERO	(KZERO+0x00008010)
#define KSTACK	8192			/* Size of kernel stack */

#define DCFADDR	FLUSHMEM	/* cached and buffered for cache writeback */
#define MCFADDR	(FLUSHMEM+(1<<20))	/* cached and unbuffered for minicache writeback */
#define UCDRAMZERO	0xC8000000	/* base of memory doubly-mapped as uncached */
#define AIVECADDR	0xFFFF0000	/* alternative interrupt vector address (other is 0) */

/*
 * Memory
 */
 
#define	EWRAMZERO	0x02000000
#define	EWRAMTOP	0x023FFFFF
#define	IWRAMZERO	0x03000000
#define	IWRAMTOP	0x03007FFF
#define 	LCD			0x04000000	/* LCD controller */
#define	SPI			0x040001C0	/* serial peripheral interface controller */
#define	VRAM		0x04000240	/* Vram bank controller */
#define	POWER		0x04000304	/* Power controller */
#define	SUBLCD		0x04001000	/* sub LCD controller */
#define	WIFI			0x04800000
#define	PALMEM		0x05000000
#define	VRAMZERO	0x06000000
#define	VRAMTOP		0x06800000
#define	VRAMLO		0x06000000
#define	VRAMHI		0x0600A000
#define	INTHAND		0x03007FFC
#define	SFRbase		0x04000000
#define	ROMZERO		0x08000000

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



#define TIMERbase	0x04000100	/* timers */
