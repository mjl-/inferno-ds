#include "../port/portfns.h"

void	archconfinit(void);
void	archreboot(void);
void	archreset(void);
void	archconsole(void);
int	cistrcmp(char *, char *);
void	clockcheck(void);
void	clockinit(void);
void	clockpoll(void);
#define	coherence()		/* nothing to do for cache coherence for uniprocessor */
uint	cpsrr(void);
void	dcflush(void*, ulong);
void	dcflushall(void);
void	dumplongs(char *, ulong *, int);
void	dumpregs(Ureg* ureg);
void	dumpstk(ulong *);
int	fpiarm(Ureg*);
void	fpinit(void);
ulong	getcallerpc(void*);
ulong	getcpuid(void);
void	gotopc(ulong);
void	idle(void);
void	idlehands(void);

void	icflush(void*, ulong);
void	icflushall(void);
void	intrenable(int, int, void (*)(Ureg*, void*), void*, char*);
void	intrclear(int, int);
void	intrmask(int, int);
void	intrunmask(int, int);
void	installprof(void (*)(Ureg *, int));
void	kbdinit(void);
void	links(void);
ulong	mpuinit(void);
void*	pa2va(ulong);
#define procsave(p)
#define procrestore(p)
ulong	rcpctl(void);
ulong	rdtcm(void);
ulong	ritcm(void);
void	screeninit(void);
void	(*screenputs)(char*, int);
int	segflush(void*, ulong);
void	setpanic(void);
void	setr13(int, void*);
uint	spsrr(void);
void	trapinit(void);
void	trapspecial(int (*)(Ureg *, uint));
ulong	va2pa(void*);
void	vectors(void);
void	vtable(void);
#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
int	wasbusy(int);
void	_vfiqcall(void);
void	_virqcall(void);
void	_vundcall(void);
void	_vsvccall(void);
void	_vpabcall(void);
void	_vdabcall(void);
ulong	wcpctl(ulong);
ulong	wdtcm(ulong);
ulong	witcm(ulong);

#define KADDR(p)	((void *) p)
#define PADDR(v)	va2pa((void*)(v))

void	delay(int ms);
void 	microdelay(int us);

/* ds port specific */
void	dispfont(void);

long	ldiv(long num, long denom);
vlong	vldiv(vlong num, vlong denom);
ulong	lsqrt(ulong p);
ulong	vlsqrt(uvlong p);

int	nbfifoput(ulong, ulong);
void	fifoput(ulong, ulong);
ulong	fifoget(ulong *);
void	fiforecvintr(Ureg*, void*);
void	fifosendintr(Ureg*, void*);

ulong	getr12(void);
void	setr12(ulong);
