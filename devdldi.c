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

#define	DPRINT	if(1)print

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
	LHDRSZ	= 14,	/* argmax { log2(sizeof(h)) }, h is a common DLDIhdr */
	SECTSZ	= 512	/* Ioifc fns work with sectors of size SECTSZ */
};

typedef struct Ioifc Ioifc;
struct Ioifc{
	char type[4];	// name
	ulong caps;	// capabilities
	
	int (* init)(void);
	int (* isinserted)(void);
	int (* read)(ulong start, ulong n, void* d);
	int (* write)(ulong start, ulong n, const void* d);
	int (* clrstatus)(void);
	int (* deinit)(void);
};

typedef struct DLDIhdr DLDIhdr;
struct DLDIhdr{
	ulong magic;		// magic number
	char magics[8];		// magic string
	uchar version;
	uchar dlogsz;		// log2 size of driver
	uchar sect2fix;		// sections to fix
	uchar slogsz;		// log2 size of alloc space
	char textid[48];
	
	// offsets to sections
	ulong sdata, edata;
	ulong sglue, eglue;
	ulong sgot, egot;
	ulong sbss, ebss;

	struct Ioifc io;
	
	//set sizeof(DLDIhdr) = 1<<LHDRSZ  bytes
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
	
.sdata = &hdr,

.sglue = 0, .eglue = 0,
.sgot = 0, .egot = 0,
.sbss = 0, .ebss = 0,	
	
.io = {
	"dldi",
	0,
	
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
	
	if (0)
	print("DLDI hdr %lx sz: %d fix: %s%s%s%s\n",
		(ulong) &hdr,
		sizeof(DLDIhdr),
		(hdr.sect2fix & Fixall)? "all": "",
		(hdr.sect2fix & Fixglue)? "glue": "",
		(hdr.sect2fix & Fixgot)? "got": "",
		(hdr.sect2fix & Fixbss)? "bss": "");
		
	if (hdr.sdata != (ulong)&hdr)
		print("bad DLDIhdr start %lx %lx\n", (ulong) &hdr, hdr.sdata);

	print("dldi: %s\n", hdr.textid);
	print("%.4s %s%s %s%s (%lx)\n",
		hdr.io.type, 
		(hdr.io.caps & Cslotgba)? "gba" : "",
		(hdr.io.caps & Cslotnds)? "nds" : "",
		(hdr.io.caps & Cread)? "r" : "",
		(hdr.io.caps & Cwrite)? "w" : "",
		hdr.io.caps);
	
//	more checks ...
//	DPRINT("hdr data %lx %x\n", hdr.sdata, hdr.edata);
//	DPRINT("hdr glue %lx %lx\n", hdr.sglue, hdr.eglue);
//	DPRINT("hdr got %lx %lx\n", hdr.sgot, hdr.egot);
//	DPRINT("hdr bss %lx %lx\n", hdr.sbss, hdr.ebss);
//	
//	DPRINT("io.type %lx %x\n", &hdr.io.type[0], hdr.io.type[0]);
//	DPRINT("io.caps %lx %lx\n", &hdr.io.caps, hdr.io.caps);
//	
//	DPRINT("io.init %lx %lx %lx\n", &hdr.io.init, (*hdr.io.init), *(*hdr.io.init));
//	DPRINT("io.isinserted %lx %lx\n", &hdr.io.isinserted, (*hdr.io.isinserted));
//	DPRINT("io.read %lx %lx\n", &hdr.io.read, (*hdr.io.read));
//	DPRINT("io.write %lx %lx\n", &hdr.io.write, *hdr.io.write);
//	DPRINT("io.clrstatus %lx %lx\n", &hdr.io.clrstatus, *hdr.io.clrstatus);
//	DPRINT("io.deinit %lx %lx\n", &hdr.io.deinit, *hdr.io.deinit);
//
	ret = hdr.io.isinserted();
	print("isinserted: %d\n", ret);

	ret = hdr.io.clrstatus();
	print("clrstatus: %d\n", ret);
	
	ret = hdr.io.init();
	print("init: %d\n", ret);
	while(1);
	
	if(0){
	uchar sect[SECTSZ];
	
	ret =hdr.io.read(1, 1, sect);
	print("ret: %d\n", ret);

	for(i=0; i < sizeof(sect); i++){
		if(1)
			print("%x,", sect[i]);
		if(0 && sect[i] == 0xaa || sect[i] == 0x55)
			print("%d %x\n", i, sect[i]);
	}
	print("fi\n");
	while(1);
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
	int ret;
	DPRINT("dldiread a %lx n %ld o %lld\n", a, n , offset);

	switch ((ulong)c->qid.path) {
	case Qdir:
		return devdirread(c, a, n, dlditab, nelem(dlditab), devgen);
	case Qdata:
		ret = hdr.io.read(offset / SECTSZ, n / SECTSZ, a);
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
	int ret;
	DPRINT("dldiwrite a %lx n %ld o %lld\n", a, n , offset);

	switch ((ulong)c->qid.path) {
	case Qdir:
		return devdirread(c, a, n, dlditab, nelem(dlditab), devgen);
	case Qdata:
		ret = hdr.io.write(offset / SECTSZ, n / SECTSZ, a);
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
