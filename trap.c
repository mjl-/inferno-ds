#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"lcdreg.h"

#include	"ureg.h"
#include	"../port/error.h"

#define waslo(sr) (!((sr) & (PsrDirq|PsrDfiq)))

typedef struct IrqEntry {
		void	(*r)(Ureg*, void*);
		void	*a;
		int	v;
} IrqEntry;

enum
{
	NumIRQbits = MaxIRQbit+1,
};

static IrqEntry Irq[NumIRQbits];

Instr BREAK = 0xE6BAD010;

int (*breakhandler)(Ureg*, Proc*);
int (*catchdbg)(Ureg *, uint);

void dumperrstk(void);
/*
 * Interrupt sources not masked by splhi() -- these are special
 *  interrupt handlers (e.g. profiler or watchdog), not allowed
 *  to share regular kernel data structures.  All interrupts are
 *  masked by splfhi(), which should only be used herein.
 */

int splfhi(void);	/* disable all */
int splflo(void);	/* enable FIQ */

static int actIrq = -1;	/* Active Irq handler, 0-31, or -1 if none */
static int wasIrq = -1;	/* Interrupted Irq handler */

static Proc *iup;	/* Interrupted kproc */

void trap(Ureg* ureg);

void
intrmask(int v, int tbdf)
{
	USED(tbdf);
	if(v < 0 || v > MaxIRQbit)
		panic("intrmask: irq source %d out of range\n", v);
	INTREG->ier &= ~(1 << v);
}

void
intrunmask(int v, int tbdf)
{
	USED(tbdf);
	if(v < 0 || v > MaxIRQbit)
		panic("intrunmask: irq source %d out of range\n", v);
	INTREG->ier |= (1 << v);
}

/* get rid of interrupts, i.e. after you've received one, probably unnecessary for ds. */
void
intrclear(int v, int tbdf)
{
	USED(tbdf);
	if(v < 0 || v > MaxIRQbit)
		panic("intrclear: irq source %d out of range\n", v);
	
	INTREG->ipr = (1 << v);
	*((ulong*)IRQCHECK9) = (1 << v);
}

void
intrenable(int sort, int v, void (*f)(Ureg*, void*), void* a, char *tbdf)
{
	int x;

	USED(tbdf);
	if(v < 0 || v > MaxIRQbit)
		panic("intrenable: irq source %d out of range\n", v);
	Irq[v].r = f;
	Irq[v].a = a;

	x = splfhi();

	/* Enable the interrupt by setting the enable bit */
	INTREG->ier |= (1 << v);
	if (v==VBLANKbit | v==HBLANKbit | v==VCOUNTbit)
		*((ulong*)DISPSTAT) |= (1 << v);

	splx(x);
}

static void
trapv(int off, void (*f)(void))
{
	ulong *vloc;
	int offset;
	vloc = (ulong *)(off+0xFFFF0000);
	offset = (((ulong *) f) - vloc)-2;
	*vloc = (0xea << 24) | offset;
}

static void
maskallints(void)
{
	INTREG->ier = 0x0;	/* disable all interrupts */
}

static void
intrs(Ureg *ur, ulong ibits)
{
	IrqEntry *cur;
	int i, s;

	for(i=0; i<nelem(Irq) && ibits; i++)
		if(ibits & (1<<i)){
			cur = &Irq[i];
			if(cur->r != nil){
				actIrq = cur->v; /* show active interrupt handler */
				up = 0;		/* Make interrupted process invisible */

				INTREG->ime = 0;
				cur->r(ur, cur->a);
				INTREG->ime = 1;

				ibits &= ~(1<<i);
			}
		}

	if(ibits != 0){
		iprint("spurious irq interrupt: %8.8lux\n", ibits);
		s = splfhi();
		INTREG->ipr &= ibits;
		*((ulong*)IRQCHECK9) = ibits;
		splx(s);
	}
}

