/* 
 * Sound Channel
 */
#define SCHANREG	((SChanReg *)(SFRZERO + 0x400))
typedef struct SChanReg SChanReg;
struct SChanReg {
	union {
		struct {
			ushort vol;
			ushort pan;
		};
		ulong ctl;
	} cr;

	ulong	src;
	ushort	tmr;
	ushort	rpt;
	ulong	wlen;	/* in words of 4 bytes */

	uchar	pad3[16];
};

/* SCHANREG->cr.ctl bits */
enum {
	NSChannels	= 16,	/* number of channels */
	
	SCvoldiv		= 3<<8,
	SChold		= 1<<15,
	SCwduty		= 7<<24,
	
	SCrepman	= 0<<27,
	SCreploop	= 1<<27,
	SCrep1shot	= 2<<27,
	
	SCpcm8bit	= 0<<29,
	SCpcm16bit	= 1<<29,
	SCadpcm		= 2<<29,
	SCpsg		= 3<<29,

	SCena		= 1<<31,

	Minvol		= 0,
	Maxvol		= 127,
};

/* usage: SCHANREG->tmr = SCHAN_BASE / hz */
#define SCHAN_BASE	(-0x1000000)

/* 
 * Master sound
 */
#define SNDREG	((SndReg *)(SFRZERO + 0x500))
typedef struct SndReg SndReg;
struct SndReg {
	union {
		uchar mvol;
		ushort ctl;
	} cr;

	ushort pad1;
	ulong bias;
	ushort s508;
	ushort s510;
	ushort s514;
	ushort s518;
	ushort s51c;
};

enum {
	Sndena		= 1<<15,

	/* TxSound.flags: */
	AFlagin		= 1<<0,	/* direction: audio in/out */
	AFlag8bit	= 1<<1,	/* sample size: 8 bit/16 bit (1/0) */
	AFlagsigned	= 1<<2,	/* sample sign: signed/unsigned */
	AFlagmono	= 1<<3,	/* sample chans: mono/stereo */

	/* encoding */
	AFlagpcm	= 1<<4,	
	AFlagadpcm	= 1<<5,
	AFlagpsg	= 1<<6,
};

typedef struct TxSound TxSound;
struct TxSound {
	ushort	inuse;
	ushort	flags;	/* audio flags */
	TxSound	*phys;	/* uncached self-ptr to this TxSound */

	void	*d;	/* data samples */
	ulong	n;	/* samples number */
	long	rate;	/* rate (hz) */

	uchar	vol;	/* volume */
	uchar	pan;	/* panning */
	uchar	chan;	/* SCHANREG channel */
};

int audiorec(TxSound *snd, int on);
int audioplay(TxSound *snd, int on);
void audiopower(int dir, int level);
