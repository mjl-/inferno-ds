#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "ureg.h"

#include <draw.h>
#include <memdraw.h>
#include <cursor.h>

#include "screen.h"

#define	DPRINT	if(0)iprint

static Memdata xgdata;
static Memimage xgscreen =
{
	{0, 0, 0, 0},	/* r */
	{0, 0, 0, 0},	/* clipr */
	8,			/* depth */
	1,			/* nchan */
	CMAP8,		/* chan */
	nil,			/* cmap */
	&xgdata,		/* data */
	0,			/* zero */
	0,			/* width */
	nil,			/* layer */
	0,			/* flags */
};

Memimage *gscreen;
Memimage *conscol;
Memimage *back;

Memsubfont *memdefont;

static Point curpos;
static Rectangle window;

Vdisplay *vd;
SWcursor *swc = nil;

static char printbuf[1024];
static int printbufpos = 0;
static void lcdscreenputs(char*, int);
static void screenpbuf(char*, int);
void (*screenputs)(char*, int) = screenpbuf;

static	void	(*flushpixels)(Rectangle, uchar*, int, uchar*, int);

static Cursor arrow = {
	.offset = { -1, -1 },
	.clr = {
	[0x00]  0xFF, 0xFF, 0x80, 0x01, 0x80, 0x02, 0x80, 0x0C,
	[0x08]  0x80, 0x10, 0x80, 0x10, 0x80, 0x08, 0x80, 0x04,
	[0x10]  0x80, 0x02, 0x80, 0x01, 0x80, 0x02, 0x8C, 0x04,
	[0x18]  0x92, 0x08, 0x91, 0x10, 0xA0, 0xA0, 0xC0, 0x40,
	},
	.set = {
	[0x00]  0x00, 0x00, 0x7F, 0xFE, 0x7F, 0xFC, 0x7F, 0xF0,
	[0x08]  0x7F, 0xE0, 0x7F, 0xE0, 0x7F, 0xF0, 0x7F, 0xF8,
	[0x10]  0x7F, 0xFC, 0x7F, 0xFE, 0x7F, 0xFC, 0x73, 0xF8,
	[0x18]  0x61, 0xF0, 0x60, 0xE0, 0x40, 0x40, 0x00, 0x00,
	},
};

static Drawcursor carrow = {
	.hotx = CURSWID, .hoty = CURSHGT,
	.minx = 0, .miny = 0,
	.maxx = CURSWID, .maxy = CURSHGT,
	.data = arrow.clr,
};

int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	if(vd->depth >= 8)
		p &= 0xff;
	else
		p &= 0xf;
	vd->colormap[p][0] = r;
	vd->colormap[p][1] = g;
	vd->colormap[p][2] = b;
	lcd_setcolor(p, r, g, b);
	return ~0;
}

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	if(vd->depth >= 8)
		p = (p&0xff)^0xff;
	else
		p = (p&0xf)^0xf;
	*pr = vd->colormap[p][0];
	*pg = vd->colormap[p][1];
	*pb = vd->colormap[p][2];
}

void
graphicscmap(int invert)
{
	int num, den, i, j;
	int r, g, b, cr, cg, cb, v, p;

	for(r=0,i=0;r!=4;r++) for(v=0;v!=4;v++,i+=16){
		for(g=0,j=v-r;g!=4;g++) for(b=0;b!=4;b++,j++){
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
			if(invert)
				p ^= 0xFF;
			if(vd->depth == 4) {
				if((p&0xf) != (p>>4))
					continue;
				p &= 0xf;
			}
			setcolor(p,
				cr*0x01010101,
				cg*0x01010101,
				cb*0x01010101);
		}
	}
	lcd_flush();
}

void flushmemscreen(Rectangle r);

static void
screenclear(void)
{
	memimagedraw(gscreen, gscreen->r, memwhite, ZP, memopaque, ZP, SoverD);
	curpos = window.min;
	flushmemscreen(gscreen->r);
}

