#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	<keyboard.h>

#include 	"arm7/jtypes.h"
#include	"arm7/ipc.h"
#include	"arm7/touch.h"
#include	"arm7/system.h"

enum {
	Qdir,
	Qctl,
	Qinfo,
	Qtouchctl,
};

enum {
	Dssoftreset = 0x00,
	Dsdelay = 0x03,
	Dsintrwait = 0x04,
	Dswaitforvblank = 0x05,
	Dswaitforirq = 0x06,
	Dsdivide = 0x09,
	Dscopy = 0x0B,
	Dsfastcopy = 0x0C,
	Dssqrt = 0x0D,
	Dscrc16 = 0x0E,
	Dsisdebugger = 0x0F,
	Dsunpackbits = 0x10,
	Dsdecompresslzsswram = 0x11,
	Dsdecompresslzssvram = 0x12,
	Dsdecompresshuffman = 0x13,
	Dsdecompressrlewram = 0x14,
	Dsdecompressrlevram = 0x15,
	Dsdecodedelta8 = 0x16,
	Dsdecodedelta16 = 0x18,
	sethaltcr = 0x1F,
};

static void touchread(void*);
static
Dirtab ndstab[]={
	".",		{Qdir, 0, QTDIR},		0,		0555,
	"ndsctl",	{Qctl},		0,		0600,
	"ndsinfo",	{Qinfo},	0,		0600,
	"touchctl",	{Qtouchctl},	0,	0644,
};

// static Point touch = Touchbase; /* what is the address of the touch screen in the nds? */

static void
ndsreset(void)
{
//	swi(Dssoftreset);
}

static void
keysintr(Ureg*, void*)
{
	print("keysintr\n");
	intrclear(KEYbit, 0);
//	wakeup(&powerevent);
}

static Chan*
ndsattach(char* spec)
{
	REG_KEYCNT = (1<<0) | (1<<1) | (1<<14);
	intrenable(0, KEYbit, keysintr, nil, 0);

	if(0)kproc("touchread", touchread, nil, 0);
	return devattach('T', spec);
}

static Walkqid*
ndswalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, ndstab, nelem(ndstab), devgen);
}

static int
ndsstat(Chan* c, uchar *db, int n)
{
	return devstat(c, db, n, ndstab, nelem(ndstab), devgen);
}

static Chan*
ndsopen(Chan* c, int omode)
{
	return devopen(c, omode, ndstab, nelem(ndstab), devgen);
}


static void
ndsclose(Chan*)
{
}

static long
ndsread(Chan* c, void* a, long n, vlong offset)
{
	char *tmp, buf[64];
	uchar reply[12];
	int v, t, l;

	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, ndstab, nelem(ndstab), devgen);
	case Qtouchctl:
		break;
	case Qinfo:
		tmp = malloc(READSTR);
		if(waserror()){
			free(tmp);
			nexterror();
		}
		v = IPC->battery;
		t = 0xff; // read console type, version from firmware
		snprint(tmp, READSTR, "ds type: %x %s\nbattery: %d %s\n",
			t, (t == 0xff? "NDS" : "NDS-lite"), v, (v? "high" : "low"));
		n = readstr(offset, a, n, tmp);
		poperror();
		free(tmp);
		break;
		
	default:
		n=0;
		break;
	}
	return n;
}

static long
ndswrite(Chan* c, void* a, long n, vlong)
{
	char cmd[64], op[32], *fielnds[6];
	int nf;
	switch((ulong)c->qid.path){
	case Qctl:
		break;
	case Qtouchctl:
		break;
//		return touchctl(a, n);
	default:
		error(Ebadusefd);
	}
	return n;
}

enum
{
	Numbtns	= 13,

	// relative to KEYINPUT (arm9)
	Abtn	=	1<<0,
	Bbtn	=	1<<1,
	Selbtn	=	1<<2,
	Startbtn=	1<<3,
	Rightbtn=	1<<4,
	Leftbtn	=	1<<5,
	Upbtn	=	1<<6,
	Downbtn	=	1<<7,
	Rbtn	=	1<<8,
	Lbtn	=	1<<9,
	
