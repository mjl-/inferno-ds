#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"arm7/jtypes.h"
#include	"arm7/ipc.h"

enum
{
	Qdir		= 0,
	Qaudio,
	Qvolume,
};

static
Dirtab audiodir[] =
{
	".",		{Qdir, 0, QTDIR},	0,	0555,
	"audio",	{Qaudio},		0,	0666,
	"volume",	{Qvolume},		0,	0666,
};

/*
//	Clean and invalidate a range
TEXT DC_FlushRange(SB), $-4
	ADD	R0, R1, R1
	BIC	$(CACHELINESZ - 1), R0, R0
.flush:
	MCR	15, 0, R0, C7, C14, 1
	ADD	$CACHELINESZ, R0
	CMP	R0, R1
	BLT	.flush
	RET
*/
void	DC_FlushRange(const void *base, u32 size);
static void playSoundBlock(TransferSound *snd) {
//	DC_FlushRange(snd, sizeof(TransferSound) );

	IPC->soundData = snd;
}

static TransferSound Snd;
static TransferSoundData SndDat =		{ (void *)0 , 0, 11025, 64, 64, 1 };

void setGenericSound( u32 rate, u8 vol, u8 pan, u8 format) {
	SndDat.rate		= rate;
	SndDat.vol		= vol;
	SndDat.pan		= pan;
	SndDat.format	= format;
}

void playSound( TransferSoundData* sound) {
	Snd.count = 1;

	memcpy(&Snd.data[0], sound, sizeof(TransferSoundData) );

	playSoundBlock(&Snd);
}

void playGenericSound(const void* data, u32 length) {
	Snd.count = 1;

	memcpy(&Snd.data[0], &SndDat, sizeof(TransferSoundData) );
	Snd.data[0].data = data;
	Snd.data[0].len = length;

	playSoundBlock(&Snd);
}

// raw pcm audio sample
extern int invhitlen;
extern char invhitcode[];

static void
audioinit(void){
	print("audioinit ...\n");
//	memset(IPC->soundData, 0, sizeof(TransferSound));

	setGenericSound(11127, 0x7F, 0, 1);
	playGenericSound(invhitcode, invhitlen);
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
}

static long
audioread(Chan *c, void *vp, long n, vlong offset)
{
	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qdir:
		return devdirread(c, vp, n, audiodir, nelem(audiodir), devgen);

	case Qaudio:
		break;

	case Qvolume:
		break;
	}
	return n;
}

static long
audiowrite(Chan *c, void *vp, long n, vlong)
{
	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qvolume:
		break;

	case Qaudio:
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
