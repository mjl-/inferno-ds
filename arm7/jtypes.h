/*---------------------------------------------------------------------------------
	$Id: jtypes.h,v 1.17 2007/07/18 05:20:45 wntrmute Exp $

	jtypes.h -- Common types (and a few useful macros)

	Copyright (C) 2005
		Michael Noland (joat)
		Jason Rogers (dovoto)
		Dave Murphy (WinterMute)
		Chris Double (doublec)

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any
	damages arising from the use of this software.

	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:

	1.	The origin of this software must not be misrepresented; you
		must not claim that you wrote the original software. If you use
		this software in a product, an acknowledgment in the product
		documentation would be appreciated but is not required.
	2.	Altered source versions must be plainly marked as such, and
		must not be misrepresented as being the original software.
	3.	This notice may not be removed or altered from any source
		distribution.

---------------------------------------------------------------------------------*/

#define GETRAW(name)      (name)
#define GETRAWSIZE(name)  ((int)name##_size)
#define GETRAWEND(name)  ((int)name##_end)

typedef uchar		uint8;
typedef ushort	uint16;
typedef ulong	uint32;
typedef uvlong	uint64;

typedef char		int8;
typedef short		int16;
typedef long		int32;
typedef vlong		int64;

typedef float		float32;
typedef double		float64;

typedef volatile uchar	vuint8;
typedef volatile ushort	vuint16;
typedef volatile ulong	vuint32;
typedef volatile uvlong	vuint64;

typedef volatile char		vint8;
typedef volatile short	vint16;
typedef volatile long	vint32;
typedef volatile vlong	vint64;

typedef volatile float32        vfloat32;
typedef volatile float64        vfloat64;

typedef uchar		byte;

typedef uchar		u8;
typedef ushort	u16;
typedef ulong	u32;
typedef uvlong	u64;

typedef char		s8;
typedef short		s16;
typedef long		s32;
typedef vlong		s64;

typedef volatile u8          vu8;
typedef volatile u16         vu16;
typedef volatile u32         vu32;
typedef volatile u64         vu64;

typedef volatile s8           vs8;
typedef volatile s16          vs16;
typedef volatile s32          vs32;
typedef volatile s64          vs64;

typedef struct touchPosition touchPosition;
struct touchPosition {
	int16	x;
	int16	y;
	int16	px;
	int16	py;
	int16	z1;
	int16	z2;
};

typedef enum { false, true } bool;

// Handy function pointer typedefs
typedef void ( * IntFn)(void);
typedef void (* VoidFunctionPointer)(void);
typedef void (* fp)(void);

