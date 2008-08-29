/* 
 * DLDI access to SD/CF storage provided by homebrew hw,
 * written using the dldi.s code from Michael "Chishm".
 * 
 * use: a dldi patcher completes I/O fns, so we we can r/w,
 * tries to be simplest/sanest way of doing it i could come with.
 *
 * TODO: the dldi io functions (arm-elf code) patched by the nds loader
 * use the R11 (SB) without restoring it, which panics the kernel.
 *
 * At this moment the DLDIhdr is only used to detect the card type.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#define	DPRINT	if(0)print

enum{
	Qdir,
	Qctl,
	Qdata,
};

static
Dirtab dlditab[]={
	".",		{ Qdir, 0, QTDIR},	0,	0555,
	"ctl",		{ Qctl },			0,	0774,
	"data",		{ Qdata },			0,	0774,
};

enum
{
	DLDImagic=		0xBF8DA5ED,
	
	Fixall =		0x01,
	Fixglue =		0x02,
	Fixgot =		0x04,
	Fixbss =		0x08,	
	
	LHDRSZ	= 15,		/* argmax {log2(sizeof(h))}, h is a common DLDIhdr */
	SECTSZ	= 512		/* Ioifc fns work with SECTSZ sectors */
};

typedef struct DLDIhdr DLDIhdr;
struct DLDIhdr{
	ulong magic;		/* magic number */
	char magics[8];	/* magic string */
	uchar version;		/* version number */
	uchar dlogsz;		/* log2 size of driver */
	uchar sect2fix;		/* sections to fix */
	uchar slogsz;		/* log2 size of alloc space */
	char textid[48];
	
	ulong sdata, etext;	/* offsets to sections */
	ulong sglue, eglue;
	ulong sgot, egot;
	ulong sbss, ebss;

	struct Ioifc io;		/* here: sizeof(DLDIhdr) = 1<<7 bytes */
	
	/* sizeof(DLDIhdr) = 1<<LHDRSZ bytes */
	uchar space[(1<<LHDRSZ) - (1<<7)]; 
};

static int
nulliofunc(void){
	return 0;
}

static Ioifc io_none = {
	.type = "none",
	.caps = 0,
	
	nulliofunc,
	nulliofunc,
	(void*) nulliofunc,
	(void*) nulliofunc,
	nulliofunc,
	nulliofunc,
};

static
DLDIhdr hdr=
{
.magic = DLDImagic,
.magics = " Chishm",
.version = 1,
.dlogsz = LHDRSZ,
.sect2fix = 0,
.slogsz = LHDRSZ,
.textid = "no interface found",
	
.sdata = &hdr, .etext = 0, /* .etext = &hdr + 1<<LHDRSZ */
.sglue = 0, .eglue = 0,
.sgot = 0, .egot = 0,
.sbss = 0, .ebss = 0,	
	
.io =	{
	.type = "none",
	.caps = 0,
	
	nulliofunc,
	nulliofunc,
	(void*) nulliofunc,
	(void*) nulliofunc,
	nulliofunc,
	nulliofunc,
	}
};

static void
mbrtest(void)
{
	int ret;
	uchar sect[SECTSZ];

	memset(sect, 0, sizeof(sect));
	ret=hdr.io.read(0, sizeof(sect)/SECTSZ, sect);
	if (!ret){
		print("read: failed\n");
		return;
	}

	if (0 && sect[SECTSZ-2] == 0x55 && sect[SECTSZ-1] == 0xaa)
		print("bingo: mbr found\n");
	
	/* TODO: parse mbr table, find fat/kfs */
}

enum { MaxIoifc = 3 };

static Ioifc* ioifc[MaxIoifc+1];

void
addioifc(Ioifc *io)
{
	static int nifc=0;

	if(nifc == MaxIoifc)
		panic("too many Ioifc");
	ioifc[nifc++] = io;
}

