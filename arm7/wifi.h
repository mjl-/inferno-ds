/*
 *   Driver for DS Linux
 *
 *       Copyright 2006 Bret Thaeler - bthaeler@aol.com
 */

// Wifi regs
#define WIFI_REG(ofs)   (*(volatile ushort*)(0x04800000+(ofs)))
#define WIFI_WEPKEY0    ( (volatile ushort*)0x04805F80)
#define WIFI_WEPKEY1    ( (volatile ushort*)0x04805FA0)
#define WIFI_WEPKEY2    ( (volatile ushort*)0x04805FC0)
#define WIFI_WEPKEY3    ( (volatile ushort*)0x04805FE0)

#define WIFI_MODE_RST   (*(volatile ushort*)0x04800004)
#define WIFI_MODE_WEP   (*(volatile ushort*)0x04800006)
#define WIFI_IF         (*(volatile ushort*)0x04800010)
#define WIFI_IE         (*(volatile ushort*)0x04800012)
#define WIFI_MACADDR    ( (volatile ushort*)0x04800018)
#define WIFI_BSSID      ( (volatile ushort*)0x04800020)
#define WIFI_AIDS       (*(volatile ushort*)0x04800028)
#define WIFI_RETRLIMIT  (*(volatile ushort*)0x0480002C)
#define WIFI_POWERSTATE (*(volatile ushort*)0x0480003C)
#define WIFI_RANDOM     (*(volatile ushort*)0x04800044)

#define WIFI_BBSIOCNT   (*(volatile ushort*)0x04800158)
#define WIFI_BBSIOWRITE (*(volatile ushort*)0x0480015A)
#define WIFI_BBSIOREAD  (*(volatile ushort*)0x0480015C)
#define WIFI_BBSIOBUSY  (*(volatile ushort*)0x0480015E)
#define WIFI_RFSIODATA2 (*(volatile ushort*)0x0480017C)
#define WIFI_RFSIODATA1 (*(volatile ushort*)0x0480017E)
#define WIFI_RFSIOBUSY  (*(volatile ushort*)0x04800180)

enum WEPMODES {
	WEPMODE_NONE = 0,
	WEPMODE_40BIT = 1,
	WEPMODE_128BIT = 2
};

enum WIFI_STATE {
	WIFI_STATE_UP = 0x0001,
	WIFI_STATE_ASSOCIATED = 0x0002,
	WIFI_STATE_ASSOCIATING = 0x0004,
	WIFI_STATE_CANNOTASSOCIATE = 0x0008,
	WIFI_STATE_AUTHENTICATED = 0x0010,
	WIFI_STATE_SAW_TX_ERR = 0x0020,
	WIFI_STATE_CHANNEL_SCANNING = 0x0040,

	WIFI_STATE_TXPENDING = 0x0100,
	WIFI_STATE_RXPENDING = 0x0200,
	WIFI_STATE_APQUERYPEND = 0x0400,
};

/*
 * Memory layout
 *
 * 0x4000 - 0x5FFF (8192 bytes)
 *
 * Recieve buffer 0x4C28 -- 0x5F5F  (4920 bytes)
 *
 * Our MTU 1556 bytes
 *    Tx header:      12 bytes
 *    802.11 headers: 24 bytes
 *    Wep keys info:   4 bytes
 *    LLC header:      8 bytes
 *    1500 data:    1500 bytes
 *    WEP IVC:         4 bytes
 *    CRC:             4 bytes
 * 
 * Transmit buffers 0x4000 - 0x4C27 (3112 bytes)
 * Each transmit buffer, 1556
 *
 * Transmit buffer1 0x4000 - 0x4613
 * Transmit buffer2 0x4614 - 0x4C27
 */
#define RX_START_SETUP 0x4C28
#define RX_END_SETUP   0x5F60
#define RX_CSR_SETUP   ((RX_START_SETUP - 0x4000) >> 1)
#define TX_MTU_BYTES   1556
#define TX1_MAC_START  0x4000
#define TX1_SETUP_VAL  (0x8000 | ((TX1_MAC_START - 0x4000) >> 1))
#define TX2_MAC_START  0x4614
#define TX2_SETUP_VAL  (0x8000 | ((TX2_MAC_START - 0x4000) >> 1))

#define MAX_KEY_SIZE 13		// 128 (?) bits


#define WFLAG_PACKET_DATA		0x0001
#define WFLAG_PACKET_MGT		0x0002
#define WFLAG_PACKET_BEACON		0x0004
#define WFLAG_PACKET_CTRL		0x0008

#define WFLAG_PACKET_ALL		0xFFFF

