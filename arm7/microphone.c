/*---------------------------------------------------------------------------------
	$Id: microphone.c,v 1.5 2007/06/25 20:23:35 wntrmute Exp $
	Microphone control for the ARM7
	Copyright (C) 2005
			Michael Noland (joat)
			Jason Rogers (dovoto)
			Dave Murphy (WinterMute)
			Chris Double (doublec)
	This software is provided 'as-is', without any express or implied
	warranty.	In no event will the authors be held liable for any
	damages arising from the use of this software.
	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:
	1. The origin of this software must not be misrepresented; you
		 must not claim that you wrote the original software. If you use
		 this software in a product, an acknowledgment in the product
		 documentation would be appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and
		 must not be misrepresented as being the original software.
	3. This notice may not be removed or altered from any source
		 distribution.
---------------------------------------------------------------------------------*/
#include <u.h>
#include "../mem.h"
#include "nds.h"

void 
PM_SetAmp(u8 control) 
{
	busywait();
	REG_SPICNT = Spien | SPI_DEVICE_POWER | SPI_BAUD_1MHz | Spicont;
	REG_SPIDATA = PM_AMP_OFFSET;
	busywait();
	REG_SPICNT = Spien | SPI_DEVICE_POWER | SPI_BAUD_1MHz;
	REG_SPIDATA = control;
}

u8 
MIC_ReadData() {
	u16 res, res2;
	busywait();
	REG_SPICNT = Spien | SPI_DEVICE_MICROPHONE | Spi2mhz | Spicont;
	REG_SPIDATA = 0xEC;	// Touchscreen cmd format for AUX
	busywait();
	REG_SPIDATA = 0x00;
	busywait();
	res = REG_SPIDATA;
		REG_SPICNT = Spien | Spitouch | Spi2mhz;
	REG_SPIDATA = 0x00;
	busywait();
	res2 = REG_SPIDATA;
	return (((res & 0x7F) << 1) | ((res2>>7)&1));
}
static u8* micbuf = 0;
static int micbuflen = 0;
static int curlen = 0;
void 
StartRecording(u8* buffer, int length) 
{
	micbuf = buffer;
	micbuflen = length;
	curlen = 0;
	MIC_On();
//	 Setup a 16kHz timer
	TIMER0_DATA = 0xF7CF;
	TIMER0_CR = TIMER_ENABLE | TIMER_DIV_1 | TIMER_IRQ_REQ;
}
int 
StopRecording() 
{
	TIMER0_CR &= ~TIMER_ENABLE;
	MIC_Off();
	micbuf = 0;
	return curlen;
}
void	
ProcessMicrophoneTimerIRQ() 
{
	if(micbuf && micbuflen > 0) {
		*micbuf++ = MIC_ReadData() ^ 0x80;
		--micbuflen;
		curlen++;
	}
}
