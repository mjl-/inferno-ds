#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"arm7/dat.h"
#include	"arm7/audio.h"

static int debug = 0;

enum
{
	Qdir		= 0,
	Qaudio,
	Qaudioctl,

	Fmono		= 1,
	Fin			= 2,
	Fout		= 4,

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
	"audioctl",	{Qaudioctl},		0,	0666,
};

static	struct
{
	QLock;
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

static TransferSound snd;
static TransferSoundData snddat;

static void
mxvolume(void)
{
	int bits = 8;

	snddat.rate = audio.lovol[Vspeed];
	snddat.vol = (Maxvol - Minvol) * audio.lovol[Vaudio] / 100;
	snddat.pan = (Maxvol - Minvol) * (audio.rovol[Vaudio] - audio.lovol[Vaudio]) / 100;
	snddat.fmt = (bits == 16);
}

static void
playaudio(const void* data, ulong length)
{
	snd.count = 1;
	snddat.data = data;
	snddat.len = length;
	memmove(&snd.d[0], &snddat, sizeof(TransferSoundData));

	dcflush(&snd, sizeof(TransferSound));
	IPC->soundData = &snd;
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
	audio.amode = Aclosed;
	resetlevel();
//	powerenable(audiopower);
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
audioopen(Chan *c, int omode)
{
	return devopen(c, omode, audiodir, nelem(audiodir), devgen);
}

static void
audioclose(Chan *c)
{
	USED(c);
}

static long
audioread(Chan *c, void *v, long n, vlong offset)
{
	int liv, riv, lov, rov;
	long m, n0;
	char buf[300], *p, *s, *e;

	p = v;
	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qdir:
		return devdirread(c, v, n, audiodir, nelem(audiodir), devgen);

	case Qaudio:
		break;

	case Qaudioctl:
		buf[0] = 0;
		s = buf;
		e = buf + 300;
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
	}

	return n;
}

static long
audiowrite(Chan *c, void *vp, long n, vlong)
{
	long m, n0;
	int i, nf, v, left, right, in, out;
	char buf[255], *field[Ncmd];
	char *p;
	int bits, chans; /* TODO currently ignored */

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
//					if(archaudiospeed(m, 0) < 0)
//						error(Evolume);
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
				bits = strtol(field[i], 0, 0);
				if(bits != 8 && bits != 16)
					error(Evolume);
				goto cont0;
			}
			if(strcmp(field[i], "chans") == 0) {
				if(++i >= nf)
					error(Evolume);
				chans = strtol(field[i], 0, 0);
				if(chans != 1 && chans != 2)
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
