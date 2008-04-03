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

/*
  * Interrupt controller
  */

#define INTREG	((IntReg *)INTbase)

typedef struct IntReg IntReg;
struct IntReg {
	ulong	ime;	// Interrupt Master Enable
	ulong	pad1;
	ulong	ier;	// Interrupt Enable
	ulong	ipr;	// Interrupt Pending
};

/* uarts? */

/* timers */

#define TIMERREG	((TimerReg*)TIMERbase)
typedef struct TimerReg TimerReg;
struct TimerReg {
	ushort	data;	/*  match */
	ushort	ctl;
};

// timer ctl
enum
{
	Tmrena	= (1<<7),	//	enables the timer.
	Tmrirq	= (1<<6),	//	request an Interrupt on overflow.
	Tmrcas	= (1<<2),	//	cause the timer to count when the timer below overflows (unavailable for timer 0).
	Tmrdiv1	= (0),		//	set timer freq to 33.514 Mhz.
	Tmrdiv64 = (1),		//	set timer freq to (33.514 / 64) Mhz.
	Tmrdiv256 = (2),	//	set timer freq to (33.514 / 256) Mhz.
	Tmrdiv1024=(3),		//	set timer freq to (33.514 / 1024) Mhz.
};

// timer data usage:
// TIMER_FREQ(div) in Hz, where div is TmrdivN
#define TIMER_FREQ(d)    (-(0x02000000 >> ((6+2*(d-1)) * (d>0))))

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

