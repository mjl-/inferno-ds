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

	// relative to XKEYS (arm7 only)
	Xbtn7	= 	0,
	Ybtn7	= 	1,
	Dbgbtn7	= 	3,	// debug btn
	Pdown7	= 	6,	// pen down
	Lclose7	= 	7,	// lid closed
};

// TODO use Dbgbtn7 to enable debug; by toggling Conf.bmap = 2
static	Rune	rockermap[3][Numbtns] ={
	{'\n', Del, '\t', Esc, Right, Left, Up, Down, RCtrl, RShift, Pgup, Pgdown, No},	// right handed
	{'\n', Del, '\t', Esc, Right, Left, Up, Down, RCtrl, RShift, Pgup, Pgdown, No},	// left handed
	{'?', '|', Del, SysRq, Right, Left, Up, Down, RCtrl, RShift, Esc, No, No},	// debug
};

// TODO
// - take care of changes in buttons, penup/pendown, rockermap, handedness
// - screen orientation switch between landscape/portrait

#define kpressed(k, os, ns) ((os & (k)) && !(ns & (k)))
#define kreleased(k, os, ns) (!(os & (k)) && (ns & (k)))
void setlcdblight(int on);

static ulong
ndskeys(void)
{
	int i, c;
	ulong st, b = 0;
	static ulong ost, ob = 0;

	st = (REG_KEYINPUT & Btn9msk);
	st |= (IPC->buttons & Btn7msk) << 10;

	if (ost == st)
		return ob;
	
	// mousemod
	if (kpressed(1<<Pdown, ost, st))
		b |= (1<<0);
	else if (kreleased(1<<Pdown, ost, st))
		b &= ~(1<<0);

	if (kpressed(1<<Pdown|1<<Rbtn, ost, st))
		b |= (1<<1);
	else if (kreleased(1<<Pdown|1<<Rbtn, ost, st))
		b &= ~(1<<1);

	if (kpressed(1<<Pdown|1<<Lbtn, ost, st))
		b |= (1<<2);
	else if (kreleased(1<<Pdown|1<<Lbtn, ost, st))
		b &= ~(1<<2);

	// lid controls lcd backlight
	if(kpressed(1<<Lclose, ost, st))
		setlcdblight(1);
	if(kreleased(1<<Lclose, ost, st))
		setlcdblight(0);

	for (i=0; i<nelem(rockermap[conf.bmap]); i++){
		if (kpressed(1<<i, ost, st)){
			kbdrepeat(0);
			kbdputc(kbdq, rockermap[conf.bmap][i]);
		}else if (kreleased(1<<i, ost, st)){
			kbdrepeat(0);
		}
	}

	ob = b;
	ost = st;
	return b;
}

static void
vblankintr()
{
	int b;
	static ulong ob;

	b = ndskeys();
	if(b)
		mousetrack(b, IPC->touchXpx, IPC->touchYpx, 0);
	else if(ob)
		mousetrack(b, 0, 0, 1);
	
	ob = b;
	intrclear(VBLANKbit, 0);
}

static void
ndsinit(void)
{
	intrenable(0, VBLANKbit, vblankintr, 0, 0);
	if (IPC->heartbeat > 1)
		print("touch worked\n");
}

static Chan*
ndsattach(char* spec)
{
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

	devreset,
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
