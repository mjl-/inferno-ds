
/*
 * Interrupt controller
 */
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
#define	IPCSYNCbit		16		/* IPCSYNC */
#define	FSENDbit		17		/* SEND FIFO empty */
#define	FRECVbit		18		/* RECV FIFO non empty */
#define	CARDbit		19		
#define	CARDLINEbit	20
#define	GEOMFIFObit	21		/* geometry FIFO */
#define	LIDbit		22
#define	SPIbit		23		/* SPI */
#define	WIFIbit		24		/* WIFI */

#define INTREG	((IntReg *)(SFRZERO + 0x208))
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
	Fcmdtlen =	2,
	Fcmdslen =	4,
	Fcmdlen =	Fcmdtlen + Fcmdslen,		

	Fcmdtmask =	((1<<Fcmdtlen) - 1)<<Fcmdslen,	/* cmd type */
	Fcmdsmask =	(1<<Fcmdslen) - 1,		/* cmd subtype */
	Fcmdmask =	Fcmdtmask | Fcmdsmask,		/* cmd type | subtype */

	Fdatalen =	32 - Fcmdlen, 		/* log2(data) <= sizeof(EWRAM addr) <= Fdatalen bits */
	Fdatamask =	((1<<Fdatalen) - 1)<<Fcmdlen,	/* Fdatamask allows tx/rx of pointers */
};

/* F9 msgs: from arm9 to arm7 */	
enum
{
	/* message types */
	F9TSystem =	0<<Fcmdslen,
	F9TAudio =	1<<Fcmdslen,
	F9TWifi =	2<<Fcmdslen,

	/* System */
	F9Sysbright = 0,
	F9Syspoweroff,
	F9Sysreboot,
	F9Sysleds,
	F9Sysrrtc,
	F9Syswrtc,
	
	/* Wifi */
	F9WFrmac = 0,
	F9WFwstats,
	F9WFapquery,
	F9WFrxpkt,
	F9WFtxpkt,

	F9WFwstate,
	F9WFscan,
	F9WFstats,
	F9WFrxdone,
	
 	F9WFwssid,
 	F9WFrap,
 	F9WFwap,
 	F9WFwchan,
 	F9WFwwepkey,
 	F9WFwwepkeyid,
 	F9WFwwepmode,
 	
	/* Audio */
 	F9Auplay = 0,
	F9Aurec,
	F9Aupower,
};

/* F7 msgs: from arm7 to arm9 */
enum
{
	F7keyup = 0,
	F7keydown,
	F7mousedown,
	F7mouseup,
	F7print,
};

/* 
 * Dma controller
 */
#define DMAREG	((DmaReg*)(SFRZERO + 0x0b0))
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
	Dmarepeat	= 1<<25,
	
	Dmadstinc	= 0,
	Dmadstdec	= 1<<21,
	Dmadstfix	= 2<<21,
	Dmadstrst	= 3<<21,
	
	Dmatxwords	= (Dmaena|Dma32bit|Dmastartnow),
	Dmatxhwords	= (Dmaena|Dma16bit|Dmastartnow),
	Dmafifo		= (Dmaena|Dma32bit|Dmadstfix|Dmastartfifo)
};

/* 
 * Timers control
 */
#define TMRREG	((TimerReg*)(SFRZERO + 0x100))
typedef struct TimerReg TimerReg;
struct TimerReg {
	ushort data;	/* match */
	ushort ctl;
};

/* usage: TMRREG->data = TIMER_BASE(div) / Hz, where div is TmrdivN */ 
#define TIMER_BASE(d)    (-(0x02000000 >> ((6+2*(d-1)) * (d>0))))

/*
 * TMRREG ctl bits
 */
enum
{
	Tmrena		= (1<<7),	// enables the timer.
	Tmrirq		= (1<<6),	// request an Interrupt on overflow.
	Tmrcas		= (1<<2),	// cause the timer to count when the timer below overflows (unavailable for timer 0).
	Tmrdiv1 	= (0),		// set timer freq to 33.514 Mhz.
	Tmrdiv64 	= (1),		// set timer freq to (33.514 / 64) Mhz.
	Tmrdiv256 	= (2),		// set timer freq to (33.514 / 256) Mhz.
	Tmrdiv1024 	= (3),		// set timer freq to (33.514 / 1024) Mhz.
};

