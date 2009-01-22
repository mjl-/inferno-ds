#include	"u.h"
#include	"io.h"
#include	"mem.h"
#include	"arm7/card.h"

/*
 * storage support for the Datel Game'n'Music card
 * Copyright (c) 2007 Michael "Chishm" Chisholm
 */

#define BYTES_PER_SECTOR 512

#define SD_COMMAND_TIMEOUT 0xFFF
#define SD_WRITE_TIMEOUT 0xFFFF
#define CARD_CR2_SETTINGS 0xA0586000

#define READ_SINGLE_BLOCK 17
#define WRITE_SINGLE_BLOCK 24
#define SD_WRITE_OK 0x05

static void
openSpi(void)
{
	volatile ulong temp;
	
	CARD_CR1 = Spiena|Spiirqena;
	CARD_COMMAND[0] = 0xF2;
	CARD_COMMAND[1] = 0x00;
	CARD_COMMAND[2] = 0x00;
	CARD_COMMAND[3] = 0x00;
	CARD_COMMAND[4] = 0x00;
	CARD_COMMAND[5] = 0xCC;			// 0xCC == enable microSD ?
	CARD_COMMAND[6] = 0x00;
	CARD_COMMAND[7] = 0x00;
	CARD_CR2 = CARD_CR2_SETTINGS;

	while (CARD_CR2 & CARD_BUSY) {
		temp = CARD_DATA_RD;
	}

    	CARD_CR1 = Spiena|Spiselect|Spihold;
}

void 
closeSpi(void)
{
	volatile ulong temp;

	CARD_CR1 = Spiena|Spiirqena;
	CARD_COMMAND[0] = 0xF2;
	CARD_COMMAND[1] = 0x00;
	CARD_COMMAND[2] = 0x00;
	CARD_COMMAND[3] = 0x00;
	CARD_COMMAND[4] = 0x00;
	CARD_COMMAND[5] = 0xC8;			// 0xCC == disable microSD ?
	CARD_COMMAND[6] = 0x00;
	CARD_COMMAND[7] = 0x00;
	CARD_CR2 = CARD_CR2_SETTINGS;

	while (CARD_CR2 & Spibusy) {
		temp = CARD_DATA_RD;
	}

    	CARD_CR1 = Spiena|Spiselect|Spihold;
	CARD_EEPDATA = 0xFF;
	
	while (CARD_CR1 & Spibusy);
}
	
static uchar 
transferSpiByte(uchar send)
{
	CARD_EEPDATA = send;
	while (CARD_CR1 & Spibusy);
	return CARD_EEPDATA;
}

static uchar
getSpiByte(void)
{
	CARD_EEPDATA = 0xFF;
	while (CARD_CR1 & Spibusy);
	return CARD_EEPDATA;
}

static uchar
sendCommand(uchar command, ulong argument)
{
	uchar commandData[6];
	int timeout;
	uchar spiByte;
	int i;

	commandData[0] = command | 0x40; 
	commandData[1] = (uchar)(argument >> 24);
	commandData[2] = (uchar)(argument >> 16);
	commandData[3] = (uchar)(argument >>  8);
	commandData[4] = (uchar)(argument >>  0);
	commandData[5] = 0x95;	// Fake CRC, 0x95 is only important for SD CMD 0
	
	// Send read sector command
	for (i = 0; i < 6; i++) {
		CARD_EEPDATA = commandData[i];
		while (CARD_CR1 & Spibusy);
	}

	// Wait for a response
	timeout = SD_COMMAND_TIMEOUT;
	do {
		spiByte = getSpiByte();
	} while (spiByte == 0xFF && --timeout > 0);
	
	return spiByte;
}

static int
sdRead(ulong sector, uchar* dest)
{
	uchar spiByte;
	int timeout;
	int i;
	volatile ulong temp;
	
	openSpi ();
	
	if (sendCommand (READ_SINGLE_BLOCK, sector * BYTES_PER_SECTOR) != 0x00) {
		closeSpi ();
		return 0;
	}

	// Wait for data start token
	timeout = SD_COMMAND_TIMEOUT;
	do {
		spiByte = getSpiByte ();
	} while (spiByte == 0xFF && --timeout > 0);

	if (spiByte != 0xFE) {
		closeSpi ();
		return 0;
	}
	
	for (i = BYTES_PER_SECTOR; i > 0; i--) {
		*dest++ = getSpiByte();
	}

	// Clean up CRC
	temp = getSpiByte();
	temp = getSpiByte();
	
	closeSpi ();
	return 1;
}

static int
sdWrite(ulong sector, uchar* src)
{
	int i;
	int timeout;
	
	openSpi ();
	
	if (sendCommand (WRITE_SINGLE_BLOCK, sector * BYTES_PER_SECTOR) != 0) {
		closeSpi ();
		return 0;
	}
	
	// Send start token
	transferSpiByte (0xFE);
	
	// Send data
	for (i = BYTES_PER_SECTOR; i > 0; i--) {
		CARD_EEPDATA = *src++;
		while (CARD_CR1 & Spibusy);
	}

	// Send fake CRC
	transferSpiByte (0xFF);
	transferSpiByte (0xFF);
	
	// Get data response
	if ((getSpiByte() & 0x0F) != SD_WRITE_OK) {
		closeSpi ();
		return 0;
	}
	
	// Wait for card to write data
	timeout = SD_WRITE_TIMEOUT;
	while (getSpiByte() == 0 && --timeout > 0);
	
	closeSpi();
	
	if (timeout == 0) {
		return 0;
	}
	
	return 1;
}

static int
gmtf_init(void){
	return 1;
}

static int
gmtf_clrstat(void){
	return 1;
}

static int
gmtf_read(ulong sector, ulong numSectors, void* buffer) {
	uchar* data = (uchar*)buffer;
	
	while (numSectors > 0) {
		if (!sdRead (sector, data)) {
			return 0;
		}
		sector ++;
		data += BYTES_PER_SECTOR;
		numSectors --;
	}
	
	return 1;
}

static int
gmtf_write(ulong sector, ulong numSectors, void* buffer) {
	uchar* data = (uchar*)buffer;
	
	while (numSectors > 0) {
		if (!sdWrite (sector, data)) {
			return 0;
		}
		sector ++;
		data += BYTES_PER_SECTOR;
		numSectors --;
	}
	
	return 1;
}

static Ioifc io_gmtf = {
	"GMTF",
	Cread|Cwrite|Cslotnds,

	gmtf_init,
	gmtf_init,
	(void*)gmtf_read,
	(void*)gmtf_write,
	gmtf_clrstat,
	gmtf_clrstat,
};

void
iogmtflink(void)
{
	addioifc(&io_gmtf);
}
