#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"arm7/audio.h"

static int debug = 0;

#define	DPRINT	if(debug)print

enum
{
	Qdir		= 0,
	Qaudio,
	Qvolume,
	Qstatus,
	Qaudioctl,

	Fmono		= 1,
	Fin			= 2,
	Fout		= 4,
	F16bits		= 8,
	
	Aclosed		= 0,
	Aread,
	Awrite,

	Vaudio		= 0,
	Vmic,
	Vtreb,
	Vbass,
	Vspeed,
	Vfilter,
	Vinvert,
	Nvol,

	Bufsize		= 4*1024,	/* 46 ms each */
	Nbuf		= 32,		/* 1.5 seconds total */

	Speed		= 44100,
	Ncmd		= 50,		/* max volume command words */
};

static
Dirtab audiodir[] =
{
	".",		{Qdir, 0, QTDIR},	0,	0555,
	"audio",	{Qaudio},		0,	0666,
	"volume",	{Qvolume},		0,	0666,
	"audiostat",	{Qstatus},		0,	0444,
	"audioctl",	{Qaudioctl},		0,	0666,
};

static	struct
{
	QLock;
	int		flags;
	int		amode;			/* Aclosed/Aread/Awrite for /audio */
	int		intr;			/* boolean an interrupt has happened */
	int		rivol[Nvol];	/* right/left input/output volumes */
	int		livol[Nvol];
	int		rovol[Nvol];
	int		lovol[Nvol];
	uvlong	totcount;		/* how many bytes processed since open */
	vlong	tottime;		/* time at which totcount bytes were processed */
	int	clockout;	/* need steady output to provide input clock */
} audio;

static	struct
{
	char*	name;
	int	flag;
	int	ilval;		/* initial values */
	int	irval;
} volumes[] =
{
[Vaudio]	{"audio",	Fout|Fmono,	 80,	 80},
[Vmic]		{"mic",		Fin|Fmono,	  0,	  0},
[Vtreb]		{"treb",	Fout|Fmono,	 50,	 50},
[Vbass]		{"bass",	Fout|Fmono, 	 50,	 50},
[Vspeed]	{"speed",	Fin|Fout|Fmono,	Speed,	Speed},
[Vfilter]	{"filter",	Fout|Fmono,	  0,	  0},
[Vinvert]	{"invert",	Fin|Fout|Fmono,	  0,	  0},
[Nvol]		{0}
};

static	char	Emode[]		= "illegal open mode";
static	char	Evolume[]	= "illegal volume specifier";

/* TODO: use the 16 sound channels available */
static int chan = 1;			/* audio sound channel */
static TxSound Snd[NSChannels+1];	/* play and record */
static TxSound* snd[NSChannels+1];

static void
mxvolume(void)
{
	int rov, lov;
	
	rov = audio.rovol[Vaudio];
	lov = audio.lovol[Vaudio];
	
	snd[chan]->chan = (audio.flags & Fmono? 1: 2);
	snd[chan]->rate = audio.lovol[Vspeed];
	snd[chan]->vol = (Maxvol - Minvol) * (lov + rov) / (2*100);
	snd[chan]->pan = (Maxvol - Minvol) * rov / (2*lov+1);
	snd[chan]->fmt = audio.flags & F16bits;
}

static void
playaudio(void* d, ulong n)
{
	snd[chan]->d = d;
	snd[chan]->n = n;
	snd[chan]->chan = chan;
	fifoput(F9TAudio|F9Auplay, (ulong)snd[chan]);
}

static void
recaudio(void* d, ulong n)
{
	memmove(snd[0], snd[chan], sizeof(TxSound));
	snd[0]->d = d;
	snd[0]->n = n;
	fifoput(F9TAudio|F9Aurec, (ulong)snd[0]);
	while(snd[0]->d != nil)
		;
}

static void
resetlevel(void)
{
	int i;

	for(i=0; volumes[i].name; i++) {
		audio.lovol[i] = volumes[i].ilval;
		audio.rovol[i] = volumes[i].irval;
		audio.livol[i] = volumes[i].ilval;
		audio.rivol[i] = volumes[i].irval;
	}
}

