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
	ulong		tm;
	Clock0link*	link;
} Clock0link;

static Clock0link *clock0link;
static Lock clock0lock;

static void (*prof_fcn)(Ureg *, int);
static int kproftickena;
void	(*kproftick)(ulong);	/* set by devkprof.c when active */

Timer*
addclock0link(void (*clock)(void), int ticks)
{
	Clock0link *lp;

	if((lp = malloc(sizeof(Clock0link))) == 0){
		print("addclock0link: too many links\n");
		return nil;
	}

	if(clock == nil || ticks <=0)
		return nil;

	ilock(&clock0lock);
	lp->tm = ticks;
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
	TmrReg *ost = TMRREG;
	int t;

	if ((ost->osmr[3] - ost->oscr) < 2*TIMER_HZ)
	{
		/* less than 2 seconds before reset, say something */
		setpanic();
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
	intrclear(TIMER0bit+1, 0);
}

static void
clockintr(Ureg *ur, void *)
{
	Clock0link *lp;

	m->ticks++;
	
	checkalarms();

	if(kproftickena)
		(*kproftick)(ur->pc);

	if(canlock(&clock0lock)){
		for(lp = clock0link; lp; lp = lp->link){
			if(0)print("clkf %lux clkt %lud\n", lp->clock, m->ticks);
			if((m->ticks & lp->tm) == lp->tm) // cheap aprox. to ticks % tm == 0
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
	TimerReg *t = TMRREG + timer;
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
	int s;
	int splfhi(void);

	s = splfhi();
	prof_fcn = pf;
	timerenable(1, HZ+1, profintr, 0);
	timer_incr[1] = timer_incr[0]+63;	/* fine tuning */
	splx(s);
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


static ushort
timer_start(void)
{
	return TMRREG->data;
}

static ushort
timer_ticks(ulong t0)
{
	return TMRREG->data - t0;
}

static void
timer_delay(int t)
{	
	ulong t0 = timer_start();
	while(timer_ticks(t0) <= t)
		; //print("d %ud < %d\n", timer_ticks(t0), t);
}

void
microdelay(int us)
{
	ulong t = US2TMR(us);
	timer_delay(t);
}

void
delay(int ms)
{
	ulong t = MS2TMR(ms);
	timer_delay(t);
}

/*
 * for devbench.c
 */
vlong
archrdtsc(void)
{
	return TMRREG->data;
}

ulong
archrdtsc32(void)
{
	return TMRREG->data;
}

/*
 * for devkprof.c
 */
long
archkprofmicrosecondspertick(void)
{
	return MS2HZ*1000;
}

void
archkprofenable(int on)
{
	kproftickena = on;
}

uvlong
fastticks(uvlong *hz)
{
	if(hz)
		*hz = HZ;
	return m->ticks;
}
