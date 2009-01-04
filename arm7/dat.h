enum {
	FWconsoletype =	0x1d,	/* 1 byte */
	FWds =	0xff,
	FWdslite =	0x20,
	FWique =	0x43,
};

enum {
	WIFItimer = 0,
	AUDIOtimer = 1,

	TIMERWIFIbit = TIMER0bit+WIFItimer,
	TIMERAUDIObit = TIMER0bit+AUDIOtimer,
};

#define IPCMEM 0x027FF000
#define IPC ((volatile TxRegion*)IPCMEM)
typedef struct TxRegion TxRegion;
struct TxRegion {
	int td1, td2;	 // TSC temperature diodes
	int temp;	// TSC computed temperature

	ushort hbt;
	int	time;
	ushort batt;	// battery life 
	ushort aux;	// mic/aux input
};

