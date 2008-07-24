#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "ureg.h"

static ulong timer_incr[4] = { 0, 0, 0, 0};

typedef struct Clock0link Clock0link;
typedef struct Clock0link {
	void		(*clock)(void);
	Clock0link*	link;
} Clock0link;

static Clock0link *clock0link;
static Lock clock0lock;
static void (*prof_fcn)(Ureg *, int);

Timer*
addclock0link(void (*clock)(void), int)
{
	Clock0link *lp;

	if((lp = malloc(sizeof(Clock0link))) == 0){
		print("addclock0link: too many links\n");
		return nil;
	}	
//	print("clocklink key: 0x%lux", clock0lock.key);	
	ilock(&clock0lock);
	lp->clock = clock;
	lp->link = clock0link;
	clock0link = lp;
	iunlock(&clock0lock);
	return nil;
}

static void
profintr(Ureg *ur, void*)
{
#ifdef PROF
	OstmrReg *ost = OSTMRREG;
	int t;

	if ((ost->osmr[3] - ost->oscr) < 2*TIMER_HZ)
	{
		/* less than 2 seconds before reset, say something */
		setpanic();
		clockpoll();
		dumpregs(ur);
		panic("Watchdog timer will expire");
	}

	/* advance the profile clock tick */
	ost->osmr[2] += timer_incr[2];
	ost->ossr = (1 << 2); 			/* Clear the SR */
	t = 1;
	while((ost->osmr[2] - ost->oscr) > 0x80000000) {
		ost->osmr[2] += timer_incr[2];
		t++;
	}
	if (prof_fcn)
		prof_fcn(ur, t);
#else
	USED(ur);
#endif
}

static void
clockintr(Ureg *ur, void *a)
{
	Clock0link *lp;
	USED(ur, a);

	m->ticks++;
	
	checkalarms();

	if(canlock(&clock0lock)){
		for(lp = clock0link; lp; lp = lp->link){
			if(0)print("clki %lud clkf %lux\n", m->ticks, lp->clock);
			if (lp->clock)
				lp->clock();
		}
		unlock(&clock0lock);
	}
	intrclear(TIMER0bit, 0);
}

void
timerdisable( int timer )
{
	if ((timer < 0) || (timer > 3))
		return;
	intrmask(TIMER0bit+timer, 0);
}

void
timerenable( int timer, int Hz, void (*f)(Ureg *, void*), void* a)
{
	TimerReg *t = TIMERREG + timer;
	if ((timer < 0) || (timer > 3))
		return;
	timerdisable(timer);
	timer_incr[timer] = TIMER_BASE(Tmrdiv1024) / Hz;	/* set up freq */
	t->data = timer_incr[timer];
	t->ctl = Tmrena | Tmrdiv1024 | Tmrirq;
	intrenable(0, TIMER0bit+timer, f, a, 0);
}

void
installprof(void (*pf)(Ureg *, int))
{
#ifdef PROF
	prof_fcn = pf;
	timerenable( 2, HZ+1, profintr, 0);
	timer_incr[2] = timer_incr[0]+63;	/* fine tuning */
#else
	USED(pf);
#endif
}

void
clockinit(void)
{
	m->ticks = 0;
	timerenable(0, HZ, clockintr, 0);
}

void
clockpoll(void)
{
}

void
clockcheck(void)
{
}

// macros for fixed-point math

ulong _mularsv(ulong m0, ulong m1, ulong a, ulong s);

/* truncated: */
#define FXDPTDIV(a,b,n) ((ulong)(((uvlong)(a) << (n)) / (b)))
#define MAXMUL(a,n)     ((ulong)((((uvlong)1<<(n))-1)/(a)))
#define MULDIV(x,a,b,n) (((x)*FXDPTDIV(a,b,n)) >> (n)) 
#define MULDIV64(x,a,b,n) ((ulong)_mularsv(x, FXDPTDIV(a,b,n), 0, (n)))

/* rounded: */
#define FXDPTDIVR(a,b,n) ((ulong)((((uvlong)(a) << (n))+((b)/2)) / (b)))
#define MAXMULR(a,n)     ((ulong)((((uvlong)1<<(n))-1)/(a)))
#define MULDIVR(x,a,b,n) (((x)*FXDPTDIVR(a,b,n)+(1<<((n)-1))) >> (n)) 
#define MULDIVR64(x,a,b,n) ((ulong)_mularsv(x, FXDPTDIVR(a,b,n), 1<<((n)-1), (n)))


// these routines are all limited to a maximum of 1165 seconds,
// due to the wrap-around of the OSTIMER

ushort
timer_start(void)
{
	return TIMERREG->data;
}

ushort
timer_ticks(ulong t0)
{
	return TIMERREG->data - t0;
}

int
timer_devwait(ulong *adr, ulong mask, ulong val, int ost)
{
	int i;
	ushort t0 = timer_start();
	while((*adr & mask) != val) 
		if(timer_ticks(t0) > ost)
			return ((*adr & mask) == val) ? 0 : -1;
		else
			for (i = 0; i < 10; i++);	/* don't pound OSCR too hard! (why not?) */
	return 0;
}

void
timer_setwatchdog(int t)
{
	USED(t);
}

void
timer_delay(int t)
{	
	ulong t0 = timer_start();
	while(timer_ticks(t0) < t)
		;
}


ulong
us2tmr(int us)
{
	return MULDIV64(us, CLOCKFREQ, 1000000, 24);
}

int
tmr2us(ulong t)
{
	return MULDIV64(t, 1000000, CLOCKFREQ, 24);
}

void
microdelay(int us)
{
	ulong t0 = timer_start();
	ulong t = us2tmr(us);
	while(timer_ticks(t0) <= t)
		;
}


ulong
ms2tmr(int ms)
{
	return MULDIV64(ms, CLOCKFREQ, 1000, 20);
}

int
tmr2ms(ulong t)
{
	return MULDIV64(t, 1000, CLOCKFREQ, 32);
}

void
delay(int ms)
{
	ulong t0 = timer_start();
	ulong t = ms2tmr(ms);
	while(timer_ticks(t0) <= t)
		clockpoll();
}

int
srand()
{
	return 0;
}

int
time()
{
	return 0;
}

uvlong
fastticks(uvlong *hz)
{
	if(hz)
		*hz = HZ;
	return m->ticks;
}
