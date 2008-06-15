#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	<keyboard.h>

enum {
	Qdir,
	Qregs,
};

static
Dirtab regstab[]={
	".",		{Qdir, 0, QTDIR},	0, 0555,
	"registers",	{Qregs},		0, 0600,
};

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

static Chan*
regsattach(char* spec)
{
	return devattach('R', spec);
}

static Walkqid*
regswalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, regstab, nelem(regstab), devgen);
}

static int
regsstat(Chan* c, uchar *db, int n)
{
	return devstat(c, db, n, regstab, nelem(regstab), devgen);
}

static Chan*
regsopen(Chan* c, int omode)
{
	return devopen(c, omode, regstab, nelem(regstab), devgen);
}


static void
regsclose(Chan*)
{
}

static long
regsread(Chan* c, void* a, long n, vlong offset)
{
	uchar b;
	ushort s;
	ulong l;
	uchar *p;

	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, regstab, nelem(regstab), devgen);
	case Qregs:
		p = a;
		switch(n) {
		case 1:
			b = inb(offset);
			p[0] = b;
			break;
		case 2:
			s = insh(offset);
			p[0] = s>>0;
			p[1] = s>>8;
			break;
		case 4:
			l = inl(offset);
			p[0] = l>>0;
			p[1] = l>>8;
			p[2] = l>>16;
			p[3] = l>>24;
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
regswrite(Chan* c, void* a, long n, vlong off)
{
	uchar *p;
	uchar b;
	ushort s;
	ulong l;

	switch((ulong)c->qid.path){
	case Qregs:
		p = a;
		switch(n) {
		case 1:
			b = p[0];
			outb(off, b);
			break;
		case 2:
			s = (p[1]<<8)|p[0];
			outsh(off, s);
			break;
		case 4:
			l = (p[3]<<24)|(p[2]<<16)|(p[1]<<8)|p[0];
			outl(off, l);
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

Dev regsdevtab = {
	'R',
	"regs",

	devreset,
	devinit,
	devshutdown,
	regsattach,
	regswalk,
	regsstat,
	regsopen,
	devcreate,
	regsclose,
	regsread,
	devbread,
	regswrite,
	devbwrite,
	devremove,
	devwstat,
};
