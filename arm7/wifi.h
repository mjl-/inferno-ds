// typedefs just to not rewrite wifi.c 
typedef uchar	u8;
typedef ushort	u16;
typedef ulong	u32;
typedef volatile unsigned short vu16;
typedef int	s32;

#define REG_IME	INTREG->ime

/* Wifi controller */
enum
{
	WR_MODE_RST	= 0x04,
	WR_MODE_WEP	= 0x06,
	WR_IF			= 0x10,
	WR_IE			= 0x12,
	
	WR_MACADDR	= 0x18,
	WR_BSSID		= 0x20,
	WR_AIDS		= 0x28,	
	WR_RETRLIMIT	= 0x2C,
	WR_POWERSTATE	= 0x3C,
	WR_RANDOM	= 0x44,

	WR_TXREQ_RESET	= 0xAC,
	WR_TXREQ_SET	= 0xAE,
	WR_TXREQ_READ	= 0xB0,
	WR_TXBUSY	= 0xB6,
	WR_TXSTAT	= 0xB8,
	WR_TXSTATCNT	= 0x08,
	WR_TX_SEQNO	= 0x210,

	WR_TXBUF_WR_ADDR	= 0x68,
	WR_TXBUF_WR_DATA	= 0x70,
	WR_TXBUF_GAP	= 0x74,
	WR_TXBUF_GAPDISP	= 0x76,
	
	WR_TXBUF_BEACON	= 0x80, 
	WR_TXBUF_EXTRA	= 0x90,
	WR_TXBUF_LOC1	= 0xA0,
	WR_TXBUF_LOC2	= 0xA4,
	WR_TXBUF_LOC3	= 0xA8,

	WR_TXBUF_RESET	= 0xB4,
	WR_TXBUF_TIM		= 0x84,
	WR_TXBUF_COUNT	= 0x6C,
	WR_TX_RETRYLIMIT	= 0x2C,
	WR_TX_ERR_COUNT	= 0x1C0,

	WR_WEPKEY0	= 0x5F80,
	WR_WEPKEY1	= 0x5FA0,
	WR_WEPKEY2	= 0x5FC0,
	WR_WEPKEY3	= 0x5FE0,
};

/* BB & RF controllers */
enum
{
	BBSIOCNT		= 0x158,
	BBSIOWRITE		= 0x15A,
	BBSIOREAD		= 0x15C,
	BBSIOBUSY		= 0x15E,
	RFSIODATA2		= 0x17C,
	RFSIODATA1		= 0x17E,
	RFSIOBUSY		= 0x180,
};

// keepalive updated in the update handler, which should be called in vblank
// keepalive set for 2 minutes.
#define WIFI_KEEPALIVE_COUNT		(60*60*2)

#define WIFI_REG(ofs)	(*(vu16*)(0x04800000+(ofs)))
#define WIFI_ADDR(ofs)	((vu16*)(0x04800000+(ofs)))

#define W_WEPKEY0		WIFI_ADDR(WR_WEPKEY0)
#define W_WEPKEY1		WIFI_ADDR(WR_WEPKEY1)
#define W_WEPKEY2		WIFI_ADDR(WR_WEPKEY2)
#define W_WEPKEY3		WIFI_ADDR(WR_WEPKEY3)

#define W_MODE_RST		WIFI_REG(WR_MODE_RST)
#define W_MODE_WEP		WIFI_REG(WR_MODE_WEP)
#define W_IF			WIFI_REG(WR_IF)
#define W_IE			WIFI_REG(WR_IE)
#define W_MACADDR		WIFI_ADDR(WR_MACADDR)
#define W_BSSID		WIFI_ADDR(WR_BSSID)
#define W_AIDS		WIFI_REG(WR_AIDS)
#define W_RETRLIMIT		WIFI_REG(WR_RETRLIMIT)
#define W_POWERSTATE		WIFI_REG(WR_POWERSTATE)
#define W_RANDOM		WIFI_REG(WR_RANDOM)

#define W_BBSIOCNT		WIFI_REG(BBSIOCNT)
#define W_BBSIOWRITE		WIFI_REG(BBSIOWRITE)
#define W_BBSIOREAD		WIFI_REG(BBSIOREAD)
#define W_BBSIOBUSY		WIFI_REG(BBSIOBUSY)
#define W_RFSIODATA2		WIFI_REG(RFSIODATA2)
#define W_RFSIODATA1		WIFI_REG(RFSIODATA1)
#define W_RFSIOBUSY		WIFI_REG(RFSIOBUSY)

/* Wifi_Data defines */

#define WIFI_RXBUFFER_SIZE	(1024*12)
#define WIFI_TXBUFFER_SIZE	(1024*24)
#define WIFI_MAX_AP			32
#define WIFI_MAX_ASSOC_RETRY	30
#define WIFI_PS_POLL_CONST  2