/* set up interrupts */
void
trapinit(void)
{
	int v;

	INTREG->ime=0;
	INTREG->ier=0;
	INTREG->ipr=~0;

	/* set up stacks for various exceptions */
	setr13(PsrMfiq, m->fiqstack+nelem(m->fiqstack));
	setr13(PsrMirq, m->irqstack+nelem(m->irqstack));
	setr13(PsrMabt, m->abtstack+nelem(m->abtstack));
	setr13(PsrMund, m->undstack+nelem(m->undstack));
	
	/* highly accessed data goes to TCM: vectors, Mach0, stacks, ... */
	witcm(0x00000000 | 0x20); /* ITCM base = 0 , size = 32 MB */
	//wdtcm(DTCMZERO | 0x0a);  /* DTCM base = __dtcm_start, size = 16 KB */

	/* set up exception vectors */
	memmove(page0->vectors, vectors, sizeof(page0->vectors));
	memmove(page0->vtable, vtable, sizeof(page0->vtable));

	dcflush(page0, sizeof(*page0));
	icflush(page0, sizeof(*page0));

	if (memcmp(page0->vectors, vectors, sizeof(page0->vectors)) ||
	    memcmp(page0->vtable, vtable, sizeof(page0->vtable)))
		print("trapinit: bad vectors/vtable at %#lux [%d]\n", page0, sizeof(*page0));

	for (v = 0; v < nelem(Irq); v++) {
		Irq[v].r = nil;
		Irq[v].a = (void *)v;
		Irq[v].v = v;
	}

	serwrite = uartputs;
	INTREG->ime=1;
}

static char *_trap_str[PsrMask+1] = {
	[ PsrMfiq ] "Fiq interrupt",
	[ PsrMirq ] "Mirq interrupt",
	[ PsrMsvc ] "SVC/SWI Exception",
	[ PsrMabt ] "Prefetch Abort/Data Abort",
	[ PsrMabt+1 ] "Data Abort",
	[ PsrMund ] "Undefined instruction",
	[ PsrMsys ] "Sys trap"
};

static char *
trap_str(int psr)
{
	char *str = _trap_str[psr & PsrMask];
	if (!str)
		str = "Undefined trap";
	return(str);
}

static void
sys_trap_error(int type)
{
	char errbuf[ERRMAX];
	sprint(errbuf, "sys: trap: %s\n", trap_str(type));
	error(errbuf);
}

void
dflt(Ureg *ureg, ulong far)
{
	char buf[ERRMAX];

	dumpregs(ureg);
	sprint(buf, "trap: fault pc=%8.8lux addr=0x%lux", (ulong)ureg->pc, far);
	disfault(ureg, buf);
}


/*
 *  All traps come here.  It is slower to have all traps ca)
 *  rather than directly vectoring the handler.
 *  However, this avoids
 *  a lot of code dup and possible bugs.
 *  trap is called splfhi().
 */

