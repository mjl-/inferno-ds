#include	"u.h"
#include 	"mem.h"
#include	"../port/lib.h"
#include 	"dat.h"
#include 	"draw.h"
#include	"fns.h"
#include	"io.h"
#include	<memdraw.h>
#include	"screen.h"
#include	"arm7/jtypes.h"
#include	"lcdreg.h"

#define	DPRINT	if(0)iprint

#define Scrbase(n) (((n)*0x800)+0x6000000)
#define Cbase(n) (((n)*0x4000)+0x6000000)

typedef struct {
	Vdisplay;
	LCDparam;
	ushort*	palette;
	ushort*	upper;
	ushort*	lower;
} LCDdisplay;

static LCDdisplay	*ld;	// current active display

void
lcd_setcolor(ulong p, ulong r, ulong g, ulong b)
{
	if(p > 255)
		return;
	ld->palette[p] = ((r>>(32-5))<<10) |
			((g>>(32-5))<<5) |
			(b>>(32-5));
}

static void
setlcdmode(LCDdisplay *ld)
{
	LcdReg *lcd = LCDREG;
	VramReg *vram = VRAMREG;
	PowerReg *power = POWERREG;

	lcd->lccr = 0;	
  	
	power->pcr = (POWER_LCD|POWER_2D_A|POWER_2D_B);
 	lcd->lccr = MODE_FB(0);
      	vram->acr = VRAM_ENABLE|VRAM_A_LCD;

	DPRINT("lccr=%8.8lux\n", lcd->lccr); 
}
static LCDdisplay main_display;	/* TODO: limits us to a single display */

void
setsublcdmode(void)
{
	VramReg *vram = VRAMREG;
	LcdReg *sublcd = SUBLCDREG;

	/* enable vram c to hold the background for the sub display */
	vram->ccr = VRAM_ENABLE|VRAM_C_SUB_BG_0x06200000;

	/* enable mode 3, enable background 3 */
	sublcd->lccr = MODE_2D(3)|DISPLAY_BG_ACTIVE(3);

	/* set the background mode:  256x256 pixels of 16 bits each */
	SUB_BG_CR(3) = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(3);
	SUB_BG3_XDX = 1 << 8;
	SUB_BG3_XDY = SUB_BG3_YDX = 0;
	SUB_BG3_YDY = 1 << 8;
	SUB_BG3_CX = SUB_BG3_CY = 0;

	/* copy in the image */
	putlogo((uchar*)0x06200000);
}

Vdisplay*
lcd_init(LCDmode *p)
{
	int fbsize;

	ld = &main_display;
	ld->Vmode = *p;
	ld->LCDparam = *p;
	DPRINT("%dx%dx%d: hz=%d\n", ld->x, ld->y, ld->depth, ld->hz); /* */

	ld->palette = (ushort*)PALMEM;
	ld->palette[0] = 0;
	ld->upper = VIDMEMLO;
	ld->bwid = ld->x; /* only 8 bit for now, may implement 16 bit later */
	ld->lower = VIDMEMHI;
	ld->fb = ld->lower;
	DPRINT("  fbsize=%d p=%p u=%p l=%p\n", fbsize, ld->palette, ld->upper, ld->lower); /* */

	setlcdmode(ld);
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
		power->pcr &= ~(POWER_LCD);
	else
		power->pcr |= POWER_LCD;
}

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
uartputs(char* s, int n) {
	USED(s,n);
}