#define WIFI_AP_TIMEOUT    40

#define WFLAG_PACKET_DATA		0x0001
#define WFLAG_PACKET_MGT		0x0002
#define WFLAG_PACKET_BEACON		0x0004
#define WFLAG_PACKET_CTRL		0x0008

#define WFLAG_PACKET_ALL		0xFFFF

#define WFLAG_ARM7_ACTIVE		0x0001
#define WFLAG_ARM7_RUNNING		0x0002

#define WFLAG_ARM9_ACTIVE		0x0001
#define WFLAG_ARM9_USELED		0x0002
#define WFLAG_ARM9_ARM7READY	0x0004
#define WFLAG_ARM9_NETUP		0x0008
#define WFLAG_ARM9_NETREADY		0x0010

#define WIFIINIT_OPTION_USELED		   0x0002
#define WFLAG_ARM9_INITFLAGMASK	0x0002

#define WFLAG_IP_GOTDHCP		0x0001

// request - request flags
#define WFLAG_REQ_APCONNECT			0x0001
#define WFLAG_REQ_APCOPYVALUES		0x0002
#define WFLAG_REQ_APADHOC			0x0008
#define WFLAG_REQ_PROMISC			0x0010
#define WFLAG_REQ_USEWEP			0x0020

// request - informational flags
#define WFLAG_REQ_APCONNECTED		0x8000

#define WFLAG_APDATA_ADHOC			0x0001
#define WFLAG_APDATA_WEP			0x0002
#define WFLAG_APDATA_WPA			0x0004
#define WFLAG_APDATA_COMPATIBLE		0x0008
#define WFLAG_APDATA_EXTCOMPATIBLE	0x0010
#define WFLAG_APDATA_SHORTPREAMBLE	0x0020
#define WFLAG_APDATA_ACTIVE			0x8000

enum WIFI_RETURN {
	WIFI_RETURN_OK =				0, // Everything went ok
	WIFI_RETURN_LOCKFAILED  =		1, // the spinlock attempt failed (it wasn't retried cause that could lock both cpus- retry again after a delay.
	WIFI_RETURN_ERROR =				2, // There was an error in attempting to complete the requested task.
	WIFI_RETURN_PARAMERROR =		3, // There was an error in the parameters passed to the function.
};

enum WIFI_STATS
{
	// software stats
	WSTAT_RXQUEUEDPACKETS, // number of packets queued into the rx fifo
	WSTAT_TXQUEUEDPACKETS, // number of packets queued into the tx fifo
	WSTAT_RXQUEUEDBYTES, // number of bytes queued into the rx fifo
	WSTAT_TXQUEUEDBYTES, // number of bytes queued into the tx fifo
	WSTAT_RXQUEUEDLOST, // number of packets lost due to space limitations in queuing
	WSTAT_TXQUEUEDREJECTED, // number of packets rejected due to space limitations in queuing
	WSTAT_RXPACKETS,
	WSTAT_RXBYTES,
	WSTAT_RXDATABYTES,
	WSTAT_TXPACKETS,
	WSTAT_TXBYTES,
	WSTAT_TXDATABYTES,
	WSTAT_ARM7_UPDATES,
	WSTAT_DEBUG,
	
	// hardware stats (function mostly unknown.)
	WSTAT_HW_1B0,WSTAT_HW_1B1,WSTAT_HW_1B2,WSTAT_HW_1B3,WSTAT_HW_1B4,WSTAT_HW_1B5,WSTAT_HW_1B6,WSTAT_HW_1B7,	
	WSTAT_HW_1B8,WSTAT_HW_1B9,WSTAT_HW_1BA,WSTAT_HW_1BB,WSTAT_HW_1BC,WSTAT_HW_1BD,WSTAT_HW_1BE,WSTAT_HW_1BF,	
	WSTAT_HW_1C0,WSTAT_HW_1C1,WSTAT_HW_1C4,WSTAT_HW_1C5,
	WSTAT_HW_1D0,WSTAT_HW_1D1,WSTAT_HW_1D2,WSTAT_HW_1D3,WSTAT_HW_1D4,WSTAT_HW_1D5,WSTAT_HW_1D6,WSTAT_HW_1D7,	
	WSTAT_HW_1D8,WSTAT_HW_1D9,WSTAT_HW_1DA,WSTAT_HW_1DB,WSTAT_HW_1DC,WSTAT_HW_1DD,WSTAT_HW_1DE,WSTAT_HW_1DF,	

	NUM_WIFI_STATS
};

