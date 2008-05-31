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
	fifoput(F9poweroff, 0);
}

void
archreboot(void)
{
	fifoput(F9reboot, 0);
}

/* no need for this? */
void
archpowerup(void)
{
	;
}


void
archconfinit(void)
{
	conf.topofmem = EWRAMTOP;
	m->cpuhz = 66*1000000;
}

void
kbdinit(void)
{
	kbdq = qopen(4*1024, 0, nil, nil);
	addclock0link(kbdclock, MS2HZ);
}

static LCDmode lcd256x192x8tft =
{
//	.x = 240, .y = 160, .depth = 16, .hz = 60,
//	.hsync_wid = 4-2, .sol_wait = 12-1, .eol_wait = 17-1,
//	.vsync_hgt = 3-1, .soft_wait = 10, .eof_wait = 1,
//	.lines_per_int = 0,  .acbias_lines = 0,
//	.vsynclow = 1, .hsynclow = 1,
	256, 192, 8, 60,
	4-2, 12-1, 17-1,
	3-1, 10, 1,
	0, 0,
	1, 1,
};

int
archlcdmode(LCDmode *m)
{
	*m =  lcd256x192x8tft;
	return 0;
}

/*
 * set ether parameters: the contents should be derived from EEPROM or NVRAM
 */
int
archether(int ctlno, Ether *ether)
{
	if(ctlno >= 0)
		return -1;
	sprint(ether->type, "nds");
	ether->mem = 0;
	ether->nopt = 0;
	ether->port = 0;
	ether->irq = WIFIbit;
	ether->itype = 0;
//	memmove(ether->ea, macaddrs[ctlno], Eaddrlen);
	return 1;
}