static void
audioinit(void)
{
	int i;

	audio.amode = Aclosed;
	resetlevel();

	for(i=0; i< NSChannels+1; i++)
		snd[i] = (TxSound*)uncached(&Snd[i]);
}

static Chan*
audioattach(char *param)
{
	return devattach('A', param);
}

static Walkqid*
audiowalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, audiodir, nelem(audiodir), devgen);
}

static int
audiostat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, audiodir, nelem(audiodir), devgen);
}

static Chan*
audioopen(Chan *c, int mode)
{
	int omode = mode;

	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qstatus:
		if((omode&7) != OREAD)
			error(Eperm);
	case Qvolume:
	case Qaudioctl:
	case Qdir:
		break;

	case Qaudio:
		omode = (omode & 0x7) + 1;
		if (omode & ~(Aread | Awrite))
			error(Ebadarg);
		qlock(&audio);
		if(audio.amode & omode){
			qunlock(&audio);
			error(Einuse);
		}
		
		if(omode & Aread) {
			fifoput(F9TAudio|F9Aupowerin, 1);
			delay(30);	/* wait at least 15 ms */
			audio.amode |= Aread;
		}
		if(omode & Awrite) {
			fifoput(F9TAudio|F9Aupowerout, 1);
			delay(30);	/* wait at least 15 ms */
			audio.amode |= Awrite;
		}
		mxvolume();
		qunlock(&audio);
		DPRINT("#A: open\n");
		break;
	}

	c = devopen(c, mode, audiodir, nelem(audiodir), devgen);
	c->mode = openmode(mode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
audioclose(Chan *c)
{
	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qdir:
	case Qvolume:
	case Qaudioctl:
	case Qstatus:
		break;

	case Qaudio:
		DPRINT("#A: close\n");
		if(c->flag & COPEN) {
			qlock(&audio);
			if(waserror()){
				qunlock(&audio);
				nexterror();
			}
			if (c->mode == OWRITE || c->mode == ORDWR) {
				/* closing the write end */
				audio.amode &= ~Awrite;
			}
			if (c->mode == OREAD || c->mode == ORDWR) {
				/* closing the read end */
				audio.amode &= ~Aread;
			}
			if (audio.amode == 0) {
				/* turn audio off */
				DPRINT("#A: audio off");
				//fifoput(F9TAudio|F9Aupowerin, 0);
				//fifoput(F9TAudio|F9Aupowerout, 0);
			}
			qunlock(&audio);
			poperror();
		}
		break;
	}
}

static long
audioread(Chan *c, void *v, long n, vlong offset)
{
	int liv, riv, lov, rov;
	long m;
	char buf[300], *p, *s, *e;

	p = v;
	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qdir:
		return devdirread(c, v, n, audiodir, nelem(audiodir), devgen);

	case Qaudioctl:
		buf[0] = 0;
		s = buf;
		e = buf + sizeof(buf);
		for(m=0; volumes[m].name; m++){
			liv = audio.livol[m];
			riv = audio.rivol[m];
			lov = audio.lovol[m];
			rov = audio.rovol[m];
			s = seprint(s, e, "%s", volumes[m].name);
			if((volumes[m].flag & Fmono) || liv==riv && lov==rov){
				if((volumes[m].flag&(Fin|Fout))==(Fin|Fout) && liv==lov)
					s = seprint(s, e, " %d", liv);
				else{
					if(volumes[m].flag & Fin)
						s = seprint(s, e, " in %d", liv);
					if(volumes[m].flag & Fout)
						s = seprint(s, e, " out %d", lov);
				}
			}else{
				if((volumes[m].flag&(Fin|Fout))==(Fin|Fout) &&
				    liv==lov && riv==rov)
					s = seprint(s, e, " left %d right %d", liv, riv);
				else{
					if(volumes[m].flag & Fin)
						s = seprint(s, e, " in left %d right %d", liv, riv);
					if(volumes[m].flag & Fout)
						s = seprint(s, e, " out left %d right %d", lov, rov);
				}
			}
			s = seprint(s, e, "\n");
		}
		return readstr(offset, p, n, buf);
		break;

	case Qaudio:
		if((audio.amode & Aread) == 0)
			error(Emode);
		recaudio(p, n);
		break;
	}

	return n;
}