enum WIFI_MODE {
	WIFIMODE_DISABLED,
	WIFIMODE_NORMAL,
	WIFIMODE_SCAN,
	WIFIMODE_ASSOCIATE,
	WIFIMODE_ASSOCIATED,
	WIFIMODE_DISASSOCIATE,
	WIFIMODE_CANNOTASSOCIATE,
};
enum WIFI_AUTHLEVEL {
	WIFI_AUTHLEVEL_DISCONNECTED,
	WIFI_AUTHLEVEL_AUTHENTICATED,
	WIFI_AUTHLEVEL_ASSOCIATED,
	WIFI_AUTHLEVEL_DEASSOCIATED,
};

enum WEPMODES {
	WEPMODE_NONE = 0,
	WEPMODE_40BIT = 1,
	WEPMODE_128BIT = 2
};

// WIFI_ASSOCSTATUS - returned by Wifi_AssocStatus() after calling Wifi_ConnectAP
enum WIFI_ASSOCSTATUS {
	ASSOCSTATUS_DISCONNECTED, // not *trying* to connect
	ASSOCSTATUS_SEARCHING, // data given does not completely specify an AP, looking for AP that matches the data.
	ASSOCSTATUS_AUTHENTICATING, // connecting...
	ASSOCSTATUS_ASSOCIATING, // connecting...
	ASSOCSTATUS_ACQUIRINGDHCP, // connected to AP, but getting IP data from DHCP
	ASSOCSTATUS_ASSOCIATED,	// Connected! (COMPLETE if Wifi_ConnectAP was called to start)
	ASSOCSTATUS_CANNOTCONNECT, // error in connecting... (COMPLETE if Wifi_ConnectAP was called to start)
};

typedef struct WIFI_TXHEADER  Wifi_TxHeader;
struct WIFI_TXHEADER {
	ushort enable_flags;
	ushort unknown;
	ushort countup;
	ushort beaconfreq;
	ushort tx_rate;
	ushort tx_length;
};

typedef struct WIFI_RXHEADER Wifi_RxHeader;
struct WIFI_RXHEADER {
	ushort a;
	ushort b;
	ushort c;
	ushort d;
	ushort byteLength;
	ushort rssi_;
};


/* Ethernet MTU is 1500 */
#define MAX_PACKET_SIZE 1600

#define WIFI_ARM7_TIMEOUT	100

typedef struct WIFI_ACCESSPOINT Wifi_AccessPoint;
struct WIFI_ACCESSPOINT {
	char ssid[33]; // 0-32byte data, zero
	char ssid_len;
	u8 bssid[6];
	u8 macaddr[6];
	u16 maxrate; // max rate is measured in steps of 1/2Mbit - 5.5Mbit will be represented as 11, or 0x0B
	u32 timectr;
	u16 rssi;
	u16 flags;
//	u32 spinlock;
	u8 channel;
	u8 rssi_past[8];
	u8 base_rates[16]; // terminated by a 0 entry
};

typedef struct Wifi_Data Wifi_Data;
struct Wifi_Data {
	u16 curChannel, reqChannel;
	u16 curMode, reqMode;
	u16 authlevel,authctr;
	u32 flags9, flags7, reqPacketFlags;
	u16 curReqFlags, reqReqFlags;
	u32 counter7,bootcounter7;
	uchar MacAddr[6];
	u16 authtype;
	u16 iptype,ipflags;
	u32 ip,snmask,gateway;
	
	u16 state;
	
	// current AP data
	char ssid7[34],ssid9[34];
	u8 bssid7[6], bssid9[6];
	u8 apmac7[6], apmac9[6];
	char wepmode7, wepmode9;
	char wepkeyid7, wepkeyid9;
	u8 wepkey7[20],wepkey9[20];
	u8 baserates7[16], baserates9[16];
	u8 apchannel7, apchannel9;
	u8 maxrate7;
	u16 ap_rssi;
	u16 pspoll_period;

	// AP data
	Wifi_AccessPoint aplist[WIFI_MAX_AP];

	// WFC data
	u8 wfc_enable[4]; // wep mode, or 0x80 for "enabled"
	Wifi_AccessPoint wfc_ap[3];
	unsigned long wfc_config[3][5]; // ip, snmask, gateway, primarydns, 2nddns
	u8 wfc_wepkey[3][16];
	
	// wifi data
	u32 rxbufIn, rxbufOut; // bufIn/bufOut have 2-byte granularity.
	u16 rxbufData[WIFI_RXBUFFER_SIZE/2]; // send raw 802.11 data through! rxbuffer is for rx'd data, arm7->arm9 transfer

	u32 txbufIn, txbufOut;
	u16 txbufData[WIFI_TXBUFFER_SIZE/2]; // tx buffer is for data to tx, arm9->arm7 transfer

	// stats data
	u32 stats[NUM_WIFI_STATS];
   
	u16 debug[30];
	
	u32 random; // semirandom number updated at the convenience of the arm7. use for initial seeds & such.
};

extern Wifi_Data *WifiData;

void Wifi_Init(u32);
void Wifi_Deinit(void);
void Wifi_Update(void);
