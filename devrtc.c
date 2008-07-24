#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"io.h"

enum{
	Qdir,
	Qrtc,
};

static Dirtab rtctab[]={
	".",		{Qdir,0,QTDIR},	0,	0555,
	"rtc",		{Qrtc},	NUMSIZE,	0664,
};

extern ulong boottime;

static void
rtcreset(void)
{
}

static Chan*
rtcattach(char *spec)
{
	return devattach('r', spec);
}

static Walkqid*
rtcwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, rtctab, nelem(rtctab), devgen);
}

static int
rtcstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, rtctab, nelem(rtctab), devgen);
}

static Chan*
rtcopen(Chan *c, int omode)
{
	return devopen(c, omode, rtctab, nelem(rtctab), devgen);
}

static void	 
rtcclose(Chan*)
{
}

static long	 
rtcread(Chan *c, void *buf, long n, vlong off)
{
	ulong secs;

	if(c->qid.type & QTDIR)
		return devdirread(c, buf, n, rtctab, nelem(rtctab), devgen);

	switch((ulong)c->qid.path){
	case Qrtc:
		secs = 1;
		fifoput(F9TSystem|F9Sysrrtc, (ulong)(&secs));
		while(secs == 1); /* wait for arm7 write */
		return readnum(off, buf, n, secs, NUMSIZE);
	}
	error(Egreg);
	return 0;		/* not reached */
}

static long	 
rtcwrite(Chan *c, void *buf, long n, vlong off)
{
	ulong offset = off;
	ulong secs;
	char *cp, sbuf[32];

	switch((ulong)c->qid.path){
	case Qrtc:
		/*
		 *  write the time
		 */
		if(offset != 0 || n >= sizeof(sbuf)-1)
			error(Ebadarg);
		memmove(sbuf, buf, n);
		sbuf[n] = '\0';
		cp = sbuf;
		while(*cp){
			if(*cp>='0' && *cp<='9')
				break;
			cp++;
		}
		secs = strtoul(cp, 0, 0);
		fifoput(F9TSystem|F9Syswrtc, (ulong)(&secs));
		return n;

	}
	error(Egreg);
	return 0;		/* not reached */
}

Dev rtcdevtab = {
	'r',
	"rtc",

	rtcreset,
	devinit,
	devshutdown,
	rtcattach,
	rtcwalk,
	rtcstat,
	rtcopen,
	devcreate,
	rtcclose,
	rtcread,
	devbread,
	rtcwrite,
	devbwrite,
	devremove,
	devwstat,
};
