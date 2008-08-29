#include	"u.h"
#include	"io.h"
#include	"mem.h"
#include 	"arm7/jtypes.h"
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
	volatile u32 temp;
	
	CARD_CR1 |= Spiena|Spiirqena;
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
	volatile u32 temp;

	CARD_CR1 |= Spiena|Spiirqena;
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
	
static u8 
transferSpiByte(u8 send)
{
	CARD_EEPDATA = send;
	while (CARD_CR1 & Spibusy);
	return CARD_EEPDATA;
}

static u8
getSpiByte(void)
{
	CARD_EEPDATA = 0xFF;
	while (CARD_CR1 & Spibusy);
	return CARD_EEPDATA;
}

static uchar
sendCommand(u8 command, u32 argument)
{
	u8 commandData[6];
	int timeout;
	u8 spiByte;
	int i;

	commandData[0] = command | 0x40; 
	commandData[1] = (u8)(argument >> 24);
	commandData[2] = (u8)(argument >> 16);
	commandData[3] = (u8)(argument >>  8);
	commandData[4] = (u8)(argument >>  0);
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
sdRead(u32 sector, u8* dest)
{
	u8 spiByte;
	int timeout;
	int i;
	volatile u32 temp;
	
	openSpi ();
	
	if (sendCommand (READ_SINGLE_BLOCK, sector * BYTES_PER_SECTOR) != 0x00) {
		closeSpi ();
		return false;
	}

	// Wait for data start token
	timeout = SD_COMMAND_TIMEOUT;
	do {
		spiByte = getSpiByte ();
	} while (spiByte == 0xFF && --timeout > 0);

	if (spiByte != 0xFE) {
		closeSpi ();
		return false;
	}
	
	for (i = BYTES_PER_SECTOR; i > 0; i--) {
		*dest++ = getSpiByte();
	}

	// Clean up CRC
	temp = getSpiByte();
	temp = getSpiByte();
	
	closeSpi ();
	return true;
}

static int
sdWrite(u32 sector, u8* src)
{
	int i;
	int timeout;
	
	openSpi ();
	
	if (sendCommand (WRITE_SINGLE_BLOCK, sector * BYTES_PER_SECTOR) != 0) {
		closeSpi ();
		return false;
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
		return false;
	}
	
	// Wait for card to write data
	timeout = SD_WRITE_TIMEOUT;
	while (getSpiByte() == 0 && --timeout > 0);
	
	closeSpi();
	
	if (timeout == 0) {
		return false;
	}
	
	return true;
}

static int
gmtf_init(void){
	return true;
}

static int
gmtf_clrstat(void){
	return true;
}

static int
gmtf_read(u32 sector, u32 numSectors, void* buffer) {
	u8* data = (u8*)buffer;
	
	while (numSectors > 0) {
		if (!sdRead (sector, data)) {
			return false;
		}
		sector ++;
		data += BYTES_PER_SECTOR;
		numSectors --;
	}
	
	return true;
}

static int
gmtf_write(u32 sector, u32 numSectors, void* buffer) {
	u8* data = (u8*)buffer;
	
	while (numSectors > 0) {
		if (!sdWrite (sector, data)) {
			return false;
		}
		sector ++;
		data += BYTES_PER_SECTOR;
		numSectors --;
	}
	
	return true;
}

static Ioifc io_gmtf = {
	"GMTF",
	Cread|Cwrite|Cslotgba,

	gmtf_init,
	gmtf_init,
	(void*)gmtf_read,
	(void*)gmtf_write,
	gmtf_clrstat,
	gmtf_clrstat,
};

void
gmtflink(void)
{
	addioifc(&io_gmtf);
}
