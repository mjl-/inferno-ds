#include	"u.h"
#include 	"mem.h"
#include	"../port/lib.h"
#include 	"dat.h"
#include 	"draw.h"
#include	"fns.h"
#include	"io.h"
#include	<memdraw.h>
#include	"screen.h"

#define	DPRINT	if(0)iprint

// macro creates a 15 bit color from 3x5 bit components
#define BGR15(r,g,b)	(((b)<<10)|((g)<<5)|(r))
#define BGR8(r,g,b)	((((b)>>3)<<10)|(((g)>>3)<<5)|((r)>>3))

#define VRAM_OFFSET(n)	((n)<<3)
#define VRAM_START(n)	((n)*0x20000)

enum
{
	/* VRAM modes */
	VRAM_LCD	= 0,
	VRAM_MAIN_BG	= 1,
	VRAM_SPRITE	= 2,
	VRAM_TEXTURE	= 3,
	VRAM_SUB_BG	= 4,

	/* VRAM A */
	VRAM_A_LCD	=	VRAM_LCD,
	VRAM_A_MAIN_BG  = VRAM_MAIN_BG,
	VRAM_A_MAIN_SPRITE = VRAM_SPRITE,
	VRAM_A_TEXTURE = VRAM_TEXTURE,

	/* VRAM B */
	VRAM_B_LCD = 0,
	VRAM_B_MAIN_BG	= VRAM_MAIN_BG | VRAM_OFFSET(1),
	VRAM_B_MAIN_SPRITE	= VRAM_SPRITE | VRAM_OFFSET(1),
	VRAM_B_TEXTURE	= VRAM_TEXTURE | VRAM_OFFSET(1),

	/* VRAM C */
	VRAM_C_LCD = 0,
	VRAM_C_MAIN_BG  = VRAM_MAIN_BG | VRAM_OFFSET(2),
	VRAM_C_ARM7	= 2,
	VRAM_C_ARM7_0x06000000 = 2,
	VRAM_C_ARM7_0x06020000 = 2 | VRAM_OFFSET(1),
	VRAM_C_SUB_BG	= VRAM_SUB_BG,
	VRAM_C_TEXTURE	= VRAM_TEXTURE | VRAM_OFFSET(2),

	/* VRAM D */
	VRAM_D_LCD = 0,
	VRAM_D_MAIN_BG  = VRAM_MAIN_BG | VRAM_OFFSET(3),
	VRAM_D_ARM7 = 2 | VRAM_OFFSET(1),
	VRAM_D_ARM7_0x06000000 = 2,
	VRAM_D_ARM7_0x06020000 = 2 | VRAM_OFFSET(1),
	VRAM_D_SUB_SPRITE  = 4,
	VRAM_D_TEXTURE = VRAM_TEXTURE | VRAM_OFFSET(3),

	/* VRAM E */
	VRAM_E_LCD             = VRAM_LCD,
	VRAM_E_MAIN_BG         = VRAM_MAIN_BG,
	VRAM_E_MAIN_SPRITE     = VRAM_SPRITE,
	VRAM_E_TEX_PALETTE     = VRAM_TEXTURE,
	VRAM_E_BG_EXT_PALETTE  = 4,
	VRAM_E_OBJ_EXT_PALETTE = 5,

	/* VRAM F */
	VRAM_F_LCD             = VRAM_LCD,
	VRAM_F_MAIN_BG         = VRAM_MAIN_BG,
	VRAM_F_MAIN_SPRITE     = VRAM_SPRITE,
	VRAM_F_TEX_PALETTE     = VRAM_TEXTURE,
	VRAM_F_BG_EXT_PALETTE  = 4,
	VRAM_F_OBJ_EXT_PALETTE = 5,

	/* VRAM G */
	VRAM_G_LCD             = VRAM_LCD,
	VRAM_G_MAIN_BG         = VRAM_MAIN_BG,
	VRAM_G_MAIN_SPRITE     = VRAM_SPRITE,
	VRAM_G_TEX_PALETTE     = VRAM_TEXTURE,
	VRAM_G_BG_EXT_PALETTE  = 4,
	VRAM_G_OBJ_EXT_PALETTE = 5,

	/* VRAM H */
	VRAM_H_LCD                = 0,
	VRAM_H_SUB_BG             = 1,
	VRAM_H_SUB_BG_EXT_PALETTE = 2,

	/* VRAM H */
	VRAM_I_LCD                    = 0,
	VRAM_I_SUB_BG                 = 1,
	VRAM_I_SUB_SPRITE             = 2,
	VRAM_I_SUB_SPRITE_EXT_PALETTE = 3,
};

typedef struct {
	Vdisplay;
	LCDparam;
	ushort*	pal;
	ushort*	subpal;
	ushort*	upper;
	ushort*	lower;
} LCDdisplay;

static LCDdisplay	*ld;	// current active display
static LCDdisplay main_display;

/* TODO: use 256 color palette */
void
lcd_setcolor(ulong p, ulong r, ulong g, ulong b)
{
	if(p > 255)
		return;
	ld->pal[p] = BGR15((r>>(32-5)), (g>>(32-5)),(b>>(32-5)));
	ld->subpal[p] = BGR15((r>>(32-5)), (g>>(32-5)),(b>>(32-5)));
}

enum
{
    Lcdytrig = 80,
    Black = 0x00,
    White = 0xff,
};

#define SetYtrigger(n)	((lcd->lcsr&0x7F) | (n<<8) | ((n&0x100)>>2))

static void
setlcdmode(void)
{
	LcdReg *lcd = LCDREG;

	POWERREG->pcr |= POWER_LCD|POWER_2D_A;
	lcd->lccr = (Disp2dmode|5) | Dispbgactive(2);
	lcd->lcsr |= SetYtrigger(Lcdytrig);

	lcd->bgctl[2] = Bgctlrs32x32|Bgctl16bpp|Bgctlbmpbase(0)|(0 & Bgctlpriomsk);
	if (ld->depth == 16) 
		lcd->bgctl[2] |= Bgctl16bpc;
	lcd->bg2rs.dx = 1<<8;
	lcd->bg2rs.dmx = 0;
	lcd->bg2rs.dy = 0;
	lcd->bg2rs.dmy = 1<<8;
	lcd->bg2rs.x = 0;
	lcd->bg2rs.y = 0;

	memset(ld->lower, White, Scrsize * ld->depth / BI2BY);
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

	memset(ld->upper, White, Scrsize * ld->depth / BI2BY);
}

Vdisplay*
lcd_init(LCDmode *p)
{
	ld = &main_display;
	ld->Vmode = *p;
	ld->LCDparam = *p;
	DPRINT("%dx%dx%d: hz=%d\n", ld->x, ld->y, ld->depth, ld->hz);

	// lcd fb @ 0x06000000, soft fb @ 0x0x06300000
	VRAMREG->acr = Vramena|VRAM_MAIN_BG|VRAM_OFFSET(0);	// [0x00000,0x20000]
	VRAMREG->bcr = Vramena|VRAM_MAIN_BG|VRAM_OFFSET(1);	// [0x20000,0x40000]
	VRAMREG->dcr = Vramena|VRAM_MAIN_BG|VRAM_OFFSET(2);	// [0x40000,0x60000]
	// sublcd fb @ 0x06200000, 
	VRAMREG->ccr = Vramena|VRAM_SUB_BG|VRAM_OFFSET(0);

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
//	DPRINT("lcd_flush\n");
//	dcflushall();	/* need more precise addresses */
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
