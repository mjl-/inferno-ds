/* hardware accelerated division and square root */

#include <u.h>

enum {
	Divbase =	0x4000280,
	Divctl =	Divbase+0x00,	/* 16 bit */
	Divnumlo =	Divbase+0x10,
	Divnumhi =	Divbase+0x14,
	Divdenomlo =	Divbase+0x18,
	Divdenomhi =	Divbase+0x1c,
	Divresultlo =	Divbase+0x20,
	Divresulthi =	Divbase+0x24,
	Divremlo =	Divbase+0x28,
	Divremhi =	Divbase+0x2c,

	Sqrtbase =	0x40002b0,
	Sqrtctl =	Sqrtbase+0x00,	/* 16 bit */
	Sqrtresult =	Sqrtbase+0x04,
	Sqrtparamlo =	Sqrtbase+0x08,
	Sqrtparamhi =	Sqrtbase+0x0c,

	Busy =		1<<15,	/* for both ctl registers */

	Divbyzero =	1<<14,
	Div32 =		0,
	Div64 =		2,

	Sqrt32 =	0,
	Sqrt64 =	1,
};

void
wait(ulong addr)
{
	while((*(ulong*)addr) & Busy)
		;
}

long
ldiv(long num, long denom)
{	
	wait(Divctl);
	*(ushort*)Divctl = Div32;
	*(long*)Divnumlo = num;
	*(long*)Divdenomlo = denom;
	*(long*)Divdenomhi = 0;	/* ensure Divbyzero will be set correctly */
	wait(Divctl);
	/* xxx panic if div by zero? */
	return *(long*)Divresultlo;
}

vlong
vldiv(vlong num, vlong denom)
{
	uvlong r;

	wait(Divctl);
	*(ushort*)Divctl = Div64;
	*(ulong*)Divnumlo = num;
	*(ulong*)Divnumhi = num>>32;
	*(ulong*)Divdenomlo = denom;
	*(ulong*)Divdenomhi = denom>>32;
	wait(Divctl);
	/* xxx panic if div by zero? */
	r = (*(ulong*)Divresultlo);
	r |= (uvlong)(*(ulong*)Divresulthi)<<32;
	return (vlong)r;
}

ulong
lsqrt(ulong p)
{
	wait(Sqrtctl);
	*(ushort*)Sqrtctl = Sqrt32;
	*(ulong*)Sqrtparamlo = p;
	wait(Sqrtctl);
	return *(ulong*)Sqrtresult;
}

ulong
vlsqrt(uvlong p)
{
	wait(Sqrtctl);
	*(ushort*)Sqrtctl = Sqrt64;
	*(ulong*)Sqrtparamlo = p;
	*(ulong*)Sqrtparamhi = p>>32;
	wait(Sqrtctl);
	return *(ulong*)Sqrtresult;
}
