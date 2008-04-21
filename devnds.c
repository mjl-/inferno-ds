#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"lcdreg.h"
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
	Qfw,
	Qrom,
	Qsram,
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
	"touchctl",	{Qtouchctl},	0,		0644,
	"ndsfw",	{Qfw},		0,		0400,
	"ndsrom",	{Qrom},		0,		0600,
	"ndssram",	{Qsram},		0,		0600,
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
	{'\n', 0x7f, '\t', Esc, Right, Left, Up, Down, RCtrl, RShift, Pgup, Pgdown, No},	// right handed
	{'\n', 0x7f, '\t', Esc, Right, Left, Up, Down, RCtrl, RShift, Pgup, Pgdown, No},	// left handed
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
		if (i == Lbtn || i == Rbtn)
			continue;
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
vblankintr(Ureg *, void *)
{
	int b;
	static ulong ob;
	int x, y;
	PERSONAL_DATA *pd = PersonalData;

	b = ndskeys();

#ifdef ARMDIVFIXED
	if(b)
		mousetrack(b, IPC->touchXpx, IPC->touchYpx, 0);
	else if(ob)
		mousetrack(b, 0, 0, 1);
#endif

	if(b) {
		x = (int)((IPC->touchX - (int)pd->calX1) * ((int)pd->calX2px - (int)pd->calX1px) / ((int)pd->calX2 - (int)pd->calX1)) + pd->calX1px - 1;
		y = (int)((IPC->touchY - (int)pd->calY1) * ((int)pd->calY2px - (int)pd->calY1px) / ((int)pd->calY2 - (int)pd->calY1)) + pd->calY1px - 1;

		if(x < 0)
			x = 0;
		else if(x > SCREEN_WIDTH-1)
			x = SCREEN_WIDTH-1;
		if(y < 0)
			y = 0;
		else if(y > SCREEN_HEIGHT-1)
			y = SCREEN_HEIGHT-1;

		mousetrack(b, x, y, 0);
	} else if(ob)
		mousetrack(b, 0, 0, 1);
	
	ob = b;
	intrclear(VBLANKbit, 0);
}

static void
ndsinit(void)
{
	// TODO complete ndstab[Qrom].length,
	// to respect mem available in the ROM cartridge
	// add checks to see if we're not overriding anything valuable

	intrenable(0, VBLANKbit, vblankintr, 0, 0);
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


/* settings flags */
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
	Langmask	= 7,
	Gbalowerscreen	= 1<<3,
	Backlightshift	= 4,
	Backlightmask	= 3,
	Autostart	= 1<<6,
	Nosettings	= 1<<9,
};

static long
ndsread(Chan* c, void* a, long n, vlong offset)
{
	char *tmp, buf[64];
	uchar reply[12];
	int v, t, l;
	char *p, *e;
	int i;
	PERSONAL_DATA *pd = PersonalData;
	Rune name[nelem(pd->name)+1];
	Rune message[nelem(pd->message)+1];

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
		l = IPC->temperature;
		t = *((uchar*)0x1d); // read console type, version from firmware
		snprint(tmp, READSTR,
			"ds type: %x %s\n"
			"battery: %d %s\n"
			"temperature: %d.%d\n",
			t, (t == 0xff? "NDS" : "NDS-lite"), 
			v, (v? "high" : "low"),
			l>>12, l & (1<<13 -1));

		n = readstr(offset, a, n, tmp);
		poperror();
		free(tmp);
		break;
	case Qfw:
		tmp = p = malloc(1024);
		if(waserror()) {
			free(tmp);
			nexterror();
		}
		e = tmp+1024;

		memmove(name, pd->name, sizeof pd->name);
		name[pd->nameLen] = 0;
		memmove(message, pd->message, sizeof pd->message);
		message[pd->messageLen] = 0;
		p = seprint(p, e, "version %d color %d birthmonth %d birthday %d\n", pd->version, pd->theme, pd->birthMonth, pd->birthDay);
		p = seprint(p, e, "nick %S\n", name);
		p = seprint(p, e, "msg %S\n", message);
		p = seprint(p, e, "alarm hour %d min %d on %d\n", pd->alarmHour, pd->alarmMinute, pd->alarmOn);
		p = seprint(p, e, "adc1 x %d y %d, adc2 x %d y %d\n", pd->calX1, pd->calY1, pd->calX2, pd->calY2);
		p = seprint(p, e, "scr1 x %d y %d, scr2 x %d y %d\n", pd->calX1px, pd->calY1px, pd->calX2px, pd->calY2px);
		p = seprint(p, e, "flags 0x%02ux lang %d backlight %d", pd->flags,
			pd->flags&Langmask, (pd->flags>>Backlightshift)&Backlightmask);
		if(pd->flags & Gbalowerscreen)
			seprint(p, e, " gbalowerscreen");
		if(pd->flags & Autostart)
			seprint(p, e, " autostart");
		if(pd->flags & Nosettings)
			seprint(p, e, " nosettings");
		seprint(p, e, "\n");

		n = readstr(offset, a, n, tmp);

		poperror();
		free(tmp);

		break;

	case Qrom:
		memmove(a, (void*)(ROMZERO+offset), n);
		break;
	case Qsram:
		memmove(a, (void*)(SRAMZERO+offset), n);
		break;
		
		
	default:
		n=0;
		break;
	}
	return n;
}

static long
ndswrite(Chan* c, void* a, long n, vlong offset)
{
	char cmd[64], op[32], *fielnds[6];
	int nf;

	switch((ulong)c->qid.path){
	case Qctl:
		break;
	case Qtouchctl:
		break;
//		return touchctl(a, n);

	case Qrom:
		memmove((void*)(ROMZERO+offset), a, n);
		break;
	case Qsram:
		memmove((void*)(SRAMZERO+offset), a, n);
		break;
		
		
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
