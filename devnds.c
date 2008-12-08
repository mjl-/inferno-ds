#include	<u.h>
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	<keyboard.h>

#include 	"arm7/dat.h"

#define	DPRINT if(0)print

enum {
	Qdir,
	Qctl,
	Qinfo,
	Qfw,
	Qrom,
	Qsram,
	Qmem,
};

static
Dirtab ndstab[]={
	".",		{Qdir, 0, QTDIR},		0,		0555,
	"ndsctl",	{Qctl},		0,		0600,
	"ndsinfo",	{Qinfo},	0,		0600,
	"ndsfw",	{Qfw},		0,		0400,
	"ndsrom",	{Qrom},		0,		0600,
	"ndssram",	{Qsram},	0,		0600,
	"ndsmem",	{Qmem},		0,		0600,
};

static	Rune	rockermap[2][Numbtns] ={	/* right and left handed */
	{ '\n', '\b', '\t', Esc, Right, Left, Up, Down, RCtrl, RShift, Pgup, Pgdown, No },
	{ Left, Down, '\t', Esc, Pgdown, '\n', Pgup, '\b', RCtrl, RShift, Up, Right, No },
};

void
fiforecv(ulong vv)
{
	ulong v;
	static uchar mousemod = 0;
	static uchar lcdyoff = 0;
	int i;

	v = vv>>Fcmdlen;
	switch(vv&Fcmdmask) {
	case F7keyup:
		if(v&(1<<Lclose))
			blankscreen(0);

		if(v&(1<<Pdown))
			mousemod &= 1<<0;
		if(v&(1<<Rbtn))
			mousemod &= 1<<1;
		if(v&(1<<Lbtn))
			mousemod &= 1<<2;

		if(v&1<<Selbtn && v&1<<Startbtn)
			lcdyoff = lcdyoff > 0? 0: Scrheight;

		for(i = 0; i < nelem(rockermap[conf.bmap]); i++)
			if (i==Rbtn||i==Lbtn){
				continue;
			}else if(v & (1<<i)){
				kbdrepeat(0);
				kbdputc(kbdq, rockermap[conf.bmap][i]);
			}	
		break;
	case F7keydown:
		if(v&(1<<Pdown))
			mousemod |= 1<<0;
		if(v&(1<<Rbtn))
			mousemod |= 1<<1;
		if(v&(1<<Lbtn))
			mousemod |= 1<<2;

		if(v&(1<<Lclose))
			blankscreen(1);
		break;
	case F7mousedown:
		//print("mousedown %lux %lud %lud %lud\n", v, v&0xff, (v>>8)&0xff, mousemod);
		mousetrack(mousemod, v&0xff, lcdyoff+((v>>8)&0xff), 0);
		break;
	case F7mouseup:
		mousetrack(0, 0, 0, 1);
		break;
	case F7print:
		print("%s", (char*) v); /* just forward arm7 prints */
		break;

	default:
		print("F9rx err %lux\n", vv);
		break;
	}
}

