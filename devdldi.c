/* 
 * DLDI access to SD/CF storage provided by homebrew hw,
 * written using the dldi.s code from Michael "Chishm".
 * 
 * use: a dldi patcher completes I/O fns, so we we can r/w,
 * tries to be simplest/sanest way of doing it i could come with.
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
	
	Cread =			0x00000001,
	Cwrite =		0x00000002,
	Cslotgba =		0x00000010,
	Cslotnds =		0x00000020,
	
	Fixall =		0x01,
	Fixglue =		0x02,
	Fixgot =		0x04,
	Fixbss =		0x08,	
};

enum
{
	LHDRSZ	= 15,		/* argmax {log2(sizeof(h))}, h is a common DLDIhdr */
	SECTSZ	= 512		/* Ioifc fns work with SECTSZ sectors */
};

typedef struct Ioifc Ioifc;
struct Ioifc{
	char type[4];			/* name */
	ulong caps;			/* capabilities */
	
	int (* init)(void);
	int (* isin)(void);
	int (* read)(ulong start, ulong n, void* d);
	int (* write)(ulong start, ulong n, const void* d);
	int (* clrstat)(void);
	int (* deinit)(void);
};

typedef struct DLDIhdr DLDIhdr;
struct DLDIhdr{
	ulong magic;			/* magic number */
	char magics[8];			/* magic string */
	uchar version;			/* version number */
	uchar dlogsz;			/* log2 size of driver */
	uchar sect2fix;			/* sections to fix */
	uchar slogsz;			/* log2 size of alloc space */
	char textid[48];
	
	ulong sdata, etext;		/* offsets to sections */
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

static
DLDIhdr hdr=
{
.magic = DLDImagic,
.magics = " Chishm",
.version = 1,
.dlogsz = LHDRSZ,
.sect2fix = 0,
.slogsz = LHDRSZ,
.textid = "no dldi interface",
	
.sdata = &hdr, .etext = 0, 
.sglue = 0, .eglue = 0,
.sgot = 0, .egot = 0,
.sbss = 0, .ebss = 0,	
	
.io = {
	.type = "DLDI",
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
dldiinit(void){
	int ret;
	int i;
	
	DPRINT("DLDI hdr %lux-%lux sz: %d fix: %s%s%s%s\n",
		hdr.sdata, hdr.etext, sizeof(hdr),
		(hdr.sect2fix & Fixall)? "all": "",
		(hdr.sect2fix & Fixglue)? "glue": "",
		(hdr.sect2fix & Fixgot)? "got": "",
		(hdr.sect2fix & Fixbss)? "bss": "");
	
	if (hdr.sect2fix & Fixall)
		DPRINT("hdr data %lux %lux\n", hdr.sdata, hdr.etext);
	if (hdr.sect2fix & Fixglue)
		DPRINT("hdr glue %lux %lux\n", hdr.sglue, hdr.eglue);
	if (hdr.sect2fix & Fixgot)
		DPRINT("hdr got %lux %lux\n", hdr.sgot, hdr.egot);
	if (hdr.sect2fix & Fixbss)
		DPRINT("hdr bss %lux %lux\n", hdr.sbss, hdr.ebss);

	if ((ulong)&hdr != hdr.sdata)
		print("bad DLDIhdr start %lux %lux\n", &hdr, &hdr.sdata);

	print("dldi: %s\n", hdr.textid);
//	while(1);
	if (hdr.io.caps){
		print("%.4s %s%s %s%s (%lx)\n",
		hdr.io.type, 
		(hdr.io.caps & Cslotgba)? "gba" : "",
		(hdr.io.caps & Cslotnds)? "nds" : "",
		(hdr.io.caps & Cread)? "r" : "",
		(hdr.io.caps & Cwrite)? "w" : "",
		hdr.io.caps);
		
		// more checks ...
		DPRINT("io.init %lux %lux\n", &hdr.io.init, *hdr.io.init);
		DPRINT("io.isin %lux %lux\n", &hdr.io.isin, *hdr.io.isin);
		DPRINT("io.read %lux %lux\n", &hdr.io.read, *hdr.io.read);
		DPRINT("io.write %lux %lux\n", &hdr.io.write, *hdr.io.write);
		DPRINT("io.clrstat %lux %lux\n", &hdr.io.clrstat, *hdr.io.clrstat);
		DPRINT("io.deinit %lux %lux\n", &hdr.io.deinit, *hdr.io.deinit);

if(0){ /* fix calling elf-arm code */
		uchar sect[SECTSZ];

		print("isin: %d\n", hdr.io.isin());
		print("clrstat: %d\n", hdr.io.clrstat());
		print("init: %d\n", hdr.io.init());
		//print("deinit: %d\n", hdr.io.deinit());

		print("while(1);\n");
		// while(1);
	
		ret =hdr.io.read(0, 1, sect);
		print("ret: %d\n", ret);

		for(i=0; i < sizeof(sect); i++){
			if(1)
			print("%x,", sect[i]);
			if(0 && sect[i] == 0xaa || sect[i] == 0x55)
				print("%d %x\n", i, sect[i]);
		}
		print("while(1);\n");
		while(1);
}
	}
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

static long
dldiread(Chan* c, void* a, long n, vlong offset)
{
	DPRINT("dldiread a %lx n %ld o %lld\n", a, n , offset);
	
	switch ((ulong)c->qid.path) {
	case Qdir:
		return devdirread(c, a, n, dlditab, nelem(dlditab), devgen);
	case Qdata:
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
	DPRINT("dldiwrite a %lx n %ld o %lld\n", a, n , offset);

	switch ((ulong)c->qid.path) {
	case Qdir:
		return devdirread(c, a, n, dlditab, nelem(dlditab), devgen);
	case Qdata:
//		hdr.io.write(offset / SECTSZ, n / SECTSZ, a);
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
