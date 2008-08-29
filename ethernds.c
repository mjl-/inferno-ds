#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "arm7/wifi.h"

#include "../port/error.h"
#include "../port/netif.h"
#include "etherif.h"

#define DPRINT if(1)iprint

enum
{
	WNameLen = 34,
};

typedef struct Stats Stats;
struct Stats
{
	ulong	collisions;
	ulong	toolongs;
	ulong	tooshorts;
	ulong	aligns;
	ulong	txerrors;
};

typedef struct WFrame	WFrame;
struct WFrame
{
	ushort	sts;
	ushort	rsvd0;
	ushort	rsvd1;
	ushort	qinfo;
	ushort	rsvd2;
	ushort	rsvd3;
	ushort	txctl;
	ushort	framectl;
	ushort	id;
	uchar	addr1[Eaddrlen];
	uchar	addr2[Eaddrlen];
	uchar	addr3[Eaddrlen];
	ushort	seqctl;
	uchar	addr4[Eaddrlen];
	ushort	dlen;
	uchar	dstaddr[Eaddrlen];
	uchar	srcaddr[Eaddrlen];
	ushort	len;
	ushort	dat[3];
	ushort	type;
};

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

	int 	attached;
	int	ptype;
	int	chan;
	int	crypt;			// encryption off/on
	int	power;
	char	netname[WNameLen];
	char	wantname[WNameLen];
	char	nodename[WNameLen];

	Stats;

	/* communication interface with ARM7 */
	volatile nds_tx_packet txpkt;
	volatile nds_rx_packet rxpkt;
	volatile ulong stats7[WF_STAT_MAX];
	volatile Wifi_AccessPoint aplist7[WIFI_MAX_AP];
	volatile uchar txpktbuf[MAX_PACKET_SIZE];
};

