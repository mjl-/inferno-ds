/* TODO: document those according to gbatek */

// Card bus
#define CARD_CR1	(*(volatile ushort*)0x040001A0)
#define CARD_CR1H	(*(volatile uchar*)0x040001A1)
#define CARD_EEPDATA	(*(volatile uchar*)0x040001A2)
#define CARD_CR2	(*(volatile ulong*)0x040001A4)
#define CARD_COMMAND	((volatile uchar*)0x040001A8)

#define CARD_DATA_RD	(*(volatile ulong*)0x04100010)

#define CARD_1B0	(*(volatile ulong*)0x040001B0)
#define CARD_1B4	(*(volatile ulong*)0x040001B4)
#define CARD_1B8	(*(volatile ushort*)0x040001B8)
#define CARD_1BA	(*(volatile ushort*)0x040001BA)

/*
 * CARD_CR2 register
 */

#define CARD_ACTIVATE	(1<<31)	// when writing, get the ball rolling
// 1<<30
#define CARD_nRESET	(1<<29)	// value on the /reset pin (1 = high out, not a reset state, 0 = low out = in reset)
#define CARD_28		(1<<28)	// when writing
#define CARD_27		(1<<27)	// when writing
#define CARD_26		(1<<26)
#define CARD_22		(1<<22)
#define CARD_19		(1<<19)
#define CARD_ENCRYPTED	(1<<14)	// when writing, this cmd should be encrypted
#define CARD_13		(1<<13)	// when writing
#define CARD_4		(1<<4)	// when writing
#define CARD_BUSY	(1<<31)	// when reading, still expecting incomming data?
#define CARD_DATA_READY (1<<23)	// when reading, CARD_DATA_RD or CARD_DATA has another word of data and is good to go

void cardWriteCommand(const uchar * cmd);

void cardPolledTransfer(ulong flags, ulong * dst, ulong len, const uchar * cmd);
void cardStartTransfer(const uchar * cmd, ulong * dst, int ch, ulong flags);
ulong cardWriteAndRead(const uchar * cmd, ulong flags);

void cardRead00(ulong address, ulong * dst, ulong len, ulong flags);
void cardReadHeader(uchar * header);
int cardReadID(ulong flags);
void cardReadEeprom(ulong address, uchar *data, ulong len, ulong addrtype);
void cardWriteEeprom(ulong address, uchar *data, ulong len, ulong addrtype);

uchar cardEepromReadID(uchar i); 
uchar cardEepromCommand(uchar cmd, ulong address); 

/*
 * -1:no card or no EEPROM
 *  0:unknown                 PassMe?
 *  1:TYPE 1  4Kbit(512Byte)  EEPROM
 *  2:TYPE 2 64Kbit(8KByte)or 512kbit(64Kbyte)   EEPROM
 *  3:TYPE 3  2Mbit(256KByte) FLASH MEMORY (some rare 4Mbit and 8Mbit chips also)
 */

int cardEepromGetType(void);
ulong cardEepromGetSize(void);
void cardEepromChipErase(void);
void cardEepromSectorErase(ulong address);