/* 
 * Lcd control (59.73 hz)
 */
#define LCDREG		((LcdReg*)LCD)
#define SUBLCDREG		((LcdReg*)SUBLCD)
typedef struct LcdReg LcdReg;
struct LcdReg {
	ulong lccr;	/* control */
	ushort lcsr;	/* status */
	ushort vcount;	/* vline count */

	ushort bgctl[4];
	struct {
		ushort xofs;
		ushort yofs;
	} bgofs[4];

	/* bg2 rotation/scaling */
	struct {
		ushort dx;
		ushort dmx;
		ushort dy;
		ushort dmy;

		ulong x;
		ulong y;
	} bg2rs;

	/* bg3 rotation/scaling */
	struct {
		ushort dx;
		ushort dmx;
		ushort dy;
		ushort dmy;

		ulong x;
		ulong y;
	} bg3rs;

	struct {
		ushort x[2];
		ushort y[2];

		ushort in;
		ushort out;
	} win;
	
	ushort mosaicsz;
	ushort pad1;

	struct {
		ushort ctl;
		ushort alpha;
		ushort bright;
	} blend;
	uchar pad2[10];
	
	ushort d3dctl;
	ulong dcapctl;
	ulong mmdfifo;
	ushort brighctl;
};

/*
 * LCDREG bits
 */
enum
{
	/* lcd sz */
	Scrwidth	=	256,
	Scrheight	=	192,
	Scrsize		= Scrwidth * Scrheight,

	/* lccr */
	Disp3dmode	=	1<<3,	/* 2d/3d mode */
	Disptilemap	=	1<<4,
	Dispiobj2ddim	=	1<<5,
	Dispobjmap	=	1<<6,
	Displhblank	=	1<<7,	/* forced hblank */

	Dispbg0ena	=	1<<8,
	Dispbg1ena	=	1<<9,
	Dispbg2ena	=	1<<10,
	Dispbg3ena	=	1<<11,
	Dispobjena	= 	1<<12,

	Dispwin0ena	=	1<<13,
	Dispwin1ena	=	1<<14,
	Dispowinena	=	1<<15,

	Disp2dmode	=	1<<16,	/* TODO: not sure about these */
	Dispfbmode	=	1<<17,

	Disp1dsz32	=	0<<20,
	Disp1dsz64	=	1<<20,
	Disp1dsz128	=	2<<20,
	Disp1dsz256	=	3<<20,
	Disp1dbmpsz128	=	0<<22,
	Disp1dbmpsz256	=	1<<22,

	Disphblankobj	=	1<<23,
	Dispextbgpal	=	1<<30,
	Dispextobjpal	=	1<<31,

	/* lcsr */
	DispInVblank	= 0,		/* display currently in a vertical blank. */
	DispInHblank	= 1,		/* display currently in a horizontal blank. */
	DispInVcount	= 2,		/* current scanline and %DISP_Y match. */
	DispIrqVblank	= 3,		/* Irq on vblank */
	DispIrqHblank	= 4,		/* Irq on hblank */
	DispIrqVcount	= 5,		/* Irq on Vcount*/

	/* blend */
	Blendnone		= 0<<6,
	Blendalpha		= 1<<6,
	Blendfadewhite		= 2<<6,
	Blendfadeblack		= 3<<6,

	Blendsrcbg0		= 1<<0,
	Blendsrcbg1		= 1<<1,
	Blendsrcbg2		= 1<<2,
	Blendsrcbg3		= 1<<3,
	Blendsrcsprite		= 1<<4,
	Blendsrcbackdrop	= 1<<5,

	Blenddstbg0		= 1<<8,
	Blenddstbg1		= 1<<9,
	Blenddstbg2		= 1<<10,
	Blenddstbg3		= 1<<11,
	
