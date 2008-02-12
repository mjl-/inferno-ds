/*---------------------------------------------------------------------------------
	$Id: clock.c,v 1.6 2007/06/16 01:09:02 wntrmute Exp $
	Copyright (C) 2005
		Michael Noland (Joat)
		Jason Rogers (Dovoto)
		Dave Murphy (WinterMute)
	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any
	damages arising from the use of this software.
	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:
	1.	The origin of this software must not be misrepresented; you
			must not claim that you wrote the original software. If you use
			this software in a product, an acknowledgment in the product
			documentation would be appreciated but is not required.
	2.	Altered source versions must be plainly marked as such, and
			must not be misrepresented as being the original software.
	3.	This notice may not be removed or altered from any source
			distribution.
---------------------------------------------------------------------------------*/
#include <u.h>
#include "../mem.h"
#include <kern.h>

#include "nds.h"
//#include <time.h>

#define RTC_DELAY 48
#define CS_0    (1<<6)
#define CS_1    ((1<<6) | (1<<2))
#define SCK_0   (1<<5)
#define SCK_1   ((1<<5) | (1<<1))
#define SIO_0   (1<<4)
#define SIO_1   ((1<<4) | (1<<0))
#define SIO_out (1<<4)
#define SIO_in  (1)

void 
BCDToInteger(uint8 * data, uint32 length) {
	u32 i;
	for (i = 0; i < length; i++) {
		data[i] = (data[i] & 0xF) + ((data[i] & 0xF0)>>4)*10;
	}
}
void integerToBCD(uint8 * data, uint32 length) {
	u32 i;
	for (i = 0; i < length; i++) {
		int high, low;
		swiDivMod(data[i], 10, &high, &low);
		data[i] = (high<<4) | low;
	}
}
void rtcTransaction(uint8 * cmd, uint32 cmdLength, uint8 * res, uint32 resLength) {
	uint32 bit;
	uint8 data;
	
	RTC_CR8 = CS_0 | SCK_1 | SIO_1;
	swiDelay(RTC_DELAY);
	RTC_CR8 = CS_1 | SCK_1 | SIO_1;
	swiDelay(RTC_DELAY);
		data = *cmd++;
		for (bit = 0; bit < 8; bit++) {
			RTC_CR8 = CS_1 | SCK_0 | SIO_out | (data>>7);
			swiDelay(RTC_DELAY);
			RTC_CR8 = CS_1 | SCK_1 | SIO_out | (data>>7);
			swiDelay(RTC_DELAY);
			data = data << 1;
		}
	for ( ; cmdLength > 1; cmdLength--) {
		data = *cmd++;
		for (bit = 0; bit < 8; bit++) {
			RTC_CR8 = CS_1 | SCK_0 | SIO_out | (data & 1);
			swiDelay(RTC_DELAY);
			RTC_CR8 = CS_1 | SCK_1 | SIO_out | (data & 1);
			swiDelay(RTC_DELAY);
			data = data >> 1;
		}
	}
	for ( ; resLength > 0; resLength--) {
		data = 0;
		for (bit = 0; bit < 8; bit++) {
			RTC_CR8 = CS_1 | SCK_0;
			swiDelay(RTC_DELAY);
			RTC_CR8 = CS_1 | SCK_1;
			swiDelay(RTC_DELAY);
			if (RTC_CR8 & SIO_in) data |= (1 << bit);
		}
		*res++ = data;
	}
	RTC_CR8 = CS_0 | SCK_1;
	swiDelay(RTC_DELAY);
}
void 
rtcReset(void) 
{
	uint8 status;
	uint8 cmd[2];
	cmd[0] = READ_STATUS_REG1;
	rtcTransaction(cmd, 1, &status, 1);
	if (status & (STATUS_POC | STATUS_BLD)) {
		cmd[0] = WRITE_STATUS_REG1;
		cmd[1] = status | STATUS_RESET;
		rtcTransaction(cmd, 2, 0, 0);
	}
}
void rtcGetTimeAndDate(uint8 * time) {
	uint8 cmd, status;
	cmd = READ_TIME_AND_DATE;
	rtcTransaction(&cmd, 1, time, 7);
	cmd = READ_STATUS_REG1;
	rtcTransaction(&cmd, 1, &status, 1);
	if ( status & STATUS_24HRS ) {
		time[4] &= 0x3f;
	} else {
	}
	BCDToInteger(time,7);
}
void rtcSetTimeAndDate(uint8 * time) {
	uint8 cmd[8];
	
	int i;
	for ( i=0; i< 8; i++ ) {
		cmd[i+1] = time[i];
	}
	cmd[0] = WRITE_TIME_AND_DATE;
	rtcTransaction(cmd, 8, 0, 0);
}
void rtcGetTime(uint8 * time) {
	uint8 cmd, status;
	cmd = READ_TIME;
	rtcTransaction(&cmd, 1, time, 3);
	cmd = READ_STATUS_REG1;
	rtcTransaction(&cmd, 1, &status, 1);
	if ( status & STATUS_24HRS ) {
		time[0] &= 0x3f;
	} else {
	}
	BCDToInteger(time,3);
}
void rtcSetTime(uint8 * time) {
	uint8 cmd[4];
	
	int i;
	for ( i=0; i< 3; i++ ) {
		cmd[i+1] = time[i];
	}
	cmd[0] = WRITE_TIME;
	rtcTransaction(cmd, 4, 0, 0);
}
void syncRTC() {
	if (++IPC->time.rtc.seconds == 60 ) {
		IPC->time.rtc.seconds = 0;
		if (++IPC->time.rtc.minutes == 60) {
			IPC->time.rtc.minutes  = 0;
			if (++IPC->time.rtc.hours == 24) {
				rtcGetTimeAndDate((uint8 *)&(IPC->time.rtc.year));
			}
		}
	}
	
	IPC->unixTime++;
}

