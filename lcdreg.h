// macro creates a 15 bit color from 3x5 bit components
#define RGB5(r,g,b)  ((r)|((g)<<5)|((b)<<10))
#define RGB8(r,g,b)  (((r)>>3)|(((g)>>3)<<5)|(((b)>>3)<<10))

#define VRAM_OFFSET(n)	((n)<<3)

/*
 * TODO:
 * read gbatek.txt and rework this file,
 * some bits from here could be in io.h:/Lcdreg
 *
 * VRAM_MAIN_BG(bank, addr) for:
 * 	- bank in {A, B, C, D}
 * 	- addr in {0x06000000, 0x06020000, 0x06040000, 0x06060000}
 * should give the results presented below
 *
 * VRAM_TEXTURE_SLOT(bank, slot) for:
 * 	- bank in {A, B, C, D}
 * 	- slot in {0, 1, 2, 3}
 *
 * VRAM_MAIN_SPRITE(bank, addr) for:
 * 	- bank in {A, B}
 * 	- addr in {0x06400000, 0x0642000, ...}
 *
 */

typedef enum {
	VRAM_A_LCD	=	0,
	VRAM_A_MAIN_BG  = 1,
	VRAM_A_MAIN_BG_0x06000000	= 1 | VRAM_OFFSET(0),
	VRAM_A_MAIN_BG_0x06020000	= 1 | VRAM_OFFSET(1),
	VRAM_A_MAIN_BG_0x06040000	= 1 | VRAM_OFFSET(2),
	VRAM_A_MAIN_BG_0x06060000	= 1 | VRAM_OFFSET(3),
	VRAM_A_MAIN_SPRITE = 2,
	VRAM_A_MAIN_SPRITE_0x06400000	= 2,
	VRAM_A_MAIN_SPRITE_0x06420000	= 2 | VRAM_OFFSET(1),
	VRAM_A_TEXTURE = 3,
	VRAM_A_TEXTURE_SLOT0	= 3 | VRAM_OFFSET(0),
	VRAM_A_TEXTURE_SLOT1	= 3 | VRAM_OFFSET(1),
	VRAM_A_TEXTURE_SLOT2	= 3 | VRAM_OFFSET(2),
	VRAM_A_TEXTURE_SLOT3	= 3 | VRAM_OFFSET(3)
} VRAM_A_TYPE;

typedef enum {
	VRAM_B_LCD = 0,
	VRAM_B_MAIN_BG	= 1 | VRAM_OFFSET(1),
	VRAM_B_MAIN_BG_0x06000000	= 1 | VRAM_OFFSET(0),
	VRAM_B_MAIN_BG_0x06020000	= 1 | VRAM_OFFSET(1),
	VRAM_B_MAIN_BG_0x06040000	= 1 | VRAM_OFFSET(2),
	VRAM_B_MAIN_BG_0x06060000	= 1 | VRAM_OFFSET(3),
	VRAM_B_MAIN_SPRITE	= 2 | VRAM_OFFSET(1),
	VRAM_B_MAIN_SPRITE_0x06400000	= 2,
	VRAM_B_MAIN_SPRITE_0x06420000	= 2 | VRAM_OFFSET(1),
	VRAM_B_TEXTURE	= 3 | VRAM_OFFSET(1),
	VRAM_B_TEXTURE_SLOT0	= 3 | VRAM_OFFSET(0),
	VRAM_B_TEXTURE_SLOT1	= 3 | VRAM_OFFSET(1),
	VRAM_B_TEXTURE_SLOT2	= 3 | VRAM_OFFSET(2),
	VRAM_B_TEXTURE_SLOT3	= 3 | VRAM_OFFSET(3)
} VRAM_B_TYPE;

typedef enum {
	VRAM_C_LCD = 0,
	VRAM_C_MAIN_BG  = 1 | VRAM_OFFSET(2),
	VRAM_C_MAIN_BG_0x06000000	= 1 | VRAM_OFFSET(0),
	VRAM_C_MAIN_BG_0x06020000	= 1 | VRAM_OFFSET(1),
	VRAM_C_MAIN_BG_0x06040000	= 1 | VRAM_OFFSET(2),
	VRAM_C_MAIN_BG_0x06060000	= 1 | VRAM_OFFSET(3),
	VRAM_C_ARM7	= 2,
	VRAM_C_ARM7_0x06000000 = 2,
	VRAM_C_ARM7_0x06020000 = 2 | VRAM_OFFSET(1),
	VRAM_C_SUB_BG	= 4,
	VRAM_C_SUB_BG_0x06200000	= 4 | VRAM_OFFSET(0),
	VRAM_C_SUB_BG_0x06220000	= 4 | VRAM_OFFSET(1),
	VRAM_C_SUB_BG_0x06240000	= 4 | VRAM_OFFSET(2),
	VRAM_C_SUB_BG_0x06260000	= 4 | VRAM_OFFSET(3),
	VRAM_C_TEXTURE	= 3 | VRAM_OFFSET(2),
	VRAM_C_TEXTURE_SLOT0	= 3 | VRAM_OFFSET(0),
	VRAM_C_TEXTURE_SLOT1	= 3 | VRAM_OFFSET(1),
	VRAM_C_TEXTURE_SLOT2	= 3 | VRAM_OFFSET(2),
	VRAM_C_TEXTURE_SLOT3	= 3 | VRAM_OFFSET(3)
} VRAM_C_TYPE;