#define WFLAG_APDATA_ADHOC			0x0001
#define WFLAG_APDATA_WEP			0x0002
#define WFLAG_APDATA_WPA			0x0004
#define WFLAG_APDATA_COMPATIBLE		0x0008
#define WFLAG_APDATA_EXTCOMPATIBLE	0x0010
#define WFLAG_APDATA_SHORTPREAMBLE	0x0020
#define WFLAG_APDATA_ACTIVE			0x8000


#define WIFI_CHANNEL_SCAN_DWEL 150 // 150 micro seconds

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

typedef struct WIFI_ACCESSPOINT Wifi_AccessPoint;
struct WIFI_ACCESSPOINT {
	char ssid[33];		// 0-32byte data, zero
	char ssid_len;
	uchar bssid[6];
	uchar macaddr[6];
	ushort maxrate;		// max rate is measured in steps of 1/2Mbit - 5.5Mbit will be represented as 11, or 0x0B
	ulong timectr;
	ushort rssi;
	ushort flags;
/*
	ulong spinlock;
*/
	uchar channel;
	uchar rssi_past[8];
	uchar base_rates[16];	// terminated by a 0 entry
};

typedef struct Wifi_Data_Struct Wifi_Data;
struct Wifi_Data_Struct {
	ushort curChannel, reqChannel;
	ushort curMode, reqMode;
	ushort authlevel, authctr;
	uchar curWepmode, reqWepMode;
	char ssid[34];
	uchar baserates[16];

	ushort scanChannel;

	ushort state;

	uchar MacAddr[6];
	uchar bssid[6];
	uchar apmac[6];
	uchar wepkey[4][MAX_KEY_SIZE + 1];
	ushort maxrate;
	ushort wepkeyid;

	uchar FlashData[512];

	/* pointers to buffers we get handed from ARM9 */
	volatile ulong *stats;
	Wifi_AccessPoint *aplist;
};

enum WIFI_STATS
{
	WF_STAT_RXPKTS = 0,
	WF_STAT_RXBYTES,
	WF_STAT_RXDATABYTES,

	WF_STAT_TXPKTS,
	WF_STAT_TXBYTES,
	WF_STAT_TXDATABYTES,
	WF_STAT_TXQREJECT,

	WF_STAT_TXRAWPKTS,
	WF_STAT_RXRAWPKTS,
	WF_STAT_RXOVERRUN,

	WF_STAT_DBG1,
	WF_STAT_DBG2,
	WF_STAT_DBG3,
	WF_STAT_DBG4,
	WF_STAT_DBG5,
	WF_STAT_DBG6,

	WF_STAT_HW_1B0,
	WF_STAT_HW_1B4,
	WF_STAT_HW_1B8,
	WF_STAT_HW_1BC,
	WF_STAT_HW_1C0,
	WF_STAT_HW_1D0,
	WF_STAT_HW_1D4,
	WF_STAT_HW_1D8,
	WF_STAT_HW_1DC,

	WF_STAT_MAX
};

enum WIFI_AP_MODE
{
	WIFI_AP_INFRA,
	WIFI_AP_ADHOC,
};

/* Ethernet MTU is 1500 */
#define MAX_PACKET_SIZE 1600

#define WIFI_ARM7_TIMEOUT	100
#define WIFI_MAX_AP		32

/* Used for transmission to arm7 */
typedef struct nds_tx_packet nds_tx_packet;
struct nds_tx_packet {
	ushort len;	
	uchar *data;
	void *skb;
};

typedef struct nds_rx_packet nds_rx_packet;
struct nds_rx_packet {
	ushort len;	
	uchar data[MAX_PACKET_SIZE];
};

extern Wifi_Data wifi_data;
extern nds_rx_packet *rx_packet;
extern int sending_packet;

void wifi_init(void);
void wifi_open(void);
void wifi_close(void);
void wifi_mac_query(void);
void wifi_interrupt(void*);
void wifi_send_ether_packet(ushort length, uchar * data);
void wifi_stats_query(void);

void Wifi_RequestChannel(int channel);
void Wifi_SetWepKey(int key, int off, uchar b1, uchar b2);
void Wifi_SetWepKeyID(int key);
void Wifi_SetWepMode(int wepmode);
void Wifi_SetSSID(int off, char b1, char b2);
void Wifi_SetAPMode(enum WIFI_AP_MODE mode);
void Wifi_GetAPMode(void);

void wifi_ap_query(ushort start_stop);
void wifi_start_scan(void);
void wifi_timer_handler(void*);
void wifi_rx_q_complete(void);
