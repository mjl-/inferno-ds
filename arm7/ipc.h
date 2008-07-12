
typedef struct TransferSoundData TransferSoundData;
struct TransferSoundData {
  const void *data;
  u32 len;
  u32 rate;
  u8 vol;
  u8 pan;
  u8 fmt;
  u8 PADDING;
};


typedef struct TransferSound TransferSound;
struct TransferSound {
  TransferSoundData d[16];
  u8 count;
  u8 PADDING[3];
};


#define IPCMEM 0x027FF000
#define IPC ((TransferRegion*)IPCMEM)
typedef struct TransferRegion TransferRegion;
struct TransferRegion {
	vint16 touchX,   touchY;	 // TSC X, Y
	vint16 touchXpx, touchYpx; //	 TSC X, Y pixel values
	vint16 touchZ1,  touchZ2; //	 TSC x-panel measurements
	vuint16 tdiode1,  tdiode2; //	 TSC temperature diodes
	vuint32 temperature;	//	 TSC computed temperature

	uint16 buttons;			//	 X, Y, /PENIRQ buttons

	vuint16 heartbeat;

	union {
		vuint8 curtime[8];	//	 current time response from RTC

		struct {
			vu8 cmd;
			vu8 year;	//	add 2000 to get 4 digit year
			vu8 month;	//	1 to 12
			vu8 day;	//	1 to (days in month)

			vu8 weekday;	// day of week
			vu8 hours;	//	0 to 11 for AM, 52 to 63 for PM
			vu8 minutes; //	0 to 59
			vu8 seconds; //	0 to 59
		} rtc;
	} time;
	vint32	unixTime;

	uint16 battery;		//	 battery life ??  hopefully.  :)
	uint16 aux;		//		 i have no idea...

/*	 Don't rely on these below, will change or be removed in the future */
	TransferSound *soundData; 

	vuint32 mailAddr;
	vuint32 mailData;
	vuint8 mailRead;
	vuint8 mailBusy;
	vuint32 mailSize;
};