	Blenddstsprite		= 1<<12,
	Blenddstbackdrop	= 1<<13,
	
	/* bgctl */
	Bgctlpriomsk		= (1<<2)  - 1,
	
	Bgctl16bpc		= 1<<2,
	Bgmosaic			= 1<<6,
	Bgctl16bpp		= 1<<7,
	Bgctlwrap			= 1<<13,
	
	Bgctlrs16x16		= 0<<14,
	Bgctlrs32x32		= 1<<14,
	Bgctlrs64x64		= 2<<14,
	Bgctlrs128x128	= 3<<14,
	
	/* capture ctl */
	Dcapena		= 1<<31,
	
	Dcapa		= 0,
	Dcapb		= 8,
	Dcapbank		= 16,
	Dcapoffset	= 18,
	Dcapsz		= 20,
	Dcapsrc		= 24,
	Dcapdst		= 26,
	Dcapmode	= 29,
};

#define Bgctltilebase(base)	((base) << 2)
#define Bgctlmapbase(base)	((base) << 8)
#define Bgctlbmpbase(base)	((base) << 8)

/* set bg active with nbg in [0-3] */
#define Dispbgactive(nbg)	 (1 << (8 + (nbg)))

/*
 * VRAM & WRAM bank control
 */
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
	uchar wcr; /* wram control */
	uchar hcr;
	uchar icr;
};

enum
{
	Vramena = 1<<7,
};

/*
 * Power control 
 */
#define POWERREG ((PowerReg*)POWER)
typedef struct PowerReg PowerReg;
struct PowerReg {
	ushort pcr;
};

enum
{
	/* arm7 exclusive */
	POWER_SOUND	= 0,
	POWER_WIFI	= 1,

	/* arm9 exclusive */
	POWER_LCD =			1<<0,
	POWER_2D_A =		1<<1,
	POWER_MATRIX = 		1<<2,
	POWER_3D_CORE = 		1<<3,
	POWER_2D_B =		1<<9,

	POWER_SWAP_LCDS =	1<<15,
};

/*
 * SPI controller
 */
#define SPIREG ((SpiReg*)SPI)
#define ASPIREG ((SpiReg*)AUXSPI)
typedef struct SpiReg SpiReg;
struct SpiReg {
	ushort ctl;
	ushort data;
};

enum
{
	// SPI clock speed
	Spi4mhz =		0<<0,
	Spi2mhz =		1<<0,
	Spi1mhz = 		2<<0,
	Spi512khz =		3<<0,

  	Spihold	=		1<<6,
	Spibusy =		1<<7,

	// SPI device
	SpiDevpower =		0<<8,
	SpiDevfirmware =	1<<8,
	SpiDevnvram =	 	1<<8,
	SpiDevtouch = 		2<<8,

	// SPI transfer length
	Spibytetx =		0<<10,
	Spihwordtx =		1<<10,

	// When used, the /CS line will stay low after the transfer ends
	// i.e. when we're part of a continuous transfer
	Spicont = 		1<<11,

	Spiselect = 		1<<13,	/* set serial slot mode (AUXSPI) */
	Spiirqena =		1<<14,
	Spiena =		1<<15,
};

/* NDS file header info */
#define NDSHeader ((NDShdr*)0x027FFE00)
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

/* ram address to save UserInfo aka: user personal data */
#define UINFOMEM  (0x27FFC80)
#define UINFOREG ((UserInfo*)UINFOMEM)
typedef struct UserInfo UserInfo;
struct UserInfo{
	uchar version;
	uchar color;

	uchar theme;		// user's theme color (0-15).
	uchar birthmonth;	// user's birth month (1-12).
	uchar birthday;		// user's birth day (1-31).

	uchar pad1;		// ???

	Rune name[10];		// user's name in UTF-16 format.
	ushort namelen;		// length of the user's name in characters.

	Rune msg[26];		// user's message.
	ushort msglen;		// length of the user's message in characters.

