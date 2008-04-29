typedef struct Cursor Cursor;
typedef struct LCDmode LCDmode;
typedef struct LCDparam LCDparam;
typedef struct Vdisplay Vdisplay;
typedef struct Vmode Vmode;
typedef struct SWcursor SWcursor;

#define CURSWID	16
#define CURSHGT	16

struct Cursor {
	Point	offset;
	uchar	clr[CURSWID/BI2BY*CURSHGT];
	uchar	set[CURSWID/BI2BY*CURSHGT];
};

struct Vmode {
	int	x;	/* 0 -> default or any match for all fields */
	int	y;
	uchar	depth;
	uchar	hz;
};

struct Vdisplay {
	ushort*	fb;		/* frame buffer */
	ulong	colormap[256][3];
	int	bwid;
	Lock;
	Vmode; 
};

struct LCDparam {
	uchar	pbs;
	uchar	dual;
	uchar	mono;
	uchar	active;
	uchar	hsync_wid;
	uchar	sol_wait;
	uchar	eol_wait;
	uchar	vsync_hgt;
	uchar	sof_wait;
	uchar	eof_wait;
	uchar	lines_per_int;
	uchar	palette_delay;
	uchar	acbias_lines;
	uchar	obits;
	uchar	vsynclow;
	uchar	hsynclow;
};

struct LCDmode {
	Vmode;
	LCDparam;
};

int	archlcdmode(LCDmode*);

Vdisplay	*lcd_init(LCDmode*);
void	lcd_setcolor(ulong, ulong, ulong, ulong);
void	lcd_flush(void);
void	flushmemscreen(Rectangle r);

void	blankscreen(int);
void	drawblankscreen(int);
Point	mousexy(void);

extern ulong blanktime;

// needed by port/swcursor.c
enum
{
	Backgnd = 0xFF,	/* white */
	Foregnd =	0x00,	/* black */
};

extern Vdisplay *vd;
extern SWcursor *swc;
extern Memimage *gscreen;

SWcursor* swcurs_create(ulong *, int, int, Rectangle, int);
void swcurs_destroy(SWcursor*);
void swcurs_enable(SWcursor*);
void swcurs_disable(SWcursor*);
void swcurs_hide(SWcursor*);
void swcurs_unhide(SWcursor*);
void swcurs_load(SWcursor*, Cursor*);
void swcurs_update(int, int, int, int);
