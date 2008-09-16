/* hardware accelerated division and square root */

#include "u.h" 
#include "../port/lib.h" 
#include "mem.h"
#include "io.h"
#include "dat.h"
#include "fns.h"

/* TODO: these versions could replace _div/_mod */

enum {
	Divbase =	0x4000280,
	Sqrtbase =	0x40002b0,

	/* ctl bits */
	Busy =		1<<15,	/* for both ctl registers */

	Divbyzero =	1<<14,
	Div32 =		0,
	Div64 =		1<<1,

	Sqrt32 =	0,
	Sqrt64 =	1<<0,
};

#define DIVREG ((DivReg*)Divbase)
typedef struct DivReg DivReg;
struct DivReg{
	ushort ctl;

	ulong pad[3];
	ulong numlo;
	ulong numhi;
	ulong denlo;
	ulong denhi;
	ulong divlo;
	ulong divhi;
	ulong modlo;
	ulong modhi;
};

#define SQRTREG	((SqrtReg*)Sqrtbase)
typedef struct SqrtReg SqrtReg;
struct SqrtReg{
	ushort ctl;

	ulong sqrt;
	ulong arglo;
	ulong arghi;
};

static void
wait(ushort *addr)
{
	while(*addr & Busy)
		;
}

static void
_div(int *res, long num, long den)
{	
	wait(&DIVREG->ctl);
	DIVREG->ctl = Div32;
	DIVREG->numlo = num;
	DIVREG->denlo = den;
	DIVREG->denhi = 0;	/* ensure Divbyzero will be set correctly */
	wait(&DIVREG->ctl);

	/* xxx panic if div by zero? */
	*res = DIVREG->divlo;
}

static void
_mod(int *res, long num, long den)
{	
	wait(&DIVREG->ctl);
	DIVREG->ctl = Div32;
	DIVREG->numlo = num;
	DIVREG->denlo = den;
	DIVREG->denhi = 0;	/* ensure Divbyzero will be set correctly */
	wait(&DIVREG->ctl);

	/* xxx panic if div by zero? */
	*res = DIVREG->modlo;
}

static void
_divu(int *res, ulong num, ulong den)
{
	_div(res, num, den);
}

static void
_modu(int *res, ulong *num, ulong den)
{	
	_mod(res, (long)num, (long)den);
}

static void
_divv(vlong *res, vlong num, vlong den)
{
	uvlong r;

	wait(&DIVREG->ctl);
	DIVREG->ctl = Div64;
	DIVREG->numlo = num;
	DIVREG->numhi = num>>32;
	DIVREG->denlo = den;
	DIVREG->denhi = den>>32;
	wait(&DIVREG->ctl);

	/* xxx panic if div by zero? */
	*res = (uvlong)(DIVREG->divhi)<<32|DIVREG->divlo;
}

static void
_modv(vlong *res, vlong num, vlong den)
{
	uvlong r;

	wait(&DIVREG->ctl);
	DIVREG->ctl = Div64;
	DIVREG->numlo = num;
	DIVREG->numhi = num>>32;
	DIVREG->denlo = den;
	DIVREG->denhi = den>>32;
	wait(&DIVREG->ctl);

	/* xxx panic if div by zero? */
	*res = (uvlong)(DIVREG->modhi)<<32|DIVREG->modlo;
}


ulong
lsqrt(ulong p)
{
	wait(&SQRTREG->ctl);
	SQRTREG->ctl = Sqrt32;
	SQRTREG->arglo = p;
	wait(&SQRTREG->ctl);

	return SQRTREG->sqrt;
}

ulong
vlsqrt(uvlong p)
{
	wait(&SQRTREG->ctl);
	SQRTREG->ctl = Sqrt64;
	SQRTREG->arglo = p;
	SQRTREG->arghi = p>>32;
	wait(&SQRTREG->ctl);

	return SQRTREG->sqrt;
}

/* TODO: not ready yet */
void
mathlink(void){
/*
	int a, b, r;
	int tk1, tk2;

	a = 500;
	b = 100;
	r = a / b;

	a = 500;
	b = 100;
	r =  a % b;

	tk1 = m->ticks;
	for (a=1; a<1000; a++)
		if(1){
			_div(&r, a, 3);
			_mod(&r, a, 3);
		}else{
			_div(&r, a, 3);
			_mod(&r, a, 3);

			print(""
			"_div(%d, 3) = %ld\n"
			"_mod(%d, 3) = %ld\n",
			a, r, a, r
			);
		}
	tk1 = m->ticks - tk1;

	tk2 = m->ticks;
	for (a=1; a<1000; a++)
		if(1){
			r = a / 3;
			r = a % 3;
		}else{
			r = a / 3;
			r = a % 3;

			print(""
			"_div(%d, 3) = %d\n"
			"_mod(%d, 3) = %d\n",
			a, r, a, r
			);
		}
	tk2 = m->ticks - tk2;

	print("math tk %ld tk1 %d tk2 %d\n", m->ticks, tk1, tk2);
*/
}