	uchar alarmhour;	// alarm clock hour (0-23).
	uchar alarmmin;		// alarm clock minute (0-59).
	ushort pad2;
	ushort alarmon;

	/* ADC Touchscreen: 1st & 2nd calibration touches */
	struct{			
		ushort x1;
		ushort y1;
		uchar xpx1;
		uchar ypx1;

		ushort x2;
		ushort y2;
		uchar xpx2;
		uchar ypx2;
	} adc;

	ushort flags;

	ushort	pad3;
	ulong	rtcoffset;
	ulong	pad4;
};

/* UserInfo.flags */
enum {
	Nickmax	= 10,
	Msgmax	= 26,

	LJapanese = 0,
	LEnglish,
	LFrench,
	LGerman,
	LItalian,
	LSpanish,
	LChinese,
	LOther,
	Langmask	= 0x7,

	Gbalowerscreen	= 1<<3,

	Blightshift	= 4,
	Blightlow	= 0,
	Blightmedium,
	Blightigh,
	Blightmax,
	Blightmask	= 0x3,

	Autostart	= 1<<6,
	Nosettings	= 1<<9,
};

/* 
 * Input Buttons
 */
#define KEYINPUT	0x04000130
#define KEYREG ((KeyReg*)KEYINPUT)
typedef struct KeyReg KeyReg;
struct KeyReg {
	ushort in;
	ushort inctl;
	ushort rcnt;
	ushort xy;
	ushort rtccr;
};

/*
 * KEYREG bits
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
	Btnmsk	=	((1<<Maxbtns) - 1),

	// relative to XKEYS (arm7 only)
	Xbtn7	= 	0,
	Ybtn7	= 	1,
	Dbgbtn7	= 	3,	// debug btn
	Pdown7	= 	6,	// pen down
	Lclose7	= 	7,	// lid closed
};

/*
 * Synchronization reg
 */
#define IPCREG ((IpcReg*)0x04000180)
typedef struct IpcReg IpcReg;
struct IpcReg {
	ulong ctl;
};

enum
{
	Ipcdatain	= 0x00F,
	Ipcnotused	= 0x0F0,
	Ipcdataout	= 0xF00,
	Ipcgenirq	= 1<<13,
	Ipcirqena	= 1<<14,

	/* intr fom arm7 wifi */
	I7WFrxdone = 1<<8,
	I7WFtxdone,
};

/*
 * External memory reg
 */
#define EXMEMREG ((ExmReg*)0x04000204)
typedef struct ExmReg ExmReg;
struct ExmReg {
	ushort ctl;
};

enum
{
	Gbasramcycles	= (1<<1|1<<0),	/* (0-3 = 10, 8, 6, 18 cycles) */
	Gbarom1cycles	= (1<<3|1<<2),	/* (0-3 = 10, 8, 6, 18 cycles) */
	Gbarom2cycles	= 1<<4,		/* (0-1 = 6, 4 cycles) */
	Gbaslotphyout	= (1<<6|1<<5),	/* (0-3 = Low, 4.19MHz, 8.38MHz, 16.76MHz) */
	Arm7hasgba	= 1<<7,		/* (0=ARM9, 1=ARM7) */
	Arm7hasds	= 1<<11,	/* (0=ARM9, 1=ARM7) */
  	Ramaccessmode	= 1<<14,	/* (0=Async/GBA/Reserved, 1=Synchronous) */
	Arm7hasram	= 1<<15,	/* (0=ARM9 Priority, 1=ARM7 Priority) */
};

/*
 * discio interface
 */
enum
{
	Cread =			0x00000001,
	Cwrite =		0x00000002,
	Cslotgba =		0x00000010,
	Cslotnds =		0x00000020,
};

typedef struct Ioifc Ioifc;
struct Ioifc {
	char type[4];			/* name */
	ulong caps;			/* capabilities */
	
	int (* init)(void);
	int (* isin)(void);
	int (* read)(ulong start, ulong n, void* d);
	int (* write)(ulong start, ulong n, const void* d);
	int (* clrstat)(void);
	int (* deinit)(void);
};
