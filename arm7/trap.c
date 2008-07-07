#include <u.h>
#include "../mem.h"
#include "../io.h"
#include "fns.h"
#include <kern.h>
#include "nds.h"

typedef struct IrqEntry IrqEntry;
struct IrqEntry{
	void (*r)(void);
	int v;
};

enum
{
	NumIRQbits = MaxIRQbit+1,
};

static IrqEntry Irq[NumIRQbits];

static void
intrs(void){
	int i;
	int ibits;

	ibits = INTREG->ipr;
	for(i=0; i < nelem(Irq) && ibits; i++){
		if (ibits & (1<<i)){
			if (Irq[i].r != nil){
				INTREG->ime= 0;
				Irq[i].r();
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

void
trapinit(void)
{
	int i;
	
	INTREG->ime = 0;
	INTREG->ier = 0;
	INTREG->ipr = ~0;
	*(ulong*)INTHAND7 = (ulong)intrs;

	for(i = 0; i < nelem(Irq); i++){
		Irq[i].r = nil;
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
intrenable(int v, void (*r)(void), int tbdf)
{
	USED(tbdf);
	if(v < 0 || v > MaxIRQbit)
		return;

	Irq[v].r	= r;
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
