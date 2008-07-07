enum {
	FWconsoletype =	0x1d,	/* 1 byte */
	FWds =	0xff,
	FWdslite =	0x20,
	FWique =	0x43,
};

enum {
	Ds =		FWds,
	Dslite =	FWdslite,
	Dsique =	FWique,
};

enum {
	WIFItimer = 0,
	AUDIOtimer = 1,
};

/* Pointer to buffer used to send incoming packets to ARM9 */
struct nds_rx_packet *rx_packet;
/* We can only send one packet to ARM9 at a time */
int sending_packet;
