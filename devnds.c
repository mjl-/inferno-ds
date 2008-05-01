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

#define	DPRINT if(0)print

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


// TODO use Dbgbtn7 to enable debug; by toggling Conf.bmap = 2
static	Rune	rockermap[3][Numbtns] ={
	{'\n', 0x7f, '\t', Esc, Right, Left, Up, Down, RCtrl, RShift, Pgup, Pgdown, No},	// right handed
	{'\n', 0x7f, '\t', Esc, Right, Left, Up, Down, RCtrl, RShift, Pgup, Pgdown, No},	// left handed
	{'?', '|', Del, SysRq, Right, Left, Up, Down, RCtrl, RShift, Esc, No, No},	// debug
};

/*
 * TODO
 * - take care of changes in buttons, penup/pendown, rockermap, handedness
 * - screen orientation switch between landscape/portrait
 */

void
fiforecv(ulong vv)
{
	ulong v;
	static uchar mousemod = 0;
	int i;

	v = vv>>Fcmdwidth;
	switch(vv&Fcmdmask) {
	case F7keyup:
		if(v&(1<<Lclose))
			blankscreen(1);

		if(v&(1<<Pdown))
			mousemod &= Button1;
		if(v&(1<<Rbtn))
			mousemod &= Button2;
		if(v&(1<<Lbtn))
			mousemod &= Button3;

		for(i = 0; i < nelem(rockermap[conf.bmap]); i++)
			if (i==Rbtn||i==Lbtn)
				continue;
			else if(v & (1<<i))
				kbdputc(kbdq, rockermap[conf.bmap][i]);
		break;
	case F7keydown:
		if(v&(1<<Pdown))
			mousemod |= Button1;
		if(v&(1<<Rbtn))
			mousemod |= Button2;
		if(v&(1<<Lbtn))
			mousemod |= Button3;

		if(v&(1<<Lclose))
			blankscreen(0);
		break;
	case F7mousedown:
		// print("mousedown %lux %lud %lud %lud\n", v, v&0xff, (v>>8)&0xff, mousemod);
		mousetrack(mousemod, v&0xff, (v>>8)&0xff, 0);
		break;
	case F7mouseup:
		mousetrack(0, 0, 0, 1);
		break;
	case F7dbgprint:
		print("F7dbgprint: %s\n", (char*) v);
		break;
	default:
		break;
	}
}

static void
vblankintr(Ureg *, void *)
{
	/* TODO: update lcd/screen fb on vblank */
	intrclear(VBLANKbit, 0);
}

static void
ndsinit(void)
{
//	NDShdr *dsh = (NDShdr*)0x027FFE00;
	NDShdr *dsh = (NDShdr*)ROMZERO;
	ulong hasrom, hassram;
	char *p;

	// Map Game Cartridge memory to ARM9
	*((ulong*)EXMEMCNT) &= ~0x80;

	hassram = (memcmp((void*)dsh->rserv4, "SRAM_V", 6) == 0) &&
		(memcmp((void*)dsh->rserv5, "PASS", 4) == 0);
	if (hassram)
		conf.bsram = SRAMZERO;

	hasrom = (dsh->romhdrsz == 0x200);
	if (hasrom)
	for (p=(char*)ROMZERO+dsh->appeoff; p < (char*)ROMTOP; p++)
		if (memcmp(p, "ROMZERO9", 8) == 0)
			break;

	conf.brom = (ulong)p + 8;
	DPRINT("ROMZERO+appeoff: %08lux\n", ROMZERO+dsh->appeoff);
	print("ndsinit: sram %lud @ %08x rom %lud @ %08x\n",
		hassram, conf.bsram, hasrom, conf.brom);

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
	char *tmp;
	int v, t, l;
	char *p, *e;
	int len;
	PERSONAL_DATA *pd = PersonalData;

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

		p = seprint(p, e, "version %d color %d birthmonth %d birthday %d\n", pd->version, pd->theme, pd->birthMonth, pd->birthDay);
		p = seprint(p, e, "nick %.*S\n", pd->nameLen, pd->name);
		p = seprint(p, e, "msg %.*S\n", pd->messageLen, pd->message);
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
		len = ROMTOP - conf.brom;
		if(offset < 0 || offset >= len)
			return 0;
		if(offset+n > len)
			n = len - offset;
		DPRINT("ndsread rom w %llux off %lld n %ld\n", (conf.brom+offset), offset, n);
 		memmove(a, (void*)(conf.brom+offset), n);
 		break;

 	case Qsram:
		len = SRAMTOP - conf.bsram;
		if(offset < 0 || offset >= len)
			return 0;
		if(offset+n > len)
			n = len - offset;
 		memmove(a, (void*)(conf.bsram+offset), n);
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
	char cmd[64], *fields[6];
	int nf, len;

	switch((ulong)c->qid.path){
	case Qctl:
		if(n >= sizeof cmd)
			error(Etoobig);
		memmove(cmd, a, n);
		cmd[n] = 0;
		nf = getfields(cmd, fields, nelem(fields), 1, " \n");
		if(nf != 2)
			error(Ebadarg);
		if(strcmp(fields[0], "brightness") == 0)
			fifoput(F9brightness, atoi(fields[1]));
		else if(strcmp(fields[0], "lcd") == 0) {
			if(strcmp(fields[1], "on") == 0)
				blankscreen(0);
			else if(strcmp(fields[1], "off") == 0)
				blankscreen(1);
			else
				error(Ebadarg);
		} else
			error(Ebadarg);
 		break;
	case Qtouchctl:
		break;
//		return touchctl(a, n);

 	case Qrom:
		len = ROMTOP - conf.brom;
		if(offset < 0 || offset >= len)
			return 0;
		if(offset+n > len)
			n = len - offset;
		memmove((void*)(conf.brom+offset), a, n);
 		break;

 	case Qsram:
		/* check before overriding anyone else's memory */
		if (!conf.bsram)
			return 0;

		len = SRAMTOP - conf.bsram;
		if(offset < 0 || offset >= len)
			return 0;
		if(offset+n > len)
			n = len - offset;
 		memmove((void*)(conf.bsram+offset), a, n);

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