static long
ifstat(Ether* ether, void* a, long n, ulong offset)
{
	Wifi_AccessPoint *app;
	char *p, *e, *tmp;
	Ctlr *ctlr;
	int i;

	DPRINT("ifstat\n");
	
	if (ether->ctlr == nil)
		return 0;

	if(n == 0 || offset != 0)
		return 0;

	ctlr = ether->ctlr;
	tmp = p = smalloc(READSTR);
	if(waserror()) {
		free(tmp);
		nexterror();
	}
	e = tmp+READSTR;

	dcflush(ctlr->stats7, sizeof(ctlr->stats7));
	fifoput(F9TWifi|F9WFstats, 0);
	fifoput(F9TWifi|F9WFscan, 0);
	
	p = seprint(p, e, "txpkts: %lud (%lud bytes)\n",
		(ulong) ctlr->stats7[WF_STAT_TXPKTS], (ulong) ctlr->stats7[WF_STAT_TXDATABYTES]);
	p = seprint(p, e, "rxpkts: %lud (%lud bytes)\n",
		(ulong) ctlr->stats7[WF_STAT_RXPKTS], (ulong) ctlr->stats7[WF_STAT_RXDATABYTES]);
	p = seprint(p, e, "txdropped: %lud\n", ctlr->stats7[WF_STAT_TXQREJECT]);

	// raw stats	
	p = seprint(p, e, "raw txpkts: %lud (%lud bytes)\n",
		(ulong) ctlr->stats7[WF_STAT_TXRAWPKTS], (ulong) ctlr->stats7[WF_STAT_TXBYTES]);
	p = seprint(p, e, "raw rxpkts: %lud (%lud bytes)\n",
		(ulong) ctlr->stats7[WF_STAT_RXRAWPKTS], (ulong) ctlr->stats7[WF_STAT_RXBYTES]);
	p = seprint(p, e, "rxoverruns: %lud\n",
		(ulong) ctlr->stats7[WF_STAT_RXOVERRUN]);
	
	p = seprint(p, e, "ie: 0x%ux if: 0x%ux ints: %lud\n",
		(ushort) ctlr->stats7[WF_STAT_DBG1],
		(ushort) ctlr->stats7[WF_STAT_DBG2],
		(ulong) ctlr->stats7[WF_STAT_DBG6]);

	p = seprint(p, e, "state (0x%ux): assoc %s%s%s auth %s pend %s%s%s\n",
		(ushort) ctlr->stats7[WF_STAT_DBG5],
		ctlr->stats7[WF_STAT_DBG5] & WIFI_STATE_ASSOCIATED? "ok" : "",
		ctlr->stats7[WF_STAT_DBG5] & WIFI_STATE_ASSOCIATING? "ing" : "",
		ctlr->stats7[WF_STAT_DBG5] & WIFI_STATE_CANNOTASSOCIATE? "ko" : "",

		ctlr->stats7[WF_STAT_DBG5] & WIFI_STATE_AUTHENTICATED? "ok" : "ko",

		ctlr->stats7[WF_STAT_DBG5] & WIFI_STATE_SAW_TX_ERR? "txerr": "",
		ctlr->stats7[WF_STAT_DBG5] & WIFI_STATE_CHANNEL_SCANNING? "scan": "",
		ctlr->stats7[WF_STAT_DBG5] & WIFI_STATE_TXPENDING? "tx": "",
		ctlr->stats7[WF_STAT_DBG5] & WIFI_STATE_RXPENDING? "rx": "",
		ctlr->stats7[WF_STAT_DBG5] & WIFI_STATE_APQUERYPEND? "apqry": ""
		);

	// order by signal quality 
	app = (Wifi_AccessPoint*) ctlr->aplist7;
	for(i=0; i < WIFI_MAX_AP && *(ulong*)app; app++)
		if (app->flags & WFLAG_APDATA_ACTIVE)
		p = seprint(p, e, "ssid:%s ch:%d f:%x %s%s %s\n",
			app->ssid, app->channel, app->flags,
			(app->flags & WFLAG_APDATA_WEP? "wep": ""),
			(app->flags & WFLAG_APDATA_WPA? "wpa": ""),
			(app->flags & WFLAG_APDATA_ADHOC? "hoc": "man")
			);

	n = readstr(offset, a, n, tmp);
	poperror();
	free(tmp);
	
	return n;
}

