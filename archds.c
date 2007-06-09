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

void
archreset(void)
{
	dsconsinit();
}

void
archconsole(void)
{
//	uartspecial(0, 57600, 'n', &kbdq, &printq, kbdcr2nl);
}

void
archpowerdown(void)
{
	_stop();
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
	m->cpuhz = CLOCKFREQ;

}

void
kbdinit(void)
{
	addclock0link(kbdclock, MS2HZ);
}

static LCDmode lcd240x160x16tft =
{
//	.x = 240, .y = 160, .depth = 16, .hz = 60,
//	.hsync_wid = 4-2, .sol_wait = 12-1, .eol_wait = 17-1,
//	.vsync_hgt = 3-1, .soft_wait = 10, .eof_wait = 1,
//	.lines_per_int = 0,  .acbias_lines = 0,
//	.vsynclow = 1, .hsynclow = 1,
	240, 160, 8, 60,
	4-2, 12-1, 17-1,
	3-1, 10, 1,
	0, 0,
	1, 1,
};

int
archlcdmode(LCDmode *m)
{
	*m = lcd240x160x16tft;
	return 0;
}


/*
void
archconsole(void)
{
	uartspecial(0, 115200, 'n', &kbdq, &printq, kbdcr2nl);
}

void
archuartpower(int port, int on)
{

}
*/
void
archreboot(void)
{
	_reset();
}

