/*
 * Copyright (C) 2005
 * Michael Noland (joat), Jason Rogers (dovoto), Dave Murphy (WinterMute)
 */
#include "u.h"
#include "mem.h"
#include "io.h"
#include <kern.h>
#include "arm7/card.h"

void
cardWriteCommand(const uchar * cmd){
	int i;

	ASPIREG->ctl |= Spiena|Spiirqena;

	for (i = 0; i < 8; i++) {
		CARD_COMMAND[7-i] = cmd[i];
	}
}

void
cardPolledTransfer(ulong flags, ulong * dst, ulong len, const uchar * cmd){
	ulong i;

	cardWriteCommand(cmd);
	CARD_CR2 = flags;
	for (i=0; i < len; i++){
		if (!(CARD_CR2 & CARD_BUSY))
			break;
		while (!(CARD_CR2 & CARD_DATA_READY));
		dst[i] = CARD_DATA_RD;
	}
}

void
cardStartTransfer(const uchar * cmd, ulong * dst, int ch, ulong flags){
	DmaReg *dma;

	cardWriteCommand(cmd);
	
	// Set up a DMA channel to transfer a word every time the card makes one
	dma = DMAREG + ch;
	dma->src = (ulong)&CARD_DATA_RD;
	dma->dst = (ulong)dst;
	dma->ctl = Dmaena | Dmastartcard | Dma32bit | Dmarepeat | Dmasrcfix | 0x0001;

	CARD_CR2 = flags;
}

ulong
cardWriteAndRead(const uchar * cmd, ulong flags){
	cardWriteCommand(cmd);
	CARD_CR2 = flags | CARD_ACTIVATE | CARD_nRESET | 0x07000000;
	while (!(CARD_CR2 & CARD_DATA_READY)) ;
	return CARD_DATA_RD;
}

void
cardRead00(ulong addr, ulong * dst, ulong len, ulong flags){
	uchar cmd[8];

	cmd[7] = 0;
	cmd[6] = (addr >> 24) & 0xff;
	cmd[5] = (addr >> 16) & 0xff;
	cmd[4] = (addr >> 8) & 0xff;
	cmd[3] = (addr >> 0) & 0xff;
	cmd[2] = 0;
	cmd[1] = 0;
	cmd[0] = 0;
	cardPolledTransfer(flags, dst, len, cmd);
}

void
cardReadHeader(uchar * header)
{
	cardRead00(0, (ulong *)header, 512, 0xA93F1FFF);
}

int
cardReadID(ulong flags)
{
	uchar cmd[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90};
	return cardWriteAndRead(cmd, flags);
}

static
void EepromWaitBusy(void)
{
	while (ASPIREG->ctl & Spibusy);
}

uchar
cardEepromReadID(uchar i){
    return cardEepromCommand(/*READID*/0xAB, i&1);
}

uchar
cardEepromCommand(uchar cmd, ulong address)
{
	uchar retval;
	int k;

	ASPIREG->ctl = Spiena|Spiselect|Spihold;
	ASPIREG->ctl = 0xFFFF;
	ASPIREG->data = cmd;

	EepromWaitBusy();

	ASPIREG->data = (address >> 16) & 0xFF;
	EepromWaitBusy();

	ASPIREG->data = (address >> 8) & 0xFF;
	EepromWaitBusy();

	ASPIREG->data = (address) & 0xFF;
	EepromWaitBusy();

	for(k=0;k<256;k++)
	{
		retval = ASPIREG->data;
		if(retval!=0xFF)
			break;
		EepromWaitBusy();
	}

	ASPIREG->ctl = Spihold;
	return retval;
}

int
cardEepromGetType(void)
{
	uchar c00;
	uchar c05;
	uchar c9f;
	uchar c03;

	c03=cardEepromCommand(0x03,0);
	c05=cardEepromCommand(0x05,0);
	c9f=cardEepromCommand(0x9f,0);
	c00=cardEepromCommand(0x00,0);

	if((c00==0x00) && (c9f==0x00)) return 0; // PassMe? 
	if((c00==0xff) && (c05==0xff) && (c9f==0xff))return -1;

	if((c00==0xff) &&  (c05 & 0xFD) == 0xF0 && (c9f==0xff))return 1;
	if((c00==0xff) &&  (c05 & 0xFD) == 0x00 && (c9f==0xff))return 2;
	if((c00==0xff) &&  (c05 & 0xFD) == 0x00 && (c9f==0x00))return 3;
	if((c00==0xff) &&  (c05 & 0xFD) == 0x00 && (c9f==0x12))return 3;	//	NEW TYPE 3
	if((c00==0xff) &&  (c05 & 0xFD) == 0x00 && (c9f==0x13))return 3;	//	NEW TYPE 3+  4Mbit
	if((c00==0xff) &&  (c05 & 0xFD) == 0x00 && (c9f==0x14))return 3;	//	NEW TYPE 3++ 8Mbit MK4-FLASH Memory

	return 0;
}

ulong
cardEepromGetSize(void)
{
	int type = cardEepromGetType();

	if(type == -1)
		return 0;
	if(type == 0)
		return 8192;
	if(type == 1)
		return 512;
	if(type == 2) {
		static const ulong offset0 = (8*1024-1);		//		8KB
		static const ulong offset1 = (2*8*1024-1);		//		16KB
		uchar buf1;		//		+0k data		read -> write
		uchar buf2;		//		+8k data		read -> read
		uchar buf3;		//		+0k ~data		   write
		uchar buf4;		//		+8k data new	comp buf2
		cardReadEeprom(offset0,&buf1,1,type);
		cardReadEeprom(offset1,&buf2,1,type);
		buf3=~buf1;
		cardWriteEeprom(offset0,&buf3,1,type);
		cardReadEeprom (offset1,&buf4,1,type);
		cardWriteEeprom(offset0,&buf1,1,type);
		if(buf4!=buf2)		//		+8k
			return 8*1024;	//		 8KB(64kbit)
		else
			return 64*1024; //		64KB(512kbit)
	}
	if(type == 3) {
		uchar c9f;
		c9f=cardEepromCommand(0x9f,0);

		if(c9f==0x14)		  
			return 1024*1024; //   NEW TYPE 3++ 8Mbit(1024KByte)

		if(c9f==0x13)		  
			return 512*1024;  //   NEW TYPE 3+ 4Mbit(512KByte)

		return 256*1024;	  //   TYPE 3  2Mbit(256KByte)
	}

	return 0;
}