static int
w_option(Ctlr* ctlr, char* buf, long n)
{
	char *p;
	int i, r;
	Cmdbuf *cb;

	USED(ctlr);
	/* TODO: complete parsing of ctl vars */
	r = 0;

	cb = parsecmd(buf, n);
	if(cb->nf < 2)
		r = -1;
	else if(cistrcmp(cb->f[0], "essid") == 0){
		if (cistrcmp(cb->f[1],"default") == 0)
			p = "";
		else
			p = cb->f[1];
		switch(ctlr->ptype){
		case WIFI_AP_ADHOC:
		case WIFI_AP_INFRA:
			memset(ctlr->netname, 0, sizeof(ctlr->netname));
			strncpy(ctlr->netname, p, WNameLen);
			
			//dcflush(ctlr->netname, WNameLen);
			nbfifoput(F9TWifi|F9WFwssid, (ulong)ctlr->netname);
			break;
		}
	}
	else if(cistrcmp(cb->f[0], "station") == 0){
		memset(ctlr->nodename, 0, sizeof(ctlr->nodename));
		strncpy(ctlr->nodename, cb->f[1], WNameLen);
	}
	else if(cistrcmp(cb->f[0], "channel") == 0){
		if((i = atoi(cb->f[1])) >= 1 && i <= 16)
			ctlr->chan = i;
		else
			r = -1;
		
		if (!r){
			//dcflush(&ctlr->chan, sizeof(ctlr->chan));
			nbfifoput(F9TWifi|F9WFwchan, ctlr->chan);
		}
	}
	else if(cistrcmp(cb->f[0], "mode") == 0){
		if(cistrcmp(cb->f[1], "managed") == 0)
			ctlr->ptype = WIFI_AP_INFRA;
		else if(cistrcmp(cb->f[1], "adhoc") == 0)
			ctlr->ptype = WIFI_AP_ADHOC;
		else if((i = atoi(cb->f[1])) >= 0 && i <= 3)
			ctlr->ptype = i;
		else
			r = -1;

		if (!r){
			//dcflush(&ctlr->ptype, sizeof(ctlr->ptype));
			nbfifoput(F9TWifi|F9WFwap, ctlr->ptype);
		}
	}
	else if(cistrcmp(cb->f[0], "crypt") == 0){
		if(cistrcmp(cb->f[1], "off") == 0)
			ctlr->crypt = 0;
		else if(cistrcmp(cb->f[1], "on") == 0)
			ctlr->crypt = 1;
		else if((i = atoi(cb->f[1])) >= 0 && i < 3)
			ctlr->crypt = i;
		else
			r = -1;

		if (!r){
			//dcflush(&ctlr->cypt, sizeof(ctlr->crypt));
			nbfifoput(F9TWifi|F9WFwwepmode, ctlr->crypt);
		}
	}
	else if(cistrcmp(cb->f[0], "power") == 0){
		if(cistrcmp(cb->f[1], "off") == 0)
			ctlr->power = 0;
		else if(cistrcmp(cb->f[1], "on") == 0)
			ctlr->power = 1;
		else if((i = atoi(cb->f[1])) >= 0 && i < 3)
			ctlr->power = i;
		else
			r = -1;

		if (r != -1){
			nbfifoput(F9TWifi|F9WFwstate, ctlr->power);
		}
	}
/*	else if(strncmp(cb->f[0], "key", 3) == 0){
		if((i = atoi(cb->f[0]+3)) >= 1 && i <= WNKeys){
			ctlr->txkey = i-1;
			key = &ctlr->keys.keys[ctlr->txkey];
			key->len = strlen(cb->f[1]);
			if (key->len > WKeyLen)
				key->len = WKeyLen;
			memset(key->dat, 0, sizeof(key->dat));
			memmove(key->dat, cb->f[1], key->len);
		}
		else
			r = -1;
	}
	else if(cistrcmp(cb->f[0], "txkey") == 0){
		if((i = atoi(cb->f[1])) >= 1 && i <= WNKeys)
			ctlr->txkey = i-1;
		else
			r = -1;
	}
*/	else
		r = -2;
	free(cb);
	return r;
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
		ctlr->attached = 1;
	}
}