	Xbtn	=	1<<10,
	Ybtn	=	1<<11,
	Lclose	= 	1<<13,
	Pdown	=	1<<16,

	// relative to XKEYS (arm7 only)
	Xbtn7	= 1<<0,
	Ybtn7	= 1<<1,
	Pdown7	= 1<<6,	// pen down
	Lclose7	= 1<<3,	// lid closed
};

static	Rune	rockermap[3][Numbtns] ={
	{'\n', Del, '\t', Esc, Right, Left, Up, Down, RCtrl, RShift, Pgup, Pgdown, No},	// right handed
	{'\n', Del, '\t', Esc, Right, Left, Up, Down, RCtrl, RShift, Pgup, Pgdown, No},	// left handed
	{'?', '|', Del, SysRq, Right, Left, Up, Down, RCtrl, RShift, Esc, No, No},	// debug
};

// TODO
// - debug mode controled by Conf.bmap, use debugkeys to perform checks & ease debug
// - take care of changes in buttons, penup/pendown, rockermap, handedness
// - screen orientation switch between landscape/portrait

#define kpressed(k, os, ns) (!(os & k) && (ns & k))
#define kreleased(k, os, ns) ((os & k) && !(ns & k))
void setlcdblight(int on);

static int
ndskeys(void)
{
	int i;
	ulong st, b;
	static ulong ost;

	st = REG_KEYINPUT;
	st |=  (~IPC->buttons & (Xbtn7|Ybtn7|Pdown7|Lclose7)) << 10;

	if(kreleased(Pdown, ost, st))
		b = 0;
	if(kpressed(Pdown, ost, st))
		b = 1;
	if(b && kpressed(Lbtn, ost, st))
		b = 2;
	if(b && kpressed(Rbtn, ost, st))
		b = 4;

 	// lid controls lcd backlight
	if(kpressed(Lclose, ost, st))
		setlcdblight(0);
	if(kreleased(Lclose, ost, st))
		setlcdblight(1);
	
	for (i=0; i<nelem(rockermap[conf.bmap]); i++){
		if (kpressed(1<<i, st, ost)){
			if (0)print("ndskeys: %#x %c(%d)\n", st, rockermap[conf.bmap][i], i);
			kbdrepeat(0);
			kbdputc(kbdq, rockermap[conf.bmap][i]);
		}else{
			kbdrepeat(0);
		}
	}

	ost = st;
	return b;
}

static int
tsactivity0(void *arg){
	return (~IPC->buttons & Pdown7) == Pdown7 || (REG_KEYINPUT & 0x3FF) != 0; 
}

static void
touchread(void*)
{
	int px, py, b, isdown;

	for(;;) {
		// pointer button events change with keys
		b = ndskeys();
		px=IPC->touchXpx;
		py=IPC->touchYpx;

		isdown=(~IPC->buttons & Pdown7);
		if(isdown)
			mousetrack(b, px, py, 0);
		else
			mousetrack(0, 0, 0, 1);

		// should sleep until the pen is down
		tsleep(&up->sleep, tsactivity0, nil, 50);

		if(0)print("ts down %x %#d %d %X\n", isdown, px, py, b);
	}
}

TransferRegion  *getIPC(void) {
	return (TransferRegion*)(0x027FF000);
}

static  void IPC_SendSync(unsigned int sync) {
	REG_IPC_SYNC = (REG_IPC_SYNC & 0xf0ff) | (((sync) & 0x0f) << 8) | IPC_SYNC_IRQ_REQUEST;
}


static  int IPC_GetSync() {
	return REG_IPC_SYNC & 0x0f;
}

Dev ndsdevtab = {
	'T',
	"nds",

	ndsreset,
	devinit,
	devshutdown,
	ndsattach,
	ndswalk,
	ndsstat,
	ndsopen,
	devcreate,
	ndsclose,
	ndsread,
	devbread,
	ndswrite,
	devbwrite,
	devremove,
	devwstat,
};
