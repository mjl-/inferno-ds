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

#define INTbase	(SFRbase + 0x4000)
#define INTREG	((IntReg *)INTbase)

typedef struct IntReg IntReg;
struct IntReg {
	ulong	msk;			
	ulong	pnd;
	ulong	wait;
	ulong	mie;	
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
typedef struct LcdReg LcdReg;
struct LcdReg {
	ushort lccr;	/* control */
	ushort lcsr;	/* status */
};

#define PAL	((ushort*)PALMEM)

#define VIDMEMHI	((ushort*)VRAMZERO)
#define VIDMEMLO	((ushort*)VRAMZERO)


void _halt(void);
void _reset(void);
void _waitvblank(void);
void _stop(void);
void _clearregs(void);

