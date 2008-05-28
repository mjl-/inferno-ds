#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

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
static void	lcdscreenputs(char*, int);
static void screenpbuf(char*, int);
void (*screenputs)(char*, int) = screenpbuf;

static Cursor arrow = {
	{ -1, -1 },
	{ 0xFF, 0xFF, 0x80, 0x01, 0x80, 0x02, 0x80, 0x0C,
	  0x80, 0x10, 0x80, 0x10, 0x80, 0x08, 0x80, 0x04,
	  0x80, 0x02, 0x80, 0x01, 0x80, 0x02, 0x8C, 0x04,
	  0x92, 0x08, 0x91, 0x10, 0xA0, 0xA0, 0xC0, 0x40,
	},
	{ 0x00, 0x00, 0x7F, 0xFE, 0x7F, 0xFC, 0x7F, 0xF0,
	  0x7F, 0xE0, 0x7F, 0xE0, 0x7F, 0xF0, 0x7F, 0xF8,
	  0x7F, 0xFC, 0x7F, 0xFE, 0x7F, 0xFC, 0x73, 0xF8,
	  0x61, 0xF0, 0x60, 0xE0, 0x40, 0x40, 0x00, 0x00,
	},
};

static	void	(*flushpixels)(Rectangle, uchar*, int, uchar*, int);

//static	void	flush2fb(Rectangle, uchar*, int, uchar*, int);



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

static uchar lum[256]={
  0,   7,  15,  23,  39,  47,  55,  63,  79,  87,  95, 103, 119, 127, 135, 143,
154,  17,   9,  17,  25,  49,  59,  62,  68,  89,  98, 107, 111, 129, 138, 146,
157, 166,  34,  11,  19,  27,  59,  71,  69,  73,  99, 109, 119, 119, 139, 148,
159, 169, 178,  51,  13,  21,  29,  69,  83,  75,  78, 109, 120, 131, 128, 149,
 28,  35,  43,  60,  68,  75,  83, 100, 107, 115, 123, 140, 147, 155, 163,  20,
 25,  35,  40,  47,  75,  85,  84,  89, 112, 121, 129, 133, 151, 159, 168, 176,
190,  30,  42,  44,  50,  90, 102,  94,  97, 125, 134, 144, 143, 163, 172, 181,
194, 204,  35,  49,  49,  54, 105, 119, 103, 104, 137, 148, 158, 154, 175, 184,
 56,  63,  80,  88,  96, 103, 120, 128, 136, 143, 160, 168, 175, 183,  40,  48,
 54,  63,  69,  90,  99, 107, 111, 135, 144, 153, 155, 173, 182, 190, 198,  45,
 50,  60,  70,  74, 100, 110, 120, 120, 150, 160, 170, 167, 186, 195, 204, 214,
229,  55,  66,  77,  79, 110, 121, 131, 129, 165, 176, 187, 179, 200, 210, 219,
 84, 100, 108, 116, 124, 140, 148, 156, 164, 180, 188, 196, 204,  60,  68,  76,
 82,  91, 108, 117, 125, 134, 152, 160, 169, 177, 195, 204, 212, 221,  66,  74,
 80,  89,  98, 117, 126, 135, 144, 163, 172, 181, 191, 210, 219, 228, 238,  71,
 76,  85,  95, 105, 126, 135, 145, 155, 176, 185, 195, 205, 225, 235, 245, 255,
};

void flushmemscreen(Rectangle r);

void
screenclear(void)
{
	memimagedraw(gscreen, gscreen->r, memwhite, ZP, memopaque, ZP, SoverD);
	curpos = window.min;
	flushmemscreen(gscreen->r);
}

static void
setscreen(LCDmode *mode)
{
	int h;

	vd = lcd_init(mode);
	if(vd == nil)
		panic("can't initialise LCD");

	gscreen = &xgscreen;
	xgdata.ref = 1;

	gscreen->r = Rect(0, 0, vd->x, vd->y);
	gscreen->clipr = gscreen->r;
	gscreen->depth = 16;
	gscreen->width = vd->x >>1;
	
	xgdata.bdata = (uchar*)vd->fb; 
	
	memimageinit();
	memdefont = getmemdefont();
	memsetchan(gscreen, CHAN4(CIgnore, 1, CBlue, 5, CGreen, 5, CRed, 5));	/* xxx we need a BGR15 in /include/draw.h */
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

	swc = swcurs_create((ulong*)gscreen->data->bdata, gscreen->width, gscreen->depth, gscreen->r, 1);
	drawcursor((Drawcursor*)&arrow);
//	if (swc != nil){
//		swcurs_load(swc, &arrow);
//		swcurs_enable(swc);
//	}
}

static void
vblankintr(Ureg *ureg, void *a)
{
	/* TODO: update lcd/screen fb on vblank */
	intrclear(VBLANKbit, 0);
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
	blanktime = 3;	/* minutes */

	setsublcdmode();
	// intrenable(0, VBLANKbit, vblankintr, 0, 0);
}

uchar*
attachscreen(Rectangle *r, ulong *chan, int* d, int *width, int *softscreen)
{
	*r = gscreen->r;
	*d = gscreen->depth;
	*chan = gscreen->chan;
	*width = gscreen->width;
	*softscreen = (gscreen->data->bdata != (uchar*)vd->fb);

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

