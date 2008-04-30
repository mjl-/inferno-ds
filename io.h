
/*
 * Interrupt controller
 */

#define INTREG	((IntReg *)INTbase)

#define	MaxIRQbit	24		/* Maximum IRQ */

#define	VBLANKbit	0		/* Vertical blank */
#define	HBLANKbit	1		/* Horizontal blank */
#define	VCOUNTbit	2		/* Vertical count */
#define	TIMER0bit		3		/* Timer 0 */
#define	TIMER1bit		4		/* Timer 1 */
#define	TIMER2bit		5		/* Timer 2 */
#define	TIMER3bit		6		/* Timer 3 */
#define	UARTbit		7		/* Serial/Comms */
#define	GDMA0		8		/* DMA 0 */
#define	GDMA1		9		/* DMA 1 */
#define	GDMA2		10		/* DMA 2 */
#define	GDMA3		11		/* DMA 3 */
#define	KEYbit		12		/* Keypad */
#define	CARTbit		13		/* CART */
#define	ARM7bit		16		/* ARM7 IPC */
#define	FSENDbit		17		/* SEND FIFO empty */
#define	FRECVbit		18		/* RECV FIFO non empty */
#define	CARDbit		19		
#define	CARDLINEbit	20
#define	GEOMFIFObit	21		/* geometry FIFO */
#define	LIDbit		22
#define	SPIbit		23		/* SPI */
#define	WIFIbit		24		/* WIFI */

#define DISP_VBLANKbit	3
#define DISP_HBLANKbit	4
#define DISP_VCOUNTbit	5

typedef struct IntReg IntReg;
struct IntReg {
	ulong	ime;	// Interrupt Master Enable
	ulong	pad1;
	ulong	ier;	// Interrupt Enable
	ulong	ipr;	// Interrupt Pending
};

/*
 * Fifo controller
 */

#define	FIFObase	0x04000184
#define	FIFOend		0x04100000
#define FIFOREG	((FifoReg*)FIFObase)
typedef struct FifoReg FifoReg;
struct FifoReg {
	ushort	ctl;
	ulong	send;
	uchar	pad1[FIFOend - FIFObase - 2*sizeof(ulong)];
	ulong	recv;
};

enum
{
	FifoTempty =	1<<0,
	FifoTfull =	1<<1,
	FifoTirq =	1<<2,
	FifoTflush =	1<<3,
	FifoRempty =	1<<8,
	FifoRfull =	1<<9,
	FifoRirq =	1<<10,
	Fifoerror =	1<<14,
	Fifoenable =	1<<15,
};


/*
 * Fifo message types
 */

enum
{
	Fcmdwidth =	4,
	Fcmdmask =	(1<<Fcmdwidth)-1,

	/* from arm9 to arm7 */
	F9brightness =	0,

	/* from arm7 to arm9 */
	F7keyup,
	F7keydown,
	F7mousedown,
	F7mouseup,
};

/* 
 * Dma controller
 */

#define DMAREG	((DmaReg*)DMAbase)
typedef struct DmaReg DmaReg;
struct DmaReg {
	ulong	src;
	ulong	dst;
	ulong	ctl;
};

enum
{
	Dmaena		= 1<<31,		/* enable dma channel */
	Dmabusy		= 1<<31,		/* is dma channel busy */
	Dmairq		= 1<<30,		/* request an interrupt on dma */

	Dmastartnow	= 0,
	Dmastartcard	= 5<<27,
	Dmastartfifo	=  7<<27,
	
	Dma16bit		= 0,
	Dma32bit		= 1<<26,
	
	Dmasrcinc	= 0,
	Dmasrcdec	= 1<<23,
	Dmasrcfix	= 1<<24,
	
	Dmadstinc	= 0,
	Dmadstdec	= 1<<21,
	Dmadstfix	= 1<<22,
	Dmadstrst	= 3<<21,
	
