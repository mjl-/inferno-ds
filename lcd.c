#include	"u.h"
#include 	"mem.h"
#include	"../port/lib.h"
#include 	"dat.h"
#include 	"draw.h"
#include	"fns.h"
#include	"io.h"
#include	<memdraw.h>
#include	"screen.h"
#include	"lcdreg.h"

#define	DPRINT	if(0)iprint

typedef struct {
	Vdisplay;
	LCDparam;
	ushort*	pal;
	ushort*	subpal;
	ushort*	upper;
	ushort*	lower;
} LCDdisplay;

static LCDdisplay	*ld;	// current active display (lower)
static LCDdisplay main_display;	/* TODO: limits us to a single display */

/* TODO: use a 265 colors palette */
void
lcd_setcolor(ulong p, ulong r, ulong g, ulong b)
{
	if(p > 255)
		return;
	ld->pal[p] = BGR15((r>>(32-5)), (g>>(32-5)),(b>>(32-5)));
	ld->subpal[p] = BGR15((r>>(32-5)), (g>>(32-5)),(b>>(32-5)));
}

enum { Lcdytrig = 192 };

static void
setlcdmode(void)
{
	LcdReg *lcd = LCDREG;

	POWERREG->pcr |= POWER_LCD|POWER_2D_A;
	lcd->lccr = (Disp2dmode|5) | Dispbgactive(2);
	lcd->lcsr |= (lcd->lcsr & 0x7F ) | (Lcdytrig << 8) | ((Lcdytrig & 0x100 ) >> 2) ;

	lcd->bgctl[2] = Bgctlrs32x32|Bgctl16bpp|Bgctlbmpbase(0)|(0 & Bgctlpriomsk);
	if (ld->depth == 16) 
		lcd->bgctl[2] |= Bgctl16bpc;
	lcd->bg2rs.dx = 1<<8;
	lcd->bg2rs.dmx = 0;
	lcd->bg2rs.dy = 0;
	lcd->bg2rs.dmy = 1<<8;
	lcd->bg2rs.x = 0;
	lcd->bg2rs.y = 0;

	memset(ld->lower, 0xffff, Scrsize * ld->depth / BI2BY);
}

static void
setsublcdmode(void)
{
	LcdReg *sublcd = SUBLCDREG;

	POWERREG->pcr |= POWER_LCD|POWER_2D_B;
	sublcd->lccr = (Disp2dmode|3)|Dispbgactive(3);

	sublcd->bgctl[3] = Bgctlrs32x32|Bgctl16bpp|Bgctlbmpbase(0)|(0 & Bgctlpriomsk);
	if (ld->depth == 16) 
		sublcd->bgctl[3] |= Bgctl16bpc;
	sublcd->bg3rs.dx = 1<<8;
	sublcd->bg3rs.dmx = 0;
	sublcd->bg3rs.dy = 0;
	sublcd->bg3rs.dmy = 1<<8;
	sublcd->bg3rs.x = 0;
	sublcd->bg3rs.y = 0;

	memset(ld->upper, 0xffff, Scrsize * ld->depth / BI2BY);
}

Vdisplay*
lcd_init(LCDmode *p)
{
	ld = &main_display;
	ld->Vmode = *p;
	ld->LCDparam = *p;
	DPRINT("%dx%dx%d: hz=%d\n", ld->x, ld->y, ld->depth, ld->hz);

	// lcd fb @ 0x06000000, soft fb @ 0x0x06300000
      	VRAMREG->acr = Vramena|VRAM_A_MAIN_BG_0x06000000;	// [0x00000,0x20000]
      	VRAMREG->bcr = Vramena|VRAM_B_MAIN_BG_0x06020000;	// [0x20000,0x40000]
	VRAMREG->dcr = Vramena|VRAM_D_MAIN_BG_0x06040000;	// [0x40000,0x60000]
	// sublcd fb @ 0x06200000, 
	VRAMREG->ccr = Vramena|VRAM_C_SUB_BG_0x06200000;

	ld->bwid = (ld->x << ld->pbs) >> 1;
	ld->pal = (ushort*)PALMEM;
	ld->subpal = (ushort*)SUBPALMEM;
	ld->upper = (ushort*)0x06200000;
	ld->lower = (ushort*)0x06000000;
	ld->fb =    (ushort*)0x06000000;
	ld->sfb =   (ushort*)0x06030000;
	DPRINT("p=%p u=%p l=%p\n", ld->pal, ld->upper, ld->lower);

	setlcdmode();
	setsublcdmode();

	if(conf.screens == 1)
		POWERREG->pcr &= ~POWER_SWAP_LCDS;
	else
		POWERREG->pcr |= POWER_SWAP_LCDS;

	return ld;
}

void
lcd_flush(void)
{
	if(0)print("lcd_flush\n");
	dcflushall();	/* need more precise addresses */
}

void
blankscreen(int blank)
{
	PowerReg *power = POWERREG;

	if(blank)
		power->pcr |= POWER_LCD;
	else
		power->pcr &= ~POWER_LCD;
}

void
uartputs(char* s, int n) {
	USED(s,n);
}
