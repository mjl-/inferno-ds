#include "u.h"
#include "mem.h"
#include "io.h"

enum {
	/* lccr */
	Mode3 = 0x3,
	Mode4 = 0x4,	/* only use mode 4 for Inferno */
	Bg2enable = 0x400, /* enable display on the frame buffer */

	/* lcsr */
	EnableCtlr =~0x7,
};

void graphicscmap(ushort *palette);
/* int setcolor(ulong p, ulong r, ulong g, ulong b); */
#define PAL 0x05000000
#define VRAM 0x6000000
ushort *vramend = 0x06017FFF;

#define RGB15(r,g,b)	((r)|(g<<5)|(b<<10))

int
main(void) {
	int i, j, k;
	ushort col;
	LcdReg *lcd = LCDREG;
	ushort *palette = ((ushort*)PAL);
	ushort *vram = ((ushort*)VRAM);
	// Enable LCD
	lcd->lccr = Mode4 | 0x400; /* | 0x800 | 0x100; */

/*	for(i = 0; i < 256; i++)
		palette[i] = RGB15(i,i,i); */
	graphicscmap(palette);
	
	for(i = 0; vram != vramend; vram++, i++ ) {
		i &= 0xff;
		col = i  | (i <<8);
		*vram = col;
	} 
	while(1);
	return 0;	
}

void
graphicscmap(ushort* palette)
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

/*


int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	p &= 0xff;

	vd->colormap[p][0] = r;
	vd->colormap[p][1] = g;
	vd->colormap[p][2] = b;
	palette16[p] = ;
	lcd_setcolor(p, r, g, b);
	return ~0;
}
*/