#include <u.h>
#include "../port/lib.h"
#include "dat.h"
#include "fns.h"
#include "fifo.h"
#include "io.h"

extern void fiforecv(ulong);

static QLock getl, putl;
static Rendez getr, putr;


/* xxx find a soluation to in* and out*.  now abusing functions from devregs.c */
uchar	inb(int reg);
ushort	insh(int reg);
ulong	inl(int reg);
void	outb(int reg, uchar b);
void	outsh(int reg, ushort sh);
void	outl(int reg, ulong l);


void
fifolink(void)
{
	outsh(Fifoctl, FifoTirq|FifoRirq|Fifoenable|FifoTflush);
	intrenable(0, FSENDbit, fifosendintr, nil, "fifosend");
	intrenable(0, FRECVbit, fiforecvintr, nil, "fiforecv");
}

static int
fifocanput(void*)
{
	return (insh(Fifoctl) & FifoTfull) == 0;
}

static int
fifocanget(void*)
{
	return (insh(Fifoctl) & FifoRempty) == 0;
}

void
fifoput(ulong cmd, ulong param)
{
	qlock(&putl);
	sleep(&putr, fifocanput, nil);
	outl(Fifosend, cmd|param<<Fcmdwidth);
	qunlock(&putl);
}

ulong
fifoget(ulong *param)
{
	ulong r;

	qlock(&getl);
	sleep(&getr, fifocanget, nil);
	r = inl(Fiforecv);
	qunlock(&getl);

	if(param != nil)
		*param = r>>Fcmdwidth;
	return r&Fcmdwidth;
}

void
fiforecvintr(Ureg*, void*)
{
	ulong v;

	if(insh(Fifoctl) & FifoRempty)
		return;

	v = inl(Fiforecv);
	fiforecv(v);
	intrclear(FRECVbit, 0);
}

void
fifosendintr(Ureg*, void*)
{
	if(insh(Fifoctl) & FifoTfull)
		return;

	wakeup(&putr);
	intrclear(FSENDbit, 0);
}