// video mem update: must set bit 15 of each ushort.
static void
flush2fb(Rectangle r, uchar *s, int sw, uchar *d, int dw)
{
	int h, w;

	DPRINT("1) s=%lux sw=%d d=%lux dw=%d r=(%d,%d)(%d,%d)\n",
		s, sw, d, dw, r.min.x, r.min.y, r.max.x, r.max.y);
	if(0){ static int i=0; if (i++ % 40 == 0) print("f\n");}
	/* TODO: sync with vblank period: 192 < vcount < 261 */
	if (0 && LCDREG->vcount - 192 > 1){
		while(LCDREG->vcount>192);
		while(LCDREG->vcount<192);
	}

	if (conf.screens >= 1){
		ulong *ud = (ulong*)d;
		ulong *us = (ulong*)s;
		ulong i;

		r.min.x &= ~15;
		r.max.x = (r.max.x + 15) & ~15;

		for(h = r.min.y; h < r.max.y; h++)
			for(w = r.min.x; w < r.max.x; w++){
				i = (h * Scrwidth + w)/2;
				ud[i] = (1<<31)|(1<<15)|us[i];
			}
	}
	if (conf.screens == 2){
		ushort *ud = (ushort*)(d + 0x00200000);
		ushort *us = (ushort*)(s + Scrsize * vd->depth / BI2BY);

		r.min.y -= Scrheight;
		r.max.y -= Scrheight;
		for(h = r.min.y; h < r.max.y; h++)
			for(w = r.min.x; w < r.max.x; w++)
				ud[h * Scrwidth + w] = (1<<15)|us[h * Scrwidth + w];
	}
}

enum {
	/* bit 15 of v written to fb must be set: v |= 1<<15 */
	ABGR15 = CHAN4(CAlpha, 1, CBlue, 5, CGreen, 5, CRed, 5),
	XBGR15 = CHAN4(CIgnore, 1, CBlue, 5, CGreen, 5, CRed, 5),
	BGR8 = CHAN3(CBlue, 3, CGreen, 3, CRed, 2),
};

static void
setscreen(LCDmode *mode)
{
	int h;

	if(swc != nil)
		swcurs_destroy(swc);

	vd = lcd_init(mode);
	if(vd == nil)
		panic("can't initialise LCD");
	
	gscreen = &xgscreen;
	xgdata.ref = 1;

	if(conf.portrait == 0)
		gscreen->r = Rect(0, 0, vd->x, vd->y);
	else
		gscreen->r = Rect(0, 0, vd->y, vd->x);

	gscreen->clipr = gscreen->r;
	gscreen->depth = vd->depth;
	gscreen->width = wordsperline(gscreen->r, gscreen->depth);
	
	xgdata.bdata = (uchar*)vd->sfb; 
	if(conf.portrait == 0)
		flushpixels = flush2fb;
	else
		flushpixels = flush2fb;
	
	memimageinit();
	memdefont = getmemdefont();
	memsetchan(gscreen, XBGR15);
	back = memwhite;
	conscol = memblack;
	memimagedraw(gscreen, gscreen->r, memwhite, ZP, memopaque, ZP, SoverD);
	DPRINT("vd->wid=%d bwid=%d gscreen->width=%ld fb=%p data=%p\n",
		vd->x, vd->bwid, gscreen->width, vd->fb, xgdata.bdata);
	graphicscmap(0);
	h = memdefont->height;
	window = insetrect(gscreen->r, 4);
	window.max.y = window.min.y+(Dy(window)/h)*h;

	screenclear(); 

	if (!conf.swcursor)
		return;
	swc = swcurs_create((ulong*)gscreen->data->bdata, gscreen->width, gscreen->depth/sizeof(ulong), gscreen->r, 0);
	drawcursor(&carrow);
}

void
screeninit(void)
{
	LCDmode lcd;

	memset(&lcd, 0, sizeof(lcd));
	if(archlcdmode(&lcd) < 0)
		return;
	setscreen(&lcd);
	screenputs = lcdscreenputs;
	if(printbufpos)
		screenputs("", 0);
	blanktime = 1;	/* minutes */
}

uchar*
attachscreen(Rectangle *r, ulong *chan, int* d, int *width, int *softscreen)
{
	*r = gscreen->r;
	*d = gscreen->depth;
	*chan = gscreen->chan;
	*width = gscreen->width;
	*softscreen = (gscreen->data->bdata != (uchar*)vd->fb);

while(0){	/* just to get some video memory timings */
	int tstart, vstart;

	tstart = m->ticks;
	vstart = LCDREG->vcount;
	while(LCDREG->vcount>192);
	while(LCDREG->vcount<192);

	print("t %d v %d-%d\n", m->ticks-tstart, vstart, LCDREG->vcount);
}	
	
	return (uchar*)gscreen->data->bdata;
}

void
detachscreen(void)
{
}

void
flushmemscreen(Rectangle r)
{

	if(rectclip(&r, gscreen->r) == 0)
		return; 
	if(r.min.x >= r.max.x || r.min.y >= r.max.y)
		return; 
	if(flushpixels != nil)
		flushpixels(r, (uchar*)gscreen->data->bdata, gscreen->width, (uchar*)vd->fb, vd->bwid >> 2);
	lcd_flush();
}

