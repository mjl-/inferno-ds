#include "u.h" 
#include "../port/lib.h" 
#include "mem.h"
#include "io.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "version.h"
#include "arm7/jtypes.h"
#include "arm7/ipc.h"

Mach *m = (Mach*)MACHADDR;
Proc *up = 0;
Conf conf;

/* So we can initialize our own data section and bss */
extern char bdata[];
extern char edata[];
extern char end[];

extern ulong kerndate;
extern int cflag;
extern int consoleprint;
extern int redirectconsole;
extern int main_pool_pcnt;
extern int heap_pool_pcnt;
extern int image_pool_pcnt;
extern int kernel_pool_pcnt;

int
segflush(void *p, ulong l)
{
	USED(p, l);
	return 1;
}

static void
poolsizeinit(void)
{
	ulong nb;
	nb = conf.npage*BY2PG;
	poolsize(mainmem, (nb*main_pool_pcnt)/100, 0);
	poolsize(heapmem, (nb*heap_pool_pcnt)/100, 0);
	poolsize(imagmem, (nb*image_pool_pcnt)/100, 1);
}

void
reboot(void)
{
	exit(0);
}

void
halt(void)
{
	spllo();
	print("cpu halted\n");
	while(1);
}

void
confinit(void)
{
	ulong base;

	archconfinit();

	base = PGROUND((ulong)end);
	conf.base0 = base;

	conf.base1 = 0;
	conf.npage1 = 0;

	conf.npage0 = (conf.topofmem - base)/BY2PG;

	conf.npage = conf.npage0 + conf.npage1;
	conf.ialloc = (((conf.npage*(main_pool_pcnt))/100)/2)*BY2PG;

	conf.nproc = 100 + ((conf.npage*BY2PG)/MB)*5;
	conf.nmach = 1;
}

void
machinit(void)
{
	Mach* p;
	int i;
	memset(m, 0, sizeof(Mach));	/* clear the mach struct */
}
void
poolmove(void* p, void* g)
{
	memmove(p, g, 100);
}

void
serputc()
{
	// dummy routine
}
void
testintr()
{
	print("vblank intr\n");
	
}
void
main(void)
{
	char *p, *g, *ep;
	ulong *t;
	ulong i,j,k;
	/* fill out the data section by hand */
/*	memset(edata, 0, end-edata); 	*/	/* clear the BSS */
	poolsetcompact(mainmem, poolmove);
	machinit();
	archreset();
	confinit();
	links();

/* TODO, fix printing, figure out what is wrong with locking and 
	memory addresses  */

	xinit();
	poolinit();
	poolsizeinit();

	trapinit(); 
	clockinit(); 
	printinit();
	m = (Mach*)MACHADDR;

	screeninit();

	i=getdtcm();
	intrenable(0, testintr, 0, 0);
//	*((ulong*)0x04000004)|=1<<3;
	spllo();
	print("addr %lux sz %lux inthand %lux\n",  (i&0xfffff000), i&0x3e, (getdtcm()&0xfffff000)+0x3ffc);
//	print("%lux\n",writedtcmctl(0xdeadbeef));
	print("addr %lux sz %lux inthand %lux\n",  (i&0xfffff000), i&0x3e, (getdtcm()&0xfffff000)+0x3ffc);
	for(;;)
		waitvblank();
	loop();
/*	for(;;);
	for(;;) {
		print("%d %d %d %d %d %d %d\n", IPC->touchX, IPC->touchY, IPC->touchXpx, IPC->touchYpx,IPC->touchZ1, IPC->touchZ2, IPC->buttons);
	} */

	procinit();

	chandevreset();

	eve = strdup("inferno");
	archconsole();
	kbdinit();

	print("\nInferno %s\n", VERSION);
	print("conf %s (%lud) jit %d\n\n", conffile, kerndate, cflag);
	userinit();
	schedinit();
}

void
init0(void)
{
	Osenv *o;


// print("init0\n");
	up->nerrlab = 0;
	spllo();
	if(waserror())
		panic("init0 %r");

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	o = up->env;
	o->pgrp->slash = namec("#/", Atodir, 0, 0);

	cnameclose(o->pgrp->slash->name);
	o->pgrp->slash->name = newcname("/");
	o->pgrp->dot = cclone(o->pgrp->slash);
	print("o->pgrp->dot %ulx\n", o->pgrp->dot);

	chandevinit();
	poperror();
// iprint("init0: disinit\n");
// print("CXXXYYYYYYYYZZZZZZZ\n");
	
	disinit("/osinit.dis");
}

void
userinit()
{
	Proc *p;
	Osenv *o;

	p = newproc();
	o = p->env;

	o->fgrp = newfgrp(nil);

	o->pgrp = newpgrp();
	kstrdup(&o->user, eve);

	strcpy(p->text, "interp");
	p->fpstate = FPINIT;

	/*
	 * Kernel Stack
	 *
	 * N.B. The -12 for the stack pointer is important.
	 *	4 bytes for gotolabel's return PC
	 */
	p->sched.pc = (ulong)init0;
	p->sched.sp = (ulong)p->kstack+KSTACK-8;

	ready(p);
}

void
exit(int inpanic)
{
	up = 0;

	/* Shutdown running devices */
	chandevshutdown();

	if(inpanic){
		print("Hit the reset button\n");
		for(;;)clockpoll();
	}
	archreboot();
}

static void
linkproc(void)
{
	spllo();
	if (waserror())
		print("error() underflow: %r\n");
	else
		(*up->kpfun)(up->arg);
	pexit("end proc", 1);
}

void
kprocchild(Proc *p, void (*func)(void*), void *arg)
{
	p->sched.pc = (ulong)linkproc;
	p->sched.sp = (ulong)p->kstack+KSTACK-8;

	p->kpfun = func;
	p->arg = arg;
}

/* stubs */
void
setfsr(ulong x) {
USED(x);
}

ulong
getfsr(){
return 0;
}

void
setfcr(ulong x) {
USED(x);
}

ulong
getfcr(){
return 0;
}

void
fpinit(void)
{
}

void
FPsave(void*)
{
}

void
FPrestore(void*)
{
}

ulong
va2pa(void *v)
{
	return (ulong)v;
}



static  void IPC_SendSync(unsigned int sync) {
	REG_IPC_SYNC = (REG_IPC_SYNC & 0xf0ff) | (((sync) & 0x0f) << 8) | IPC_SYNC_IRQ_REQUEST;
}


static  int IPC_GetSync() {
	return REG_IPC_SYNC & 0x0f;
}