void
cardReadEeprom(ulong address, uchar *data, ulong length, ulong addrtype)
{
	ASPIREG->ctl = Spiena|Spiselect|Spihold;
	ASPIREG->data = 0x03 | ((addrtype == 1) ? address>>8<<3 : 0);
	EepromWaitBusy();

	if (addrtype == 3) {
		ASPIREG->data = (address >> 16) & 0xFF;
		EepromWaitBusy();
	} 
	
	if (addrtype >= 2) {
		ASPIREG->data = (address >> 8) & 0xFF;
		EepromWaitBusy();
	}
	
	ASPIREG->data = (address) & 0xFF;
	EepromWaitBusy();

	while (length > 0) {
		ASPIREG->data = 0;
		EepromWaitBusy();
		*data++ = ASPIREG->data;
		length--;
	}

	EepromWaitBusy();
	ASPIREG->ctl = Spihold;
}

void
cardWriteEeprom(ulong address, uchar *data, ulong length, ulong addrtype)
{
	ulong address_end = address + length;
	int i;
	int maxblocks = 32;
	
	if(addrtype == 1) maxblocks = 16;
	if(addrtype == 2) maxblocks = 32;
	if(addrtype == 3) maxblocks = 256;

	while (address < address_end) {
		// set WEL (Write Enable Latch)
		ASPIREG->ctl = Spiena|Spiselect|Spihold;
		ASPIREG->data = 0x06;
		EepromWaitBusy();
		ASPIREG->ctl = Spihold;

		// program maximum of 32 bytes
		ASPIREG->ctl = Spiena|Spiselect|Spihold;

		if(addrtype == 1) {
		//	WRITE COMMAND 0x02 + A8 << 3
			ASPIREG->data = 0x02 | (address & BIT(8)) >> (8-3) ;
			EepromWaitBusy();
			ASPIREG->data = address & 0xFF;
			EepromWaitBusy();
		}
		else if(addrtype == 2) {
			ASPIREG->data = 0x02;
			EepromWaitBusy();
			ASPIREG->data = address >> 8;
			EepromWaitBusy();
			ASPIREG->data = address & 0xFF;
			EepromWaitBusy();
		}
		else if(addrtype == 3) {
			ASPIREG->data = 0x02;
			EepromWaitBusy();
			ASPIREG->data = (address >> 16) & 0xFF;
			EepromWaitBusy();
			ASPIREG->data = (address >> 8) & 0xFF;
			EepromWaitBusy();
			ASPIREG->data = address & 0xFF;
			EepromWaitBusy();
		}
	
		for (i=0; address<address_end && i<maxblocks; i++, address++){ 
			ASPIREG->data = *data++;
			EepromWaitBusy();
		}
	
		ASPIREG->ctl = Spihold;
	
		// wait programming to finish
		ASPIREG->ctl = Spiena|Spiselect|Spihold;
		ASPIREG->data = 0x05;
		EepromWaitBusy();
		do {
			ASPIREG->data = 0;
			EepromWaitBusy();
		} while (ASPIREG->data & 0x01);	// WIP (Write In Progress) ?
	        EepromWaitBusy();
	        ASPIREG->ctl = Spihold;
	}
}

//  Chip Erase : clear FLASH MEMORY (TYPE 3 ONLY)
void
cardEepromChipErase(void) {
	int sz;
	
	sz=cardEepromGetSize();
	cardEepromSectorErase(0x00000);
	cardEepromSectorErase(0x10000);
	cardEepromSectorErase(0x20000);
	cardEepromSectorErase(0x30000);
	if(sz==512*1024)
	{
		cardEepromSectorErase(0x40000);
		cardEepromSectorErase(0x50000);
		cardEepromSectorErase(0x60000);
		cardEepromSectorErase(0x70000);
	}
}

//  COMMAND Sec.erase 0xD8
void
cardEepromSectorErase(ulong address)
{
	// set WEL (Write Enable Latch)
	ASPIREG->ctl = Spiena|Spiselect|Spihold;
	ASPIREG->data = 0x06;
	EepromWaitBusy();

	ASPIREG->ctl = Spihold;

	// SectorErase 0xD8
	ASPIREG->ctl = Spiena|Spiselect|Spihold;
	ASPIREG->data = 0xD8;
	EepromWaitBusy();
	ASPIREG->data = (address >> 16) & 0xFF;
	EepromWaitBusy();
	ASPIREG->data = (address >> 8) & 0xFF;
	EepromWaitBusy();
	ASPIREG->data = address & 0xFF;
	EepromWaitBusy();

	ASPIREG->ctl = Spihold;

	// wait erase to finish
	ASPIREG->ctl = Spiena|Spiselect|Spihold;
	ASPIREG->data = 0x05;
	EepromWaitBusy();
	do
	{
	 	ASPIREG->data = 0;
		EepromWaitBusy();
	} while (ASPIREG->data & 0x01);	// WIP (Write In Progress) ?
	ASPIREG->ctl = Spihold;
}
