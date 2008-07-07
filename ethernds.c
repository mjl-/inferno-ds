#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "arm7/jtypes.h"
#include "arm7/wifi.h"

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

/*
 * Communication interface with ARM7.
 * Each of these data structures is aligned on an ARM9 cache line, 
 * to allow independent cache invalidation.
 */
static volatile nds_tx_packet tx_pkt = {0,0,0};
static volatile nds_rx_packet rx_pkt;
static volatile u32 arm7_stats[WIFI_STATS_MAX];
static volatile Wifi_AccessPoint aplist[WIFI_MAX_AP];
static volatile u8 txpaketbuffer[NDS_WIFI_MAX_PACKET_SIZE];

static long
ifstat(Ether* ether, void* a, long n, ulong offset)
{
	USED(ether, a, n, offset);
	DPRINT("ifstat\n");
	
if(1){
	int i;
	Wifi_AccessPoint *app;

	app = (Wifi_AccessPoint*) aplist;
	for(i=0; i < WIFI_MAX_AP && *(ulong*)app; app++)
		print("ssid %s ch %d\n", app->ssid, app->channel);
}
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

/* set scanning interval */
static void
scanbs(void *a, uint secs)
{
	DPRINT("scanbs\n");
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
	 
	/* Initialise stats buffer and send address to ARM7 */
	memset(arm7_stats, 0, sizeof(arm7_stats));
	dcflush(arm7_stats, sizeof(arm7_stats));
	nbfifoput(F9WFstats, (ulong)arm7_stats);
	
	/* Initialise AP list buffer and send address to ARM7 */
	memset(aplist, 0, sizeof(aplist));
	dcflush(aplist, sizeof(aplist));
	nbfifoput(F9WFapquery, (ulong)aplist);
	
	memset(&rx_pkt, 0, sizeof(rx_pkt));
	dcflush(&rx_pkt, sizeof(rx_pkt));
	nbfifoput(F9WFrxpkt, (ulong)&rx_pkt);
	
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
	ether->scanbs = scanbs;
	ether->promiscuous = promiscuous;

	ether->arg = ether;

	return 0;
}

void
etherndslink(void)
{
	addethercard("nds",  etherndsreset);
}
