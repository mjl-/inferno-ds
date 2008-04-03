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
	Btn7msk = (1<<7)  - 1,

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
	Lclose7	= 1<<3,	// lid closed
	Pdown7	= 1<<6,	// pen down
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

#define kpressed(k, os, ns) ((os & (k)) && !(ns & (k)))
#define kreleased(k, os, ns) (!(os & (k)) && (ns & (k)))
void setlcdblight(int on);

static ulong mousemod = 0;	/* updated by ndskeys to reflect mouse btns*/

static void
ndskeys(void)
{
	int i;
	ulong st;
	static ulong ost;

	st = (REG_KEYINPUT & Btn9msk);
	st |= (IPC->buttons & Btn7msk) << 10;

	if(kreleased(Pdown, ost, st))
		mousemod &= 1<<0;
	if(kpressed(Pdown, ost, st))
		mousemod |= 1<<0;

	if(kreleased(Pdown|Lbtn, ost, st))
		mousemod &= 1<<2;
	if(kpressed(Pdown|Lbtn, ost, st))
		mousemod |= 1<<2;

	if(kreleased(Pdown|Rbtn, ost, st))
		mousemod &= 1<<4;
	if(kpressed(Pdown|Rbtn, ost, st))
		mousemod |= 1<<4;

 	// lid controls lcd backlight
	if(kpressed(~Lclose, ost, st))
		setlcdblight(0);
	if(kreleased(~Lclose, ost, st))
		setlcdblight(1);

	for (i=0; ost != st && i<nelem(rockermap[conf.bmap]); i++){
		if (kpressed(1<<i, ost, st)){
			kbdrepeat(0);
			kbdputc(kbdq, rockermap[conf.bmap][i]);
		}else if (kreleased(1<<i, ost, st)){
			kbdrepeat(0);
		}
	}

	ost = st;
}

static void
keysintr(Ureg*, void*)
{
//	print("keysintr\n");
	intrclear(KEYbit, 0);
//	wakeup(&powerevent);
}

static void
vblankintr()
{
//	print("vblankintr\n");
	ndskeys();
	intrclear(VBLANKbit, 0);
}

static void
ndsinit(void)
{
	REG_KEYCNT = (1<<0) | (1<<1) | (1<<14);
	intrenable(0, KEYbit, keysintr, nil, 0);

	intrenable(0, VBLANKbit, vblankintr, 0, 0);
	if (IPC->heartbeat > 1)
		print("touch worked\n");
}

static Chan*
ndsattach(char* spec)
{
	kproc("touchread", touchread, nil, 0);
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

// todo use keyintr
static int
tsactivity0(void *arg){

	return (~IPC->buttons & Pdown7) == Pdown7;
}

static void
touchread(void*)
{
	int px, py, isdown;

	for(;;) {
		// pointer button events change with keys
		px=IPC->touchXpx;
		py=IPC->touchYpx;

		isdown=(~IPC->buttons & Pdown7);
		if(isdown)
			mousetrack(mousemod, px, py, 0);
		else
			mousetrack(0, 0, 0, 1);

		// should sleep until the pen is down
		tsleep(&up->sleep, tsactivity0, nil, 100);

		if(0)print("ts down %x %#d %d %X\n", isdown, px, py, mousemod);
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