static void
ndsinit(void)
{
	NDShdr *dsh;
	ulong hassram;
	ulong *p;
	
	dsh = nil;
	/* look for a valid NDShdr */
	if (memcmp((void*)((NDShdr*)ROMZERO)->gcode, "INFRME", 6) == 0)
		dsh =  (NDShdr*)ROMZERO;
	else if (memcmp((void*)NDSHeader->gcode, "INFRME", 6) == 0)
		dsh = NDSHeader;

	if(dsh == nil)
		return;

	/* check before overriding anyone else's memory */
	hassram = (memcmp((void*)dsh->rserv5, "PASS", 4) == 0) &&
		  (memcmp((void*)dsh->rserv4, "SRAM_V", 6) == 0);

	conf.bsram = SRAMTOP;
	if (hassram)
		conf.bsram = SRAMZERO;
		
	/* BUG: rom only present on certain slot2 devices */
	if (0 && dsh == (NDShdr*)ROMZERO){
		for (p=(ulong*)(ROMZERO+dsh->appeoff); p < (ulong*)(ROMTOP); p++)
			if (memcmp(p, "ROMZERO9", 8) == 0)
				break;

		conf.brom = ROMTOP;
		if (p < (ulong*)(ROMTOP - sizeof("ROMZERO9") - 1))
			conf.brom = (ulong)p + sizeof("ROMZERO9") - 1;
	}
	
	DPRINT("ndsinit: hdr %08lx sram %08x rom %08x\n",
		dsh, conf.bsram, conf.brom);
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

/* in/out used to access Qmem */

static uchar
inb(ulong reg)
{
	return *(uchar*)reg;
}

static ushort
insh(ulong reg)
{
	return *(ushort*)reg;
}

static ulong
inl(ulong reg)
{
	return *(ulong*)reg;
}

static void
outb(ulong reg, uchar b)
{
	*(uchar*)reg = b;
}

static void
outsh(ulong reg, ushort sh)
{
	*(ushort*)reg = sh;
}

static void
outl(ulong reg, ulong l)
{
	*(ulong*)reg = l;
}

static long
ndsread(Chan* c, void* a, long n, vlong offset)
{
	char *tmp, *p, *e;
	int v, t, temp, len;
	UserInfo *pu = UINFOREG;
	uchar *pa;
	uchar b;
	ushort s;
	ulong l;

	char *langlst[Langmask+1] = {
		[LJapanese] "Japanese",
		[LEnglish] "English",
		[LFrench] "French",
		[LGerman] "German",
		[LItalian] "Italian",
		[LSpanish] "Spanish",
		[LChinese] "Chinese",
		[LOther] "Other",
	};

	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, ndstab, nelem(ndstab), devgen);

	case Qinfo:
		tmp = malloc(READSTR);
		if(waserror()){
			free(tmp);
			nexterror();
		}
		v = IPC->batt;
		temp = IPC->temp;
		t = 0xff;
		snprint(tmp, READSTR,
			"ds type: %x %s\n"
			"battery: %d %s\n"
			"temp: %d.%.2d\n",
			t, (t == 0xff? "ds" : "ds-lite"), 
			v, (v? "low" : "high"),
			temp>>12, temp & ((1<<13) -1));

		n = readstr(offset, a, n, tmp);
		poperror();
		free(tmp);
		break;

	case Qfw:
		tmp = p = malloc(READSTR);
		if(waserror()) {
			free(tmp);
			nexterror();
		}
		e = tmp+READSTR;

		p = seprint(p, e, "version %d theme %d birth %02d/%02d\n", pu->version, pu->theme, pu->birthday, pu->birthmonth);
		p = seprint(p, e, "nick %.*S\n", pu->namelen, pu->name);
		p = seprint(p, e, "msg %.*S\n", pu->msglen, pu->msg);
		p = seprint(p, e, "alarm %s [%.2d:%.2d]\n", pu->alarmon? "on": "off", pu->alarmhour, pu->alarmmin);
		p = seprint(p, e, "adc t1 (%d,%d) t2 (%d,%d)\n", pu->adc.x1, pu->adc.y1, pu->adc.x2, pu->adc.y2);
		p = seprint(p, e, "scr t1 (%d,%d) t2 (%d,%d)\n", pu->adc.xpx1, pu->adc.ypx1, pu->adc.xpx2, pu->adc.ypx2);
		p = seprint(p, e, "flags 0x%02ux: lang %s blight %d %s %s %s\n", 
			pu->flags,
			langlst[pu->flags&Langmask],
			(pu->flags>>Blightshift)&Blightmask,
			(pu->flags & Gbalowerscreen)? "gbalowerscreen": "",
			(pu->flags & Autostart)? "autostart": "",
			(pu->flags & Nosettings)? "nosettings": "");

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
 		memmove(a, (void*)(conf.brom+offset), n);
 		break;

 	case Qsram:
		len = SRAMTOP - conf.bsram;
		if(offset < 0 || offset >= len)
			return 0;
		if(offset+n > len)
			n = len - offset;
 		memmove(a, (uchar*)(conf.bsram+offset), n);
		break;

	case Qmem:
		pa = a;
		switch(n) {
		case 1:
			b = inb(offset);
			pa[0] = b;
			break;
		case 2:
			s = insh(offset);
			pa[0] = s>>0;
			pa[1] = s>>8;
			break;
		case 4:
			l = inl(offset);
			pa[0] = l>>0;
			pa[1] = l>>8;
			pa[2] = l>>16;
			pa[3] = l>>24;
			break;
		default:
			error(Ebadusefd);
		}
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
	uchar *pa;

	switch((ulong)c->qid.path){
	case Qctl:
		if(n >= sizeof cmd)
			error(Etoobig);
		memmove(cmd, a, n);
		cmd[n] = 0;
		nf = getfields(cmd, fields, nelem(fields), 1, " \n");
		if(nf != 2)
			error(Ebadarg);
		if(strcmp(fields[0], "blight") == 0) {
			fifoput(F9TSystem|F9Sysbright, atoi(fields[1]) & Blightmask);
		} else if(strcmp(fields[0], "lcd") == 0) {
			if(strcmp(fields[1], "on") == 0)
				blankscreen(1);
			else if(strcmp(fields[1], "off") == 0)
				blankscreen(0);
			else
				error(Ebadarg);
		} else if(strcmp(fields[0], "screens") == 0) {
			conf.screens = atoi(fields[1]);	
		} else
			error(Ebadarg);
 		break;

 	case Qrom:
		len = ROMTOP - conf.brom;
		if(offset < 0 || offset >= len)
			return 0;
		if(offset+n > len)
			n = len - offset;
		memmove((void*)(conf.brom+offset), a, n);
		DPRINT("rom w %llux off %lld n %ld\n", (conf.brom+offset), offset, n);
 		break;

 	case Qsram:
		len = SRAMTOP - conf.bsram;
		if(offset < 0 || offset >= len)
			return 0;
		if(offset+n > len)
			n = len - offset;
 		memmove((uchar*)(conf.bsram+offset), a, n);
		DPRINT("sram w %llux off %lld n %ld\n", (conf.bsram+offset), offset, n);
 		break;

	case Qmem:
		pa = a;
		switch(n) {
		case 1:
			outb(offset, pa[0]);
			break;
		case 2:
			outsh(offset, (pa[1]<<8)|pa[0]);
			break;
		case 4:
			outl(offset, (pa[3]<<24)|(pa[2]<<16)|(pa[1]<<8)|pa[0]);
			break;
		default:
			error(Ebadusefd);
		}
		break;

	default:
		error(Ebadusefd);
	}
	return n;
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