static void
scroll(void)
{
	int o;
	Point p;
	Rectangle r;

	o = 4*memdefont->height;
	r = Rpt(window.min, Pt(window.max.x, window.max.y-o));
	p = Pt(window.min.x, window.min.y+o);
	memimagedraw(gscreen, r, gscreen, p, nil, p, SoverD);
	flushmemscreen(r);
	r = Rpt(Pt(window.min.x, window.max.y-o), window.max);
	memimagedraw(gscreen, r, back, ZP, nil, ZP, SoverD);
	flushmemscreen(r);

	curpos.y -= o;
}

static void
clearline(void)
{
	Rectangle r;
	int yloc = curpos.y;

	r = Rpt(Pt(window.min.x, window.min.y + yloc),
		Pt(window.max.x, window.min.y+yloc+memdefont->height));
	memimagedraw(gscreen, r, back, ZP, nil, ZP, SoverD);
}

static void
screenputc(char *buf)
{
	Point p;
	int h, w, pos;
	Rectangle r;
	static int *xp;
	static int xbuf[256];

	h = memdefont->height;
	if(xp < xbuf || xp >= &xbuf[sizeof(xbuf)])
		xp = xbuf;

	switch(buf[0]) {
	case '\n':
		if(curpos.y+h >= window.max.y)
			scroll();
		curpos.y += h;
		/* fall through */
	case '\r':
		xp = xbuf;
		curpos.x = window.min.x;
		break;
	case '\t':
		if(curpos.x == window.min.x)
			clearline();
		p = memsubfontwidth(memdefont, " ");
		w = p.x;
		*xp++ = curpos.x;
		pos = (curpos.x-window.min.x)/w;
		pos = 8-(pos%8);
		r = Rect(curpos.x, curpos.y, curpos.x+pos*w, curpos.y+h);
		memimagedraw(gscreen, r, back, ZP, nil, ZP, SoverD);
		flushmemscreen(r);
		curpos.x += pos*w;
		break;
	case '\b':
		if(xp <= xbuf)
			break;
		xp--;
		r = Rpt(Pt(*xp, curpos.y), Pt(curpos.x, curpos.y + h));
		memimagedraw(gscreen, r, back, ZP, nil, ZP, SoverD);
		flushmemscreen(r);
		curpos.x = *xp;
		break;
	case '\0':
		break;
	default:
		p = memsubfontwidth(memdefont, buf);
		w = p.x;

		if(curpos.x >= window.max.x-w)
			screenputc("\n");

		if(curpos.x == window.min.x)
			clearline();
		if(xp < xbuf+nelem(xbuf))
			*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x+w, curpos.y+h);
		memimagedraw(gscreen, r, back, ZP, nil, ZP, SoverD);
		memimagestring(gscreen, curpos, conscol, ZP, memdefont, buf);
		flushmemscreen(r);
		curpos.x += w;
	}
}

static void
screenpbuf(char *s, int n)
{
	if(printbufpos+n > sizeof(printbuf))
		n = sizeof(printbuf)-printbufpos;
	if(n > 0) {
		memmove(&printbuf[printbufpos], s, n);
		printbufpos += n;
	}
}

static void
screendoputs(char *s, int n)
{
	int i;
	Rune r;
	char buf[4];

	while(n > 0) {
		i = chartorune(&r, s);
		if(i == 0){
			s++;
			--n;
			continue;
		}
		memmove(buf, s, i);
		buf[i] = 0;
		n -= i;
		s += i;
		screenputc(buf);
	}
}

void
screenflush(void)
{
	int j = 0;
	int k;

	for (k = printbufpos; j < k; k = printbufpos) {
		screendoputs(printbuf + j, k - j);
		j = k;
	}
	printbufpos = 0;
}

static void
lcdscreenputs(char *s, int n)
{
	static Proc *me;

	if(!canlock(vd)) {
		/* don't deadlock trying to print in interrupt */
		/* don't deadlock trying to print while in print */
		if(islo() == 0 || up != nil && up == me){
			/* save it for later... */
			/* In some cases this allows seeing a panic message
			  that would be locked out forever */
			screenpbuf(s, n);
			return;
		}
		lock(vd);
	}

	me = up;
	if (printbufpos)
		screenflush();
	screendoputs(s, n);
	if (printbufpos)
		screenflush();
	me = nil;

	unlock(vd);
}

