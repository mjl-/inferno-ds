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
#define	DPRINT	if(1)iprint

enum {
	/* lccr */
/*	Mode0 = 0x0, */
	Mode0 = 0x10000,
	Mode3 = 0x3,
	Mode4 = 0x4,	/* only use mode 3 for Inferno for now */
	Mode5 = 0x5,	/* only use mode 3 for Inferno for now */
	
	Modefb = 0x00020000,

	Bg0enable = 0x100,
	Bg1enable = 0x200,
	Bg2enable = 0x400,
	/* lcsr */
	EnableCtlr =~0x7
};
#define Scrbase(n) (((n)*0x800)+0x6000000)
#define Cbase(n) (((n)*0x4000)+0x6000000)

ushort *bg0map = (ushort*)Scrbase(31);




typedef struct {
	Vdisplay;
	LCDparam;
	ushort*	palette;
	ushort*	upper;
	ushort*	lower;
} LCDdisplay;

static LCDdisplay	*vd;	// current active display

void
lcd_setcolor(ulong p, ulong r, ulong g, ulong b)
{
	if(p > 255)
		return;
	vd->palette[p] = ((r>>(32-5))<<10) |
			((g>>(32-5))<<5) |
			(b>>(32-5));
}

static void
disablelcd(void)
{
	LcdReg *lcd = LCDREG;

	/* if LCD enabled, turn off */
	if(lcd->lcsr & EnableCtlr) {
		lcd->lcsr &= ~EnableCtlr;
	}
}

static void
setlcdmode(LCDdisplay *vd)
{
	LCDmode *p;
	LcdReg *sublcd = SUBLCDREG;
	LcdReg *lcd = LCDREG;
	VramReg *vram = VRAMREG;
	PowerReg *power = POWERREG;
	p = (LCDmode*)&vd->Vmode;

	disablelcd();
	lcd->lccr = 0;	
  	
 //	sublcd->lccr = MODE_0_2D | DISPLAY_BG0_ACTIVE;
   	power->pcr = POWER_ALL_2D;
 	lcd->lccr = MODE_FB0;
      	vram->acr = VRAM_ENABLE|VRAM_A_LCD;

	iprint("lccr=%8.8lux\n", lcd->lccr); 
}
static LCDdisplay main_display;	/* TO DO: limits us to a single display */

Vdisplay*
lcd_init(LCDmode *p)
{
	int palsize;
	int fbsize;

	vd = &main_display;
	vd->Vmode = *p;
	vd->LCDparam = *p;
	DPRINT("%dx%dx%d: hz=%d\n", vd->x, vd->y, vd->depth, vd->hz); /* */

	vd->palette = PAL;
	vd->palette[0] = 0;
	vd->upper = VIDMEMLO;
	vd->bwid = vd->x; /* only 8 bit for now, may implement 16 bit later */
	vd->lower = VIDMEMHI;
	vd->fb = vd->lower;
	DPRINT("  fbsize=%d p=%p u=%p l=%p\n", fbsize, vd->palette, vd->upper, vd->lower); /* */

	setlcdmode(vd);
	return vd;
}

void
lcd_flush(void)
{
//		dcflushall();	/* need more precise addresses */
}

void
blankscreen(int blank)
{
	LcdReg *lcd = LCDREG;
	if(blank)
		lcd->lcsr |= ~EnableCtlr;
	else
		lcd->lcsr |= EnableCtlr;			
}
void cmap(ushort* palette);


#define RGB15(r,g,b)	((r)|(g<<5)|(b<<10))
void
cmap(ushort* palette)
{
	int num, den, i, j;
	int r, g, b, cr, cg, cb, v, p;

	for(r=0,i=0;r!=4;r++) 
		for(v=0;v!=4;v++,i+=16){
			for(g=0,j=v-r;g!=4;g++) 
				for(b=0;b!=4;b++,j++){
			den=r;
			if(g>den) den=g;
			if(b>den) den=b;
			if(den==0)	/* divide check -- pick grey shades */
				cr=cg=cb=v*17;
			else{
				num=17*(4*den+v);
				cr=r*num/den;
				cg=g*num/den;
				cb=b*num/den;
			}
			p = (i+(j&15));

			palette[p] =RGB15(cr*0x01010101,cg*0x01010101,cb*0x01010101);
		}
	}
}

void
dsconsinit(void)
{

	ushort *palette = ((ushort*)PAL);
	setlcdmode(vd);


}
void
uartputs(char* s, int n) {
}