void
trap(Ureg* ureg)
{
	 //
	 // This is here to make sure that a clock interrupt doesn't
	 // cause the process we just returned into to get scheduled
	 // before it single stepped to the next instruction.
	 //
	static struct {int callsched;} c = {1};
	int itype;
	/*
	 * All interrupts/exceptions should be resumed at ureg->pc-4,
	 * except for Data Abort which resumes at ureg->pc-8.
	 */
	itype = ureg->type;
	if(itype == PsrMabt+1)
		ureg->pc -= 8;
	else
		ureg->pc -= 4;
	ureg->sp = (ulong)(ureg+1);
	if (itype == PsrMirq || itype == PsrMfiq) {	/* Interrupt Request */
		Proc *saveup;
		int t;

		SET(t);
		SET(saveup);

		if (itype == PsrMirq) {
			splflo();	/* Allow nonmasked interrupts */
			if (saveup = up) {
				t = m->ticks;	/* CPU time per proc */
				saveup->pc = ureg->pc;	/* debug info */
				saveup->dbgreg = ureg;
			}
		} else {
					 /* for profiler(wasbusy()): */
			wasIrq = actIrq; /* Save ID of interrupted handler */
			iup = up;	 /* Save ID of interrupted proc */
		}

		/* Use up all the active interrupts */
		intrs(ureg, INTREG->ipr);

		if (itype == PsrMirq) {
			up = saveup;	/* Make interrupted process visible */
			actIrq = -1;	/* No more interrupt handler running */
			preemption(m->ticks - t);
			saveup->dbgreg = nil;
		} else {
			actIrq = wasIrq;
			up = iup;
		}
		return;
	}

	/* All other traps */
	if (ureg->psr & PsrDfiq)
		goto faultpanic;
	if (up)
		up->dbgreg = ureg;
	switch(itype) {

	case PsrMund:				/* Undefined instruction */
		if(*(ulong*)ureg->pc == BREAK && breakhandler) {
			int s;
			Proc *p;

			p = up;
			/* if (!waslo(ureg->psr) || (ureg->pc >= (ulong)splhi && ureg->pc < (ulong)islo))
				p = 0; */
			s = breakhandler(ureg, p);
			if(s == BrkSched) {
				c.callsched = 1;
				sched();
			} else if(s == BrkNoSched) {
				c.callsched = 0;
				if(up)
					up->dbgreg = 0;
				return;
			}
			break;
		}
		if (!up)
			goto faultpanic;
		spllo();
		if (waserror()) {
			if(waslo(ureg->psr) && (up->type == Interp))
				disfault(ureg, up->env->errstr);
			setpanic();
			dumpregs(ureg);
			panic("%s", up->env->errstr);
		}
		if (!fpiarm(ureg)) {
			dumpregs(ureg);
			sys_trap_error(ureg->type);
		}
		poperror();
		break;

	case PsrMsvc:				/* Jump through 0 or SWI */
		if (waslo(ureg->psr) && up && (up->type == Interp)) {
			spllo();
			dumpregs(ureg);
			sys_trap_error(ureg->type);
		}
		goto faultpanic;

	case PsrMabt:				/* Prefetch abort */
		if (catchdbg && catchdbg(ureg, 0))
			break;
		ureg->pc -= 4;
	case PsrMabt+1:	{			/* Data abort */
		if (waslo(ureg->psr) && up && (up->type == Interp)) {
			spllo();
			dflt(ureg, 0);
		}
		goto faultpanic;
	}
	default:				/* ??? */
faultpanic:
		setpanic();
		dumpregs(ureg);
		panic("exception %uX %s\n", ureg->type, trap_str(ureg->type));
		break;
	}

	splhi();
	if(up)
		up->dbgreg = 0;		/* becomes invalid after return from trap */
}

void
setpanic(void)
{
	extern void screenon(int);
	extern int consoleprint;

	if (breakhandler != 0)	/* don't mess up debugger */
		return;
	maskallints();
//	spllo();
	/* screenon(!consoleprint); */
	consoleprint = 1;
	serwrite = uartputs;
}

int
isvalid_wa(void *v)
{
	return((ulong)v >= 0x8000 && (ulong)v < conf.topofmem && !((ulong)v & 3));
}

int
isvalid_va(void *v)
{
	return((ulong)v >= 0x8000 && (ulong)v < conf.topofmem);
}

void
dumplongs(char *msg, ulong *v, int n)
{
	int	ii;
	int	ll;

	ll = print("%s at %ulx: ", msg, v);
	for (ii = 0; ii < n; ii++)
	{
		if (ll >= 60)
		{
			print("\n");
			ll = print("    %ulx: ", v);
		}
		if (isvalid_va(v))
			ll += print(" %ulx", *v++);
		else
		{
			ll += print(" invalid");
			break;
		}
	}
	print("\n");
	USED(ll);
}