typedef enum {
	VRAM_D_LCD = 0,
	VRAM_D_MAIN_BG  = 1 | VRAM_OFFSET(3),
	VRAM_D_MAIN_BG_0x06000000  = 1 | VRAM_OFFSET(0),
	VRAM_D_MAIN_BG_0x06020000  = 1 | VRAM_OFFSET(1),
	VRAM_D_MAIN_BG_0x06040000  = 1 | VRAM_OFFSET(2),
	VRAM_D_MAIN_BG_0x06060000  = 1 | VRAM_OFFSET(3),
	VRAM_D_ARM7 = 2 | VRAM_OFFSET(1),
	VRAM_D_ARM7_0x06000000 = 2,
	VRAM_D_ARM7_0x06020000 = 2 | VRAM_OFFSET(1),
	VRAM_D_SUB_SPRITE  = 4,
	VRAM_D_TEXTURE = 3 | VRAM_OFFSET(3),
	VRAM_D_TEXTURE_SLOT0 = 3 | VRAM_OFFSET(0),
	VRAM_D_TEXTURE_SLOT1 = 3 | VRAM_OFFSET(1),
	VRAM_D_TEXTURE_SLOT2 = 3 | VRAM_OFFSET(2),
	VRAM_D_TEXTURE_SLOT3 = 3 | VRAM_OFFSET(3)
} VRAM_D_TYPE;

typedef enum {
	VRAM_E_LCD             = 0,
	VRAM_E_MAIN_BG         = 1,
	VRAM_E_MAIN_SPRITE     = 2,
	VRAM_E_TEX_PALETTE     = 3,
	VRAM_E_BG_EXT_PALETTE  = 4,
	VRAM_E_OBJ_EXT_PALETTE = 5,
} VRAM_E_TYPE;

typedef enum {
	VRAM_F_LCD             = 0,
	VRAM_F_MAIN_BG         = 1,
	VRAM_F_MAIN_SPRITE     = 2,
	VRAM_F_MAIN_SPRITE_0x06000000     = 2,
	VRAM_F_MAIN_SPRITE_0x06004000     = 2 | VRAM_OFFSET(1),
	VRAM_F_MAIN_SPRITE_0x06010000     = 2 | VRAM_OFFSET(2),
	VRAM_F_MAIN_SPRITE_0x06014000     = 2 | VRAM_OFFSET(3),
	VRAM_F_TEX_PALETTE     = 3,
	VRAM_F_BG_EXT_PALETTE  = 4,
	VRAM_F_OBJ_EXT_PALETTE = 5,
} VRAM_F_TYPE;

typedef enum {
	VRAM_G_LCD             = 0,
	VRAM_G_MAIN_BG         = 1,
	VRAM_G_MAIN_SPRITE     = 2,
	VRAM_G_MAIN_SPRITE_0x06000000     = 2,
	VRAM_G_MAIN_SPRITE_0x06004000     = 2 | VRAM_OFFSET(1),
	VRAM_G_MAIN_SPRITE_0x06010000     = 2 | VRAM_OFFSET(2),
	VRAM_G_MAIN_SPRITE_0x06014000     = 2 | VRAM_OFFSET(3),
	VRAM_G_TEX_PALETTE     = 3,
	VRAM_G_BG_EXT_PALETTE  = 4,
	VRAM_G_OBJ_EXT_PALETTE = 5,
} VRAM_G_TYPE;

typedef enum {
	VRAM_H_LCD                = 0,
	VRAM_H_SUB_BG             = 1,
	VRAM_H_SUB_BG_EXT_PALETTE = 2,
} VRAM_H_TYPE;

typedef enum {
	VRAM_I_LCD                    = 0,
	VRAM_I_SUB_BG                 = 1,
	VRAM_I_SUB_SPRITE             = 2,
	VRAM_I_SUB_SPRITE_EXT_PALETTE = 3,
}VRAM_I_TYPE;

// main display only
#define MODE_FIFO      (3<<16)

// The next two defines only apply to MAIN 2d engine
// In tile modes, this is multiplied by 64KB and added to BG_TILE_BASE
// In all bitmap modes, it is not used.
#define DISPLAY_CHAR_BASE(n) (((n)&7)<<24)

