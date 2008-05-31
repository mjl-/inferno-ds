#include <u.h>
#include "../port/lib.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

static QLock getl, putl;
static Rendez getr, putr;

static int
fifocanput(void*)
{
	return (FIFOREG->ctl & FifoTfull) == 0;
}

static int
fifocanget(void*)
{
	return (FIFOREG->ctl & FifoRempty) == 0;
}

void
fifoput(ulong cmd, ulong param)
{
	qlock(&putl);
	sleep(&putr, fifocanput, nil);
	FIFOREG->send = (cmd|param<<Fcmdwidth);
	qunlock(&putl);
}

ulong
fifoget(ulong *param)
{
	ulong r;

	qlock(&getl);
	sleep(&getr, fifocanget, nil);
	r = FIFOREG->recv;
	qunlock(&getl);

	if(param != nil)
		*param = r>>Fcmdwidth;
	return r&Fcmdwidth;
}

extern void fiforecv(ulong);

static void
fiforxintr(Ureg*, void*)
{
	ulong v;

	while(!(FIFOREG->ctl & FifoRempty)) {
		v = FIFOREG->recv;
		fiforecv(v);
	}
	intrclear(FRECVbit, 0);
}

static void
fifotxintr(Ureg*, void*)
{
	if(FIFOREG->ctl & FifoTfull)
		return;

	wakeup(&putr);
	intrclear(FSENDbit, 0);
}

static void
fifoinit(void (*rxintr)(Ureg *, void*), void (*txintr)(Ureg *, void*))
{
	FIFOREG->ctl = (FifoTirq|FifoRirq|Fifoenable|FifoTflush);
	intrenable(0, FSENDbit, txintr, nil, "txintr");
	intrenable(0, FRECVbit, rxintr, nil, "rxintr");
}

void
fifolink(void){
	fifoinit(fiforxintr, fifotxintr);
}
