/*
 * ds
 */ 
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"io.h"
#include	"draw.h"
#include	<memdraw.h>
#include	"screen.h"

#include "../port/netif.h"
#include "etherif.h"

#define DPRINT if(0)print

// swi bios calls (not used)
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

void
archreset(void)
{
}

void
archconsole(void)
{
}

void
archpowerdown(void)
{
	fifoput(F9TSystem|F9Syspoweroff, 1);
}

void
archreboot(void)
{
	fifoput(F9TSystem|F9Sysreboot, 1);
}

/* no need for this? */
void
archpowerup(void)
{
	;
}

enum
{
	/* Opera memory expansion */
	Operactl = 0x08240000, 			/* set to 0x0001 when enabled */
	Operastart = 0x09000000,		/* start and len of psram (bytes) */
	Operalen = 8*1024*1024,			/* 8 Mb available */

	Canary = 0x11223344,
};

static int
ramcheck(ulong *s, ulong n)
{
	ulong *p, *e;

	p = s;
	e = s + n;
	while(p < e){
		*p = Canary;
		if (*p != Canary)
			return 0;
		//print("ok %x %x\n", p, *p);
		p += 256;
	}
	return 1;
}

void
archconfinit(void)
{
	ushort *operactl = (ushort*)Operactl;

	// arm9 is the owner of ram, slot-1 & slot-2 
	EXMEMREG->ctl &= ~(Arm7hasds|Arm7hasgba|Arm7hasram);

	/* default memory bank */
	conf.base0 = PGROUND((ulong)end);
	conf.npage0 = (EWRAMTOP - conf.base0)/BY2PG;
	conf.topofmem = EWRAMTOP;
	
	/* extra memory bank: slot2 expansion */
	if (*operactl){
		*operactl = 1;

		if (0 && ramcheck((ulong*)Operastart, Operalen/4)){
			conf.base1 = PGROUND(Operastart);
			conf.npage1 = (Operalen)/BY2PG;
		}
		else if	(ramcheck((ulong*)ROMZERO, Operalen)){
			/* another desmume trick */
			conf.base1 = PGROUND(ROMZERO);
			conf.npage1 = (Operalen)/BY2PG;
		}
		else{
			conf.base1 = 0;
			conf.npage1 = 0;
		}

		DPRINT("opera base1 %lux npage1 %lud\n", conf.base1, conf.npage1);
	}

	m->cpuhz = 66*1000000;
	conf.bsram = SRAMTOP;
	conf.brom = ROMTOP;
	conf.bmap = 0;
}

void
kbdinit(void)
{
	kbdq = qopen(4*1024, 0, nil, nil);
	addclock0link(kbdclock, MS2HZ);
}

static LCDmode lcd256x192x16tft =
{
//	.x = 240, .y = 160, .depth = 16, .hz = 60,
//	.hsync_wid = 4-2, .sol_wait = 12-1, .eol_wait = 17-1,
//	.vsync_hgt = 3-1, .soft_wait = 10, .eof_wait = 1,
//	.lines_per_int = 0,  .acbias_lines = 0,
//	.vsynclow = 1, .hsynclow = 1,
	256, 192, 16, 60,
	4-2, 12-1, 17-1,
	3-1, 10, 1,
	0, 0,
	1, 1,
};

int
archlcdmode(LCDmode *m)
{
	*m =  lcd256x192x16tft;
	return 0;
}

/*
 * set ether parameters: the contents should be derived from EEPROM or NVRAM
 */
int
archether(int ctlno, Ether *ether)
{
	static char opt[128];

	if(ctlno > 0)
		return -1;

	sprint(ether->type, "nds");
	ether->mem = 0;
	ether->nopt = 0;
	ether->port = 0;
	ether->irq = IPCSYNCbit;
	ether->itype = 0;
	ether->mbps = 2;
	ether->maxmtu = 1492;

	/* IPCSYNC irq from arm7 when there's wifi activity (tx/rx) */
	IPCREG->ctl |= Ipcirqena;
	
	memset(ether->ea, 0xff, Eaddrlen);
	nbfifoput(F9TWifi|F9WFrmac, (ulong)ether->ea);	/* mac from arm7 */
	
	if(1){	/* workaround for desmume */
		uchar i, maczero[Eaddrlen];
		
		for(i=0; i < 1<<(8*sizeof(uchar))-1; i++);
		memset(maczero, 0x00, Eaddrlen);
		if(memcmp(ether->ea, maczero, Eaddrlen) == 0)
			memset(ether->ea, 0x01, Eaddrlen);
	}

	strcpy(opt, "mode=managed channel=1 crypt=off essid=THOMSON station=ds");
	ether->nopt = tokenize(opt, (char **)ether->opt, nelem(ether->opt));

	return 1;
}
