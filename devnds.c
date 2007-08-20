#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include 	"arm7/jtypes.h"
#include	"arm7/ipc.h"
#include	"arm7/touch.h"
#include	"arm7/system.h"

enum {
	Qdir,
	Qctl,
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
	int v, p, l;

	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, ndstab, nelem(ndstab), devgen);
	case Qtouchctl:
		break;
	case Qbattery:
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
enum {
	Rbut=1<<8,
	Abut=1,
	Bbut=1<<1,
};
static void
touchread(void*)
{
	int x=0, y=0, dx, dy, buttons, b, oldb=0, n=0;
	for(;;) {
	//	print("touchread iter\n");
	//	if(!(IPC->buttons&Pendown))
	//		continue; // should sleep until the pen is down
	//	dx=IPC->touchXpx-x;
	//	dy=IPC->touchYpx-y;
	/* have an option for handedness here? right now left handed */
		buttons=~((IPC->buttons&Pendown)>>6|(REG_KEYINPUT&Rbut)>>6|(REG_KEYINPUT&Abut)<<1)&0x7;
		if((b=!(REG_KEYINPUT&Bbut))^oldb && !n--) {
			kbdputc(kbdq,'\n');
			b=oldb;
			n=500;
		}
		mousetrack(buttons, IPC->touchXpx, IPC->touchYpx, 0);
	//	x=IPC->touchXpx;
	//	y=IPC->touchYpx;
	}
}
TransferRegion  *getIPC() {
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
