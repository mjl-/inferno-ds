#define MaxIRQbit		13			/* Maximum IRQ */
#define VBLANKbit		0
#define HBLANKbit		1
#define VCOUNTbit		2
#define TIMER0bit		3
#define TIMER1bit		4
#define TIMER2bit		5
#define TIMER3bit		6
#define UARTbit		7
#define GDMA0		8
#define GDMA1		9
#define GDMA2		10
#define GDMA3		11
#define KEYbit			12
#define CARTbit		13

#define TIMERbit(n) 		(TIMER0bit + n)

/*
  * Interrupt controller
  */

#define INTbase	(SFRbase + 0x208)
#define INTREG	((IntReg *)INTbase)

typedef struct IntReg IntReg;
struct IntReg {
	ushort	master;
	ulong	pad1;
	ulong	msk;
	ulong	pad2[4];
	ulong	pnd;
};




/* uarts? */

/* timers */
#define TIMERREG	((TimerReg*)TIMERbase)
typedef struct TimerReg TimerReg;
struct TimerReg {
	ulong	data[4];	/*  match */
	ulong	ctl[4];
};

/* lcd */
/* 59.73 hz */

#define LCDREG		((LcdReg*)LCD)
#define SUBLCDREG		((LcdReg*)SUBLCD)
typedef struct LcdReg LcdReg;
struct LcdReg {
	ulong lccr;	/* control */
	ulong lcsr;	/* status */
};

#define VRAMREG	((VramReg*)VRAM)
typedef struct VramReg VramReg;
struct VramReg {
	ulong acr;
	ulong bcr;
	ulong ccr;
	ulong dcr;
	ulong ecr;
	ulong fcr;
	ulong gcr;
	ulong hcr;
	ulong icr;
};

#define POWERREG ((PowerReg*)POWER)
typedef struct PowerReg PowerReg;
struct PowerReg {
	ushort pcr;
};

#define SPIREG ((PowerReg*)SPI)
typedef struct SpiReg SpiReg;
struct SpiReg {
	ushort spicr;
	ushort spidat;
};

#define PAL	((ushort*)PALMEM)

#define VIDMEMHI	((ushort*)VRAMTOP)
#define VIDMEMLO	((ushort*)VRAMZERO)

typedef struct Ipc Ipc;
struct Ipc {
	ulong cr;
};
void _halt(void);
void _reset(void);
void _waitvblank(void);
void _stop(void);
void _clearregs(void);

