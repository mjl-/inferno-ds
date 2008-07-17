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

	TIMERWIFIbit = TIMER0bit+WIFItimer,
	TIMERAUDIObit = TIMER0bit+AUDIOtimer,
};

typedef struct TransferSoundData TransferSoundData;
struct TransferSoundData {
  const void *data;
  ulong len;
  ulong rate;
  uchar vol;
  uchar pan;
  uchar fmt;
  uchar PADDING;
};

typedef struct TransferSound TransferSound;
struct TransferSound {
  TransferSoundData d[16];
  uchar count;
  uchar PADDING[3];
};

#define IPCMEM 0x027FF000
#define IPC ((volatile TransferRegion*)IPCMEM)
typedef struct TransferRegion TransferRegion;
struct TransferRegion {
	ushort touchX,   touchY;	 // TSC X, Y
	ushort touchXpx, touchYpx; //	 TSC X, Y pixel values
	short touchZ1,  touchZ2; //	 TSC x-panel measurements
	int td1, td2;	 	//	 TSC temperature diodes
	ushort temp;		//	 TSC computed temperature

	ushort buttons;			//	 X, Y, /PENIRQ buttons

	ushort hbt;

	union {
		uchar curtime[8];	//	 current time response from RTC

		struct {
			uchar cmd;
			uchar year;	//	add 2000 to get 4 digit year
			uchar month;	//	1 to 12
			uchar day;	//	1 to (days in month)

			uchar weekday;	// day of week
			uchar hours;	//	0 to 11 for AM, 52 to 63 for PM
			uchar minutes; //	0 to 59
			uchar seconds; //	0 to 59
		} rtc;
	} time;
	int	unixTime;

	ushort batt;		//	 battery life ??  hopefully.  :)
	ushort aux;		//		 i have no idea...

/*	 Don't rely on these below, will change or be removed in the future */
	TransferSound *soundData; 

	ulong mailAddr;
	ulong mailData;
	uchar mailRead;
	uchar mailBusy;
	ulong mailSize;
};

