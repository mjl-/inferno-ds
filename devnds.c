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
	Qbattery,
	Qversion,
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
	"battery",	{Qbattery},	0,		0444,
	"version",	{Qversion},	0,		0444,
	"touchctl",	{Qtouchctl},	0,	0644,
};

// static Point touch = Touchbase; /* what is the address of the touch screen in the nds? */

static void
ndsreset(void)
{
//	swi(Dssoftreset);
}


static void
ndsinit(void)
{
}

static Chan*
ndsattach(char* spec)
{
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
	int v, p, l;

	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, ndstab, nelem(ndstab), devgen);
	case Qtouchctl:
		break;
	case Qbattery:
		tmp = malloc(READSTR);
		if(waserror()){
			free(tmp);
			nexterror();
		}
		v = IPC->battery;
		snprint(tmp, READSTR, "status: %d %s\n", v, (v? "high" : "low"));
		n = readstr(offset, a, n, tmp);
		poperror();
		free(tmp);
		break;
		
	case Qversion:
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
	
	Xbtn =	1<<10,
	Ybtn	=	1<<11,
	Pdown	= 1<<12,

	// relative to XKEYS (arm7 only)
	Xbtn7	= 1<<0,
	Ybtn7	= 1<<1,
	Pdown7	= 1<<2,	// pen down
	Lclose7	= 1<<3,	// lid closed
};

static	Rune	rockermap[3][Numbtns] ={
	{'\n', '\t', Del, SysRq, Right, Left, Up, Down, RCtrl, RShift},	// right handed
	{'\n', '\t', Del, SysRq, Right, Left, Up, Down, RCtrl, RShift},	// left handed
	{'?', '|', Del, SysRq, Right, Left, Up, Down, RCtrl, RShift},	// debug
};

// done on vblank irq why ? polling ?; the same for touchread
// need a way to be notified when keys state changes

// debug mode controled by Conf.bmap, key combo to enable debug mode?
// to allow registering debugkeys to perform checks & ease debug

void setlcdblight(int on);

static int
ndskeys(void)
{
	int i;
	ushort state;
	static ushort ostate, b;

	state = REG_KEYINPUT;
	state |=  ~(IPC->buttons & (Xbtn7|Ybtn7|Pdown7)) << 10;

	if (ostate == state)
		return 0;

 	// check Lclose7 and switch lcd backlight on/off
	if (IPC->buttons & Lclose7)
		setlcdblight(0);
	else
		setlcdblight(1);
	
	for (i = 0 ; i < nelem(rockermap[conf.bmap]) ; i++){
		if ((state >> i) & 1){
			kbdrepeat(0);
		}else{
			if (0)print("ndskeys: %#x %c(%d)\n", state, rockermap[conf.bmap][i], i);
			kbdrepeat(0);
			kbdputc(kbdq, rockermap[conf.bmap][i]);
		}
	}
	ostate = state;

	if (state & Selbtn)
		b = 0;
	if (state & Startbtn)
		b = 1;
	if (state & Lbtn)
		b = 2;
	if (state & Rbtn)
		b = 4;
	
	return b;
}

static void
touchread(void*)
{
//	int x, y;
	int px, py, b, bbtn, oldbbtn=0, n=0;
	for(;;) {
		// should sleep until the pen is down
		if((IPC->buttons&Pendown))
			continue;
	
//		x=IPC->touchX;
//		y=IPC->touchY;

		if((bbtn=!(REG_KEYINPUT&Bbtn))^oldbbtn && !n--) {
			kbdputc(kbdq,'\n');
			bbtn=oldbbtn;
			n=500;
		}

		px=IPC->touchXpx;
		py=IPC->touchYpx;

		// the state of touch screen presses changes with buttons
		b = ndskeys();
			
		// todo take care of changes in buttons, penup/pendown, rockermap, handedness
		mousetrack(b, px, py, 0);

		if(1)print("ts %#d %d %X\n", px, py, b);
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
	ndsinit,
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
