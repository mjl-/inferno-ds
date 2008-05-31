#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/netif.h"

#include "etherif.h"

typedef struct Ctlr Ctlr;
struct Ctlr {
	Lock;
	uchar	*base;
	int	type;
	int	rev;
	int	hasmii;
	int	phyad;
	int	bank;	/* currently selected bank */
	Block*	waiting;	/* waiting for space in FIFO */

	ulong	collisions;
	ulong	toolongs;
	ulong	tooshorts;
	ulong	aligns;
	ulong	txerrors;
	int	oddworks;
	int	bus32bit;
};

#define DPRINT if(0)iprint

static long
ifstat(Ether* ether, void* a, long n, ulong offset)
{
	USED(ether, a, n, offset);
	DPRINT("ifstat\n");
	return n;
}

static void
promiscuous(void* arg, int on)
{
	USED(arg, on);
	DPRINT("promiscuous\n");
}

static void
attach(Ether *ether)
{
	USED(ether);
	DPRINT("attach\n");
}

static void
transmit(Ether *ether)
{
	USED(ether);
	DPRINT("transmit\n");
}

static void
interrupt(Ureg*, void *arg)
{
	DPRINT("interrupt\n");
}

static int
etherndsreset(Ether* ether)
{
	int i;
	char *p;
	uchar ea[Eaddrlen];
	ushort rev;

	DPRINT("etherndsreset\n");
	if(ether->ctlr == nil){
		ether->ctlr = malloc(sizeof(Ctlr));
		if(ether->ctlr == nil)
			return -1;
	}

	/*
	 * do architecture dependent intialisation
	 */

	/*
	 * Identify the chip by reading...
	 * 1) the bank select register - the top byte will be 0x33
	 * 2) changing the bank to see if it reads back appropriately
	 * 3) check revision register for code 9
	 */

	memset(ea, 0, sizeof(ea));
	if(memcmp(ether->ea, ea, Eaddrlen) == 0)
		panic("ethernet address not set");

	/*
	 * Linkage to the generic ethernet driver.
	 */
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->ifstat = ifstat;

	ether->arg = ether;
	ether->promiscuous = promiscuous;

	return 0;
}

void
etherndslink(void)
{
	addethercard("nds",  etherndsreset);
}