static void
dldiinit(void)
{
 	int n;

	if ((ulong)&hdr != hdr.sdata)
		DPRINT("bad DLDIhdr start %lux %lux\n", &hdr, &hdr.sdata);

	/*
	print("DLDI hdr %lux-%lux sz: %d fix: %s%s%s%s\n",
		hdr.sdata, hdr.etext, sizeof(hdr),
		(hdr.sect2fix & Fixall)? "all": "",
		(hdr.sect2fix & Fixglue)? "glue": "",
		(hdr.sect2fix & Fixgot)? "got": "",
		(hdr.sect2fix & Fixbss)? "bss": "");

	if (hdr.sect2fix & Fixall) print("hdr data %lux %lux\n", hdr.sdata, hdr.etext);
	if (hdr.sect2fix & Fixglue) print("hdr glue %lux %lux\n", hdr.sglue, hdr.eglue);
	if (hdr.sect2fix & Fixgot) print("hdr got %lux %lux\n", hdr.sgot, hdr.egot);
	if (hdr.sect2fix & Fixbss) print("hdr bss %lux %lux\n", hdr.sbss, hdr.ebss);
	*/

	if (!hdr.io.caps)
		return;
	
	// detect card type using dldi's info
	for(n = 0; ioifc[n]; n++){
		if(memcmp(ioifc[n]->type, hdr.io.type, 4))
			continue;
		if(ioifc[n]->caps != hdr.io.caps)
			continue;

		ioifc[n]->isin();
		ioifc[n]->clrstat();
		if(ioifc[n]->init())
			break;
	}

	if(!ioifc[n]){
		/* restore hdr.io in case of dldi-autopatching */
		memmove(&hdr.io, &io_none, sizeof(Ioifc));
		return;
	}

	DPRINT("dldi: %s\n", hdr.textid);
	DPRINT("ioifc: %.4s %s%s %s%s (%lux)\n",
		hdr.io.type, 
		(hdr.io.caps & Cslotgba)? "gba" : "",
		(hdr.io.caps & Cslotnds)? "nds" : "",
		(hdr.io.caps & Cread)? "r" : "",
		(hdr.io.caps & Cwrite)? "w" : "",
		hdr.io.caps);
		
	// set the default io interface
	memmove(&hdr.io, ioifc[n], sizeof(Ioifc));
	mbrtest();
}

static Chan*
dldiattach(char *spec)
{
	return devattach('L', spec);
}

static Walkqid*
dldiwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, dlditab, nelem(dlditab), devgen);
}

static int
dldistat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, dlditab, nelem(dlditab), devgen);
}

static Chan*
dldiopen(Chan *c, int omode)
{
	return devopen(c, omode, dlditab, nelem(dlditab), devgen);
}

static void
dldiclose(Chan *)
{
}

static char Ebadsize[] = "not a multiple of 512";

static long
dldiread(Chan* c, void* a, long n, vlong offset)
{
	switch ((ulong)c->qid.path) {
	case Qdir:
		return devdirread(c, a, n, dlditab, nelem(dlditab), devgen);
	case Qdata:
		DPRINT("dldiread a %lx n %ld o %lld\n", a, n, offset);
		if(offset % SECTSZ || n % SECTSZ)
			error(Ebadsize);
		hdr.io.read(offset / SECTSZ, n / SECTSZ, a);
		break;
	default:
		n = 0;
		break;
	}
	return n;
}

static long
dldiwrite(Chan* c, void* a, long n, vlong offset)
{
	switch ((ulong)c->qid.path) {
	case Qdir:
		return devdirread(c, a, n, dlditab, nelem(dlditab), devgen);
	case Qdata:
		DPRINT("dldiwrite a %lx n %ld o %lld\n", a, n, offset);
		if(offset % SECTSZ || n % SECTSZ)
			error(Ebadsize);
		hdr.io.write(offset / SECTSZ, n / SECTSZ, a);
		break;
	default:
		n = 0;
		break;
	}
	return n;
}

Dev dldidevtab = {
	'L',
	"dldi",

	devreset,
	dldiinit,
	devshutdown,
	dldiattach,
	dldiwalk,
	dldistat,
	dldiopen,
	devcreate,
	dldiclose,
	dldiread,
	devbread,
	dldiwrite,
	devbwrite,
	devremove,
	devwstat,
};