static long
audiowrite(Chan *c, void *vp, long n, vlong)
{
	long m;
	int i, nf, v, left, right, in, out;
	char buf[255], *field[Ncmd];
	char *p;

	p = vp;
	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qaudioctl:
		v = Vaudio;
		left = 1;
		right = 1;
		in = 1;
		out = 1;
		if(n > sizeof(buf)-1)
			n = sizeof(buf)-1;
		memmove(buf, p, n);
		buf[n] = '\0';
		n = 0;

		nf = getfields(buf, field, Ncmd, 1, " \t\n");
		for(i = 0; i < nf; i++){
			/*
			 * a number is volume
			 */
			if(field[i][0] >= '0' && field[i][0] <= '9') {
				m = strtoul(field[i], 0, 10);
				if(v == Vspeed){
					; /* ignored as m specifies speed */
				}else
					if(m < 0 || m > 100)
						error(Evolume);
	
				if(left && out)
					audio.lovol[v] = m;
				if(left && in)
					audio.livol[v] = m;
				if(right && out)
					audio.rovol[v] = m;
				if(right && in)
					audio.rivol[v] = m;
				goto cont0;
			}
			if(strcmp(field[i], "rate") == 0)
				field[i] = "speed";	/* honestly ... */

			for(m=0; volumes[m].name; m++) {
				if(strcmp(field[i], volumes[m].name) == 0) {
					v = m;
					in = 1;
					out = 1;
					left = 1;
					right = 1;
					goto cont0;
				}
			}
			if(strcmp(field[i], "enc") == 0) {
				if(++i >= nf)
					error(Evolume);
				if(strcmp(field[i], "pcm") != 0)
					error(Evolume);
				goto cont0;
			}
			if(strcmp(field[i], "bits") == 0) {
				if(++i >= nf)
					error(Evolume);
				m = strtol(field[i], 0, 0);
				if(m == 8) 
					audio.flags &= ~F16bits;
				else if (m == 16)
					audio.flags |= F16bits;
				else
					error(Evolume);
				goto cont0;
			}
			if(strcmp(field[i], "chans") == 0) {
				if(++i >= nf)
					error(Evolume);
				m = strtol(field[i], 0, 0);
				if (m == 1)
					audio.flags |= Fmono;
				else if (m == 2)
					audio.flags &= ~Fmono;
				else
					error(Evolume);
				goto cont0;
			}
			if(strcmp(field[i], "debug") == 0) {
				debug = debug?0:1;
				goto cont0;
			}
			if(strcmp(field[i], "in") == 0) {
				in = 1;
				out = 0;
				goto cont0;
			}
			if(strcmp(field[i], "out") == 0) {
				in = 0;
				out = 1;
				goto cont0;
			}
			if(strcmp(field[i], "left") == 0) {
				left = 1;
				right = 0;
				goto cont0;
			}
			if(strcmp(field[i], "right") == 0) {
				left = 0;
				right = 1;
				goto cont0;
			}
			error(Evolume);
			break;
		cont0:;
		}

		mxvolume();
		break;

	case Qaudio:
		if((audio.amode & Awrite) == 0)
			error(Emode);
		playaudio(p, n);
		break;
	}
	return n;
}

Dev audiodevtab = {
	'A',
	"audio",

	devreset,
	audioinit,
	devshutdown,
	audioattach,
	audiowalk,
	audiostat,
	audioopen,
	devcreate,
	audioclose,
	audioread,
	devbread,
	audiowrite,
	devbwrite,
	devremove,
	devwstat,
};