	Dmatxwords	= (Dmaena|Dma32bit|Dmastartnow),
	Dmatxhwords	= (Dmaena|Dma16bit|Dmastartnow),
	Dmafifo		= (Dmaena|Dma32bit|Dmadstfix|Dmastartfifo)
};

/* 
 * Timers control
 */

#define TIMERREG	((TimerReg*)TIMERbase)
typedef struct TimerReg TimerReg;
struct TimerReg {
	ushort	data;	/* match */
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

/* usage: 
 * tmr->data = TIMER_BASE(div) / Hz, where div is TmrdivN 
 */ 
#define TIMER_BASE(d)    (-(0x02000000 >> ((6+2*(d-1)) * (d>0))))

/* 
 * Lcd control (59.73 hz)
 */

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
	uchar acr;
	uchar bcr;
	uchar ccr;
	uchar dcr;
	uchar ecr;
	uchar fcr;
	uchar gcr;
	uchar hcr;
	uchar icr;
};

#define POWERREG ((PowerReg*)POWER)
typedef struct PowerReg PowerReg;
struct PowerReg {
	ushort pcr;
};

enum {
	POWER_LCD =		1<<0,
	POWER_2D_A =		1<<1,
	POWER_MATRIX = 		1<<2,
	POWER_3D_CORE = 	1<<3,
	POWER_2D_B =		1<<9,
	POWER_SWAP_LCDS =	1<<15,
};


#define SPIREG ((PowerReg*)SPI)
typedef struct SpiReg SpiReg;
struct SpiReg {
	ushort spicr;
	ushort spidat;
};

#define VIDMEMHI	((ushort*)VRAMTOP)
#define VIDMEMLO	((ushort*)VRAMZERO)

/* NDS file header info */
typedef struct NDShdr NDShdr;
struct NDShdr{
	char gtitle[12];
	char gcode[4];
	char mcode[2];
	uchar unitcode;	
	uchar devtype;		
	uchar devcapa;
	uchar rserv1[10];
	uchar romver;
	uchar rserv2;
	ulong romoff9;
	ulong entry9;
	ulong ram9;
	ulong size9;
	ulong romoff7;
	ulong entry7;
	ulong ram7;
	ulong fntoff;
	ulong fntsz;
	ulong fatoff;
	ulong fatsz;
	ulong ovloff9;
	ulong ovlsz9;
	ulong ovloff7;
	ulong ovlsz7;
	ulong romctl1;
	ulong romctl2;
	ulong iconoff;
	ushort sacrc;
	ushort romctl3;
	ulong arm9;
	ulong arm7;
	ulong magic1;
	ulong magic2;
	ulong appeoff;
	ulong romhdrsz;
	uchar rserv3[24];
	ulong rserv4[3];
	ulong rserv5[1];
	ushort logocrc;
	ushort hdrcrc;
};


/* 
 * Input Buttons
 */
enum
{
	Numbtns	= 13,
	Btn9msk = (1<<10) - 1,
	Btn7msk = (1<<8)  - 1,

	// relative to KEYINPUT (arm9)
	Abtn	=	0,
	Bbtn	=	1,
	Selbtn	=	2,
	Startbtn=	3,
	Rightbtn=	4,
	Leftbtn	=	5,
	Upbtn	=	6,
	Downbtn	=	7,
	Rbtn	=	8,
	Lbtn	=	9,
	
	Xbtn	=	10,
	Ybtn	=	11,
	Dbgbtn	= 	13,
	Pdown	=	16,
	Lclose	= 	17,
	Maxbtns,

	// relative to XKEYS (arm7 only)
	Xbtn7	= 	0,
	Ybtn7	= 	1,
	Dbgbtn7	= 	3,	// debug btn
	Pdown7	= 	6,	// pen down
	Lclose7	= 	7,	// lid closed
};

enum
{
	Button1 =	1<<0,
	Button2 =	1<<1,
	Button3 =	1<<2,
};

typedef struct Ipc Ipc;
struct Ipc {
	ulong cr;
};

void _halt(void);
void _reset(void);
void _waitvblank(void);
void _stop(void);
void _clearregs(void);