void initclkirq() {
	uint8 cmd[4];
	struct Tm currentTime;
	
	REG_RCNT = 0x8100;
	irqset(IRQ_NETWORK, syncRTC);
	rtcReset();
	cmd[0] = READ_STATUS_REG2;
	rtcTransaction(cmd, 1, &cmd[1], 1);
	cmd[0] = WRITE_STATUS_REG2;
	cmd[1] = 0x41;
	rtcTransaction(cmd, 2, 0, 0);
	
	cmd[0] = WRITE_INT_REG1;
	cmd[1] = 0x01;
	rtcTransaction(cmd, 2, 0, 0);
	
	cmd[0] = WRITE_INT_REG2;
	cmd[1] = 0x00;
	cmd[2] = 0x21;
	cmd[3] = 0x35;
	rtcTransaction(cmd, 4, 0, 0);
//	 Read all time settings on first start
	rtcGetTimeAndDate((uint8 *)&(IPC->time.rtc.year));

	currentTime.sec  = IPC->time.rtc.seconds;
	currentTime.min  = IPC->time.rtc.minutes;
	currentTime.hour = IPC->time.rtc.hours;
	currentTime.mday = IPC->time.rtc.day;
	currentTime.mon  = IPC->time.rtc.month - 1;
	currentTime.year = IPC->time.rtc.year + 100;
	
	currentTime.tzoff = -1;
	IPC->unixTime = tm2sec(&currentTime);
}

#define SEC2MIN 60L
#define SEC2HOUR (60L*SEC2MIN)
#define SEC2DAY (24L*SEC2HOUR)

/*
 *  days per month plus days/year
 */
static	int	dmsize[] =
{
	365, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
static	int	ldmsize[] =
{
	366, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 *  return the days/month for the given year
 */
static int*
yrsize(int yr)
{
	if( (yr % 4 == 0)) // && (yr % 100 != 0 || yr % 400 == 0) 
		return ldmsize;
	else
		return dmsize;
}

// taken from emu/Nt/ie-os.c
long
tm2sec(Tm *tm)
{
	long secs;
	int i, *d2m;

	secs = 0;

	/*
	 *  seconds per year
	 */
	for(i = 1970; i < tm->year; i++){
		d2m = yrsize(i);
		secs += d2m[0] * SEC2DAY;
	}

	/*
	 *  seconds per month
	 */
	d2m = yrsize(tm->year);
	for(i = 1; i < tm->mon; i++){
		secs += d2m[i] * SEC2DAY;
	}

	/*
	 * secs in last month
	 */
	secs += (tm->mday-1) * SEC2DAY;

	/*
	 * hours, minutes, seconds
	 */
	secs += tm->hour * SEC2HOUR;
	secs += tm->min * SEC2MIN;
	secs += tm->sec;

	return secs;
}