void
dumpregs(Ureg* ureg)
{
	Proc *p;
	print("TRAP: %s", trap_str(ureg->type));
	if ((ureg->psr & PsrMask) != PsrMsvc)
		print(" in %s", trap_str(ureg->psr));
/*
	if ((ureg->type == PsrMabt) || (ureg->type == PsrMabt + 1))
		print(" FSR %8.8luX FAR %8.8luX\n", mmuregr(CpFSR), mmuregr(CpFAR));
*/
	print("\n");
	print("PSR %8.8uX type %2.2uX PC %8.8uX LINK %8.8uX\n",
		ureg->psr, ureg->type, ureg->pc, ureg->link);
	print("R14 %8.8uX R13 %8.8uX R12 %8.8uX R11 %8.8uX R10 %8.8uX\n",
		ureg->r14, ureg->r13, ureg->r12, ureg->r11, ureg->r10);
	print("R9  %8.8uX R8  %8.8uX R7  %8.8uX R6  %8.8uX R5  %8.8uX\n",
		ureg->r9, ureg->r8, ureg->r7, ureg->r6, ureg->r5);
	print("R4  %8.8uX R3  %8.8uX R2  %8.8uX R1  %8.8uX R0  %8.8uX\n",
		ureg->r4, ureg->r3, ureg->r2, ureg->r1, ureg->r0);
	print("Stack is at: %8.8luX\n",ureg);
	print("CPSR %8.8uX SPSR %8.8uX ", cpsrr(), spsrr());
	print("PC %8.8lux LINK %8.8lux\n", (ulong)ureg->pc, (ulong)ureg->link);

	p = (actIrq >= 0) ? iup : up;
	if (p != nil)
		print("Process stack:  %lux-%lux\n",
			p->kstack, p->kstack+KSTACK-4);
	else
		print("System stack: %lux-%lux\n",
			(ulong)(m+1), (ulong)m+KSTACK-4);
	dumplongs("stk", (ulong *)(ureg + 1), 16);
	print("bl's: ");
	dumpstk((ulong *)(ureg + 1));
	if (isvalid_wa((void *)ureg->pc))
		dumplongs("code", (ulong *)ureg->pc - 5, 12);

	dumperrstk();
	/* for(;;) ; */
}

void
dumpstack(void)
{
	ulong l;

	if (breakhandler != 0)
		dumpstk(&l);
}

void
dumpstk(ulong *l)
{
	ulong *v, i;
	ulong inst;
	ulong *estk;
	int len;

	len = KSTACK/sizeof *l;
	if (up == 0)
		len -= l - (ulong *)m;
	else
		len -= l - (ulong *)up->kstack;

	if (len > KSTACK/sizeof *l)
		len = KSTACK/sizeof *l;
	else if (len < 0)
		len = 50;

	i = 0;
	for(estk = l + len; l<estk; l++) {
		if (!isvalid_wa(l)) {
			i += print("invalid(%lux)", l);
			break;
		}
		v = (ulong *)*l;
		if (isvalid_wa(v)) {
			inst = *(v - 1);
			if (	(
					((inst & 0x0ff0f000) == 0x0280f000)
					&&
					((*(v-2) & 0x0ffff000) == 0x028fe000)
				)
				||
				((inst & 0x0f000000) == 0x0b000000)
			) {
				i += print("%8.8lux ", v);
			}
		}
		if (i >= 60) {
			print("\n");
			i = print("    ");
		}
	}
	if (i)
		print("\n");
}

void
dumperrstk(void)
{
	int ii, ll;

	if (!up)
		return;

	ll = print("err stk: ");
	for (ii = 0; ii < NERR; ii++) {
		if (ii == up->nerrlab)
			ll += print("* ");
		if (up->errlab[ii].pc) {
			ll += print(" %lux/%8.8lux",
				up->errlab[ii].sp, up->errlab[ii].pc);
			if (ll >= 60) {
				print("\n");
				ll = 0;
			}
		}
	}
	if (ll)
		print("\n");
}

void
trapspecial(int (*f)(Ureg *, uint))
{
	catchdbg = f;
}