static char*
dump_pkt(uchar *data, ushort len)
{
	uchar *c;
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
	memmove((void *)ctlr->txpktbuf, b->rp, lenb);
	ctlr->txpkt.len = lenb;
	ctlr->txpkt.data = (void *)ctlr->txpktbuf;
	freeb(b);

	if(1)print("dump txpkt[%lud] @ %lux:\n%s",
		(ulong)ctlr->txpkt.len, ctlr->txpkt.data,
		dump_pkt((uchar*)ctlr->txpkt.data, ctlr->txpkt.len));

	// write data to memory before ARM7 gets hands on
	dcflush(&ctlr->txpkt, sizeof(ctlr->txpkt));
	//dcflush(txpktbuf, txpkt.len);
	fifoput(F9TWifi|F9WFtxpkt, (ulong)&ctlr->txpkt);
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

#define IEEE80211_FCTL_FROMDS	
enum {
	WF_Data		= 0x0008,
	WF_Fromds	= 0x0200,
};

static void
rxstart(Ether *ether)
{
	static uchar bcastaddr[Eaddrlen] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	Wifi_RxHeader *rx_hdr;
	WFrame *f;
	Ctlr *ctlr;
	Block* bp;
	Etherpkt* ep;

	if (ether->ctlr == nil)
		return;

	ctlr = ether->ctlr;
	/* invalidate cache before we read data written by ARM7 */
	dcflush(&ctlr->rxpkt, sizeof(ctlr->rxpkt));

	if(1)print("dump rxpkt[%lud] @ %lux:\n%s",
		(ulong)ctlr->rxpkt.len, ctlr->rxpkt.data,
		dump_pkt((uchar*)ctlr->rxpkt.data, ctlr->rxpkt.len));

	rx_hdr = (Wifi_RxHeader*)(uchar*)&ctlr->rxpkt + 2;
	f = (WFrame *)(uchar*)&ctlr->rxpkt + 2;

	if ((f->framectl & 0x01CF) == WF_Data) {
		if (memcmp(ether->ea, f->addr1, Eaddrlen) == 0
			|| memcmp(ether->ea, bcastaddr, Eaddrlen) == 0){

			/* hdrlen == 802.11 header length  bytes */
			int base2, hdrlen;
			base2 = 22;
			hdrlen = 24;
			// looks like WEP IV and IVC are removed from RX packets

			// check for LLC/SLIP header...
			if (((ushort *) rx_hdr)[base2 - 4 + 0] == 0xAAAA
			    && ((ushort *) rx_hdr)[base2 - 4 + 1] == 0x0003
			    && ((ushort *) rx_hdr)[base2 - 4 + 2] == 0) {
				// mb = sgIP_memblock_allocHW(14,len-8-hdrlen);
				// Wifi_RxRawReadPacket(base2,(len-8-hdrlen)&(~1),((u16 *)mb->datastart)+7);
				/*
				 * 14 (ether header) 
				 * + byte_length
				 *  - (ieee hdr 24 bytes) 
				 *  - 8 bytes LLC
				 */
				int len = rx_hdr->byteLength;
				bp = iallocb(ETHERHDRSIZE + len - 8 + hdrlen + 2);
				if (!bp)
					return;
					/* priv->stats.rx_dropped++; */

				ep = (Etherpkt*) bp->wp;
				memmove(ep->d, f->addr1, Eaddrlen);
				if (f->framectl & WF_Fromds)
					memmove(ep->s, f->addr3, Eaddrlen);
				else
					memmove(ep->s, f->addr2, Eaddrlen);
		
				memmove(ep->type,&f->type,2);
				bp->wp = bp->rp+(ETHERHDRSIZE+f->dlen);

				etheriq(ether, bp, 1);
			}
		}

	}
	
	fifoput(F9TWifi|F9WFrxdone, 0);
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
	Ether *ether;
	Ctlr *ctlr;
	ulong type;
	
	DPRINT("interrupt\n");
	ether = arg;
	ctlr = ether->ctlr;
	if (ctlr == nil)
		return;
	
	/* IPCSYNC irq:
	 * used by arm7 to notify the arm9 
	 * that there's wifi activity (tx/rx)
	 */ 
	type = IPCREG->ctl & Ipcdatain;
	ilock(ctlr);
	if (type == I7WFrxdone)
		rxstart(ether);
	else if (type == I7WFtxdone)
		;
	intrclear(ether->irq, 0);
	iunlock(ctlr);
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
	memset(ctlr->stats7, 0, sizeof(ctlr->stats7));
	dcflush(ctlr->stats7, sizeof(ctlr->stats7));
	nbfifoput(F9TWifi|F9WFwstats, (ulong)ctlr->stats7);
	
	/* Initialise AP list buffer and send address to ARM7 */
	memset(ctlr->aplist7, 0, sizeof(ctlr->aplist7));
	dcflush(ctlr->aplist7, sizeof(ctlr->aplist7));
	nbfifoput(F9TWifi|F9WFapquery, (ulong)ctlr->aplist7);
	
	memset(&ctlr->rxpkt, 0, sizeof(ctlr->rxpkt));
	dcflush(&ctlr->rxpkt, sizeof(ctlr->rxpkt));
	nbfifoput(F9TWifi|F9WFrxpkt, (ulong)&ctlr->rxpkt);
	
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

