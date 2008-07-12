#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "arm7/jtypes.h"
#include "arm7/wifi.h"

#include "../port/error.h"
#include "../port/netif.h"
#include "etherif.h"

typedef struct Ctlr Ctlr;
struct Ctlr {
	Lock;

	int 	attached;
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

#define DPRINT if(1)iprint

/*
 * Communication interface with ARM7.
 */
static volatile nds_tx_packet tx_pkt = {0,0,0};
static volatile nds_rx_packet rx_pkt;
static volatile u32 stats7[WF_STAT_MAX];
static volatile Wifi_AccessPoint aplist7[WIFI_MAX_AP];
static volatile u8 txpktbuffer[MAX_PACKET_SIZE];

static long
ifstat(Ether* ether, void* a, long n, ulong offset)
{
	Wifi_AccessPoint *app;
	char *p, *e, *tmp;
	int i;

	DPRINT("ifstat\n");
	
	if (ether->ctlr == nil)
		return;

	if(n == 0 || offset != 0)
		return 0;

	tmp = p = smalloc(READSTR);
	if(waserror()) {
		free(tmp);
		nexterror();
	}
	e = tmp+READSTR;

	dcflush(stats7, sizeof(stats7));
	fifoput(F9WFctl, WFCtlstats);
	fifoput(F9WFctl, WFCtlscan);
	
	p = seprint(p, e, "TxPackets: %lud (%lud bytes)\n",
		(ulong) stats7[WF_STAT_TXPKTS], (ulong) stats7[WF_STAT_TXDATABYTES]);
	p = seprint(p, e, "RxPackets: %lud (%lud bytes)\n",
		(ulong) stats7[WF_STAT_RXPKTS], (ulong) stats7[WF_STAT_RXDATABYTES]);
	p = seprint(p, e, "Dropped: %lud\n", stats7[WF_STAT_TXQREJECT]);

	// raw stats	
	p = seprint(p, e, "TxPackets (raw): %lud (%lud bytes)\n",
		(ulong) stats7[WF_STAT_RXRAWPKTS], (ulong) stats7[WF_STAT_TXBYTES]);
	p = seprint(p, e, "RxPackets (raw): %lud (%lud bytes)\n",
		(ulong) stats7[WF_STAT_RXRAWPKTS], (ulong) stats7[WF_STAT_RXBYTES]);
	
	p = seprint(p, e, "WIFI_IE: 0x%ux\n", (ushort) stats7[WF_STAT_DBG1]);
	p = seprint(p, e, "WIFI_IF: 0x%ux\n", (ushort) stats7[WF_STAT_DBG2]);
	p = seprint(p, e, "wifi state: 0x%ux\n", (ushort) stats7[WF_STAT_DBG5]);
	p = seprint(p, e, "Interrupts: 0x%lux\n", (ulong) stats7[WF_STAT_DBG6]);
	
	// order by signal quality 
	app = (Wifi_AccessPoint*) aplist7;
	for(i=0; i < WIFI_MAX_AP && *(ulong*)app; app++)
		p = seprint(p, e, "ssid:%s ch:%d f:%x\n",
			app->ssid, app->rssi, app->flags);

	n = readstr(offset, a, n, tmp);
	poperror();
	free(tmp);
	
	return n;
}

static int
w_option(Ctlr* ctlr, char* buf, long n)
{
	USED(ctlr, buf, n);
	return 0;
}

static long
ctl(Ether* ether, void* buf, long n)
{
	Ctlr *ctlr;

	DPRINT("ctl\n");
	
	if(ether->ctlr == nil)
		error(Enonexist);

	ctlr = ether->ctlr;
	if(ctlr->attached == 0)
		error(Eshutdown);

	ilock(ctlr);
	if(w_option(ctlr, buf, n)){
		iunlock(ctlr);
		error(Ebadctl);
	}

	iunlock(ctlr);

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
	Ctlr* ctlr;
	
	DPRINT("attach\n");
	if (ether->ctlr == nil)
		return;

	ctlr = (Ctlr*) ether->ctlr;
	if (ctlr->attached == 0){
		// TODO goes to archether
		IPCREG->ctl |= Ipcirqena;
		intrenable(ether->itype, ether->irq, ether->interrupt, ether, "nds");

		// enable rx/tx, ...
		fifoput(F9WFctl, WFCtlopen);
		ctlr->attached = 1;
	}
}

static char*
dump_pkt(uchar *data, ushort len)
{
	u8 *c;
	static char buff[2024];
	char *c2;

	c = data;
	c2 = buff;
	while ((c - data) < len) {
		if (((*c) >> 4) > 9)
			*(c2++) = ((*c) >> 4) - 10 + 'A';
		else
			*(c2++) = ((*c) >> 4) + '0';

		if (((*c) & 0x0f) > 9)
			*(c2++) = ((*c) & 0x0f) - 10 + 'A';
		else
			*(c2++) = ((*c) & 0x0f) + '0';
		c++;
		if ((c - data) % 2 == 0)
			*(c2++) = ' ';
	}
	*c2 = '\0';
	return buff;
}

static void
txloadpacket(Ether *ether)
{
	Ctlr *ctlr;
	Block *b;
	int lenb;

	ctlr = ether->ctlr;
	b = ctlr->waiting;
	ctlr->waiting = nil;
	if(b == nil)
		return;	/* shouldn't happen */

	// only transmit one packet at a time
	lenb = BLEN(b);
	
	// wrap up packet information and send it to arm7
	memmove((void *)txpktbuffer, b->rp, lenb);
	tx_pkt.len = lenb;
	tx_pkt.data = (void *)txpktbuffer;
	freeb(b);

	if(0)iprint("dump pkt[%d] @ %x:\n%s",
		tx_pkt.len, tx_pkt.data,
		dump_pkt((uchar*)tx_pkt.data, tx_pkt.len));

	// write data to memory before ARM7 gets hands on
	//dcflush(&tx_pkt, sizeof(tx_pkt));
	dcflush(txpktbuffer, lenb);
	fifoput(F9WFtxpkt, (ulong)&tx_pkt);
}

static void
txstart(Ether *ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	if(ctlr->waiting != nil)	/* allocate pending; must wait for that */
		return;
	for(;;){
		if((ctlr->waiting = qget(ether->oq)) == nil)
			break;
		/* ctlr->waiting is a new block to transmit: allocate space */
		txloadpacket(ether);
	}
}

static void
transmit(Ether *ether)
{
	Ctlr *ctlr;

	DPRINT("transmit\n");
	ctlr = ether->ctlr;
	ilock(ctlr);
	txstart(ether);
	iunlock(ctlr);
}

static void
interrupt(Ureg*, void *arg)
{
	/* TODO
	 * called when arm7 tx/rx a pkt:
	 * use IPCSYNC to emulate a WIFI7 tx/rx intr
	 */
	USED(arg);
	DPRINT("interrupt\n");
}

/* set scanning interval */
static void
scanbs(void *a, uint secs)
{
	USED(a, secs);
	DPRINT("scanbs\n");
}

static int
etherndsreset(Ether* ether)
{
	int i;
	char *p;
	uchar ea[Eaddrlen];
	Ctlr *ctlr;

	DPRINT("etherndsreset\n");
	if((ctlr = malloc(sizeof(Ctlr))) == nil)
		return -1;

	ilock(ctlr);
	
	/* Initialise stats buffer and send address to ARM7 */
	memset(stats7, 0, sizeof(stats7));
	dcflush(stats7, sizeof(stats7));
	nbfifoput(F9WFstats, (ulong)stats7);
	
	/* Initialise AP list buffer and send address to ARM7 */
	memset(aplist7, 0, sizeof(aplist7));
	dcflush(aplist7, sizeof(aplist7));
	nbfifoput(F9WFapquery, (ulong)aplist7);
	
	memset(&rx_pkt, 0, sizeof(rx_pkt));
	dcflush(&rx_pkt, sizeof(rx_pkt));
	nbfifoput(F9WFrxpkt, (ulong)&rx_pkt);
	
	memset(ea, 0, sizeof(ea));
	if(memcmp(ether->ea, ea, Eaddrlen) == 0)
		panic("ethernet address not set");

	for(i = 0; i < ether->nopt; i++){
		//
		// The max. length of an 'opt' is ISAOPTLEN in dat.h.
		// It should be > 16 to give reasonable name lengths.
		//
		if(p = strchr(ether->opt[i], '='))
			*p = ' ';
		w_option(ctlr, ether->opt[i], strlen(ether->opt[i]));
	}

	// link to ether
	ether->ctlr = ctlr;
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->ifstat = ifstat;
	ether->ctl = ctl;
	ether->scanbs = scanbs;
	ether->promiscuous = promiscuous;

	ether->arg = ether;

	iunlock(ctlr);
	return 0;
}

void
etherndslink(void)
{
	addethercard("nds",  etherndsreset);
}