// In tile modes, this is multiplied by 64KB and added to BG_MAP_BASE
// In bitmap modes, this is multiplied by 64KB and added to BG_BMP_BASE
// In large bitmap modes, this is not used
#define DISPLAY_SCREEN_BASE(n) (((n)&7)<<27)

// 3D core control

#define GFX_CONTROL           (*(volatile ushort*) 0x04000060)

#define GFX_FIFO              (*(volatile ulong*) 0x04000400)
#define GFX_STATUS            (*(volatile ulong*) 0x04000600)
#define GFX_COLOR             (*(volatile ulong*) 0x04000480)

#define GFX_VERTEX10          (*(volatile ulong*) 0x04000490)
#define GFX_VERTEX_XY         (*(volatile ulong*) 0x04000494)
#define GFX_VERTEX_XZ         (*(volatile ulong*) 0x04000498)
#define GFX_VERTEX_YZ         (*(volatile ulong*) 0x0400049C)
#define GFX_VERTEX_DIFF       (*(volatile ulong*) 0x040004A0)

#define GFX_VERTEX16          (*(volatile ulong*) 0x0400048C)
#define GFX_TEX_COORD         (*(volatile ulong*) 0x04000488)
#define GFX_TEX_FORMAT        (*(volatile ulong*) 0x040004A8)
#define GFX_PAL_FORMAT        (*(volatile ulong*) 0x040004AC)

#define GFX_CLEAR_COLOR       (*(volatile ulong*) 0x04000350)
#define GFX_CLEAR_DEPTH       (*(volatile ushort*) 0x04000354)

#define GFX_LIGHT_VECTOR      (*(volatile ulong*) 0x040004C8)
#define GFX_LIGHT_COLOR       (*(volatile ulong*) 0x040004CC)
#define GFX_NORMAL            (*(volatile ulong*) 0x04000484)

#define GFX_DIFFUSE_AMBIENT   (*(volatile ulong*) 0x040004C0)
#define GFX_SPECULAR_EMISSION (*(volatile ulong*) 0x040004C4)
#define GFX_SHININESS         (*(volatile ulong*) 0x040004D0)

#define GFX_POLY_FORMAT       (*(volatile ulong*) 0x040004A4)
#define GFX_ALPHA_TEST        (*(volatile ushort*) 0x04000340)

#define GFX_BEGIN			(*(volatile ulong*) 0x04000500)
#define GFX_END				(*(volatile ulong*) 0x04000504)
#define GFX_FLUSH			(*(volatile ulong*) 0x04000540)
#define GFX_VIEWPORT		(*(volatile ulong*) 0x04000580)
#define GFX_TOON_TABLE		((volatile ushort*)  0x04000380)
#define GFX_EDGE_TABLE		((volatile ushort*)  0x04000330)
#define GFX_FOG_COLOR		(*(volatile ulong*) 0x04000358)
#define GFX_FOG_OFFSET		(*(volatile ulong*) 0x0400035C)
#define GFX_FOG_TABLE		((volatile uchar*)   0x04000360)
#define GFX_BOX_TEST		(*(volatile long*)  0x040005C0)
#define GFX_POS_TEST		(*(volatile ulong*) 0x040005C4)
#define GFX_POS_RESULT		((volatile long*)   0x04000620)

#define GFX_BUSY (GFX_STATUS & BIT(27))

#define GFX_VERTEX_RAM_USAGE	(*(uint16*)  0x04000606)
#define GFX_POLYGON_RAM_USAGE	(*(uint16*)  0x04000604)

#define GFX_CUTOFF_DEPTH		(*(uint16*)0x04000610)

// Matrix processor control

#define MATRIX_CONTROL		(*(volatile ulong*)0x04000440)
#define MATRIX_PUSH			(*(volatile ulong*)0x04000444)
#define MATRIX_POP			(*(volatile ulong*)0x04000448)
#define MATRIX_SCALE		(*(volatile long*) 0x0400046C)
#define MATRIX_TRANSLATE	(*(volatile long*) 0x04000470)
#define MATRIX_RESTORE		(*(volatile ulong*)0x04000450)
#define MATRIX_STORE		(*(volatile ulong*)0x0400044C)
#define MATRIX_IDENTITY		(*(volatile ulong*)0x04000454)
#define MATRIX_LOAD4x4		(*(volatile long*) 0x04000458)
#define MATRIX_LOAD4x3		(*(volatile long*) 0x0400045C)
#define MATRIX_MULT4x4		(*(volatile long*) 0x04000460)
#define MATRIX_MULT4x3		(*(volatile long*) 0x04000464)
#define MATRIX_MULT3x3		(*(volatile long*) 0x04000468)

//matrix operation results
#define MATRIX_READ_CLIP		((volatile long*) (0x04000640))
#define MATRIX_READ_VECTOR		((volatile long*) (0x04000680))
#define POINT_RESULT			((volatile long*) (0x04000620))
#define VECTOR_RESULT			((volatile ushort*)(0x04000630))
