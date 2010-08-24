#include <u.h>
#include "../mem.h"
#include "../io.h"
#include "fns.h"
#include <kern.h>

typedef struct IrqEntry IrqEntry;
struct IrqEntry{
	void (*r)(void*);
	void *a;
	int v;
};

enum
{
	NumIRQbits = MaxIRQbit+1,
};

static IrqEntry Irq[NumIRQbits];

static void
intrs(void)
{
	IrqEntry *cur;
	int i, ibits;

	ibits = INTREG->ipr;
	for(i=0; i < nelem(Irq) && ibits; i++){
		if (ibits & (1<<i)){
			cur = &Irq[i];
			if (cur->r != nil){
				INTREG->ime= 0;
				cur->r(cur->a);
				INTREG->ime = 1;
				
				ibits &= ~(1<<i);
			}
		}
	}

	// ack. unhandled ints
	if (ibits != 0){
		INTREG->ipr = ibits;
		*(ulong*)IRQCHECK7 = ibits;
	}
}

static void
panic(void)
{
	print("panic7!\n");
	while(1);
}

void
trapinit(void)
{
	int i;
	
	INTREG->ime = 0;
	INTREG->ier = 0;
	INTREG->ipr = ~0;

	*(ulong*)INTHAND7 = (ulong)intrs;
	*(ulong*)EXCHAND7 = (ulong)panic;

	for(i = 0; i < nelem(Irq); i++){
		Irq[i].r = nil;
		Irq[i].a = nil;
		Irq[i].v = i;
	}
	
	INTREG->ime = 1;
}

void
intrclear(int v, int tbdf)
{
	USED(tbdf);
	if(v < 0 || v > MaxIRQbit)
		return;

	INTREG->ipr = (1 << v);
	*(ulong*)IRQCHECK7 = (1 << v);
}

void
intrenable(int v, void (*r)(void*), void *a, int tbdf)
{
	USED(tbdf);
	if(v < 0 || v > MaxIRQbit)
		return;

	Irq[v].r	= r;
	Irq[v].a	= a;
	Irq[v].v	= v;
		
	INTREG->ier |= (1 << v);
	if (VBLANKbit <= v && v <= VCOUNTbit)
		LCDREG->lcsr |= (1 << (DispIrqVblank+v));
}

void
intrmask(int v, int tbdf)
{
	USED(tbdf);
	if(v < 0 || v > MaxIRQbit)
		return;
	
	INTREG->ier &= ~(1<<v);
	if (VBLANKbit <= v && v <= VCOUNTbit)
		LCDREG->lcsr &= ~(1<<(DispIrqVblank+v));
}

void
intrunmask(int v, int tbdf)
{
	USED(tbdf);
	if(v < 0 || v > MaxIRQbit)
		return;
	INTREG->ier |= (1 << v);
}
