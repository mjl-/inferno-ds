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
#include "../io.h"
#include <kern.h>
#include "nds.h"
#include "fns.h"

#define RTC_DELAY 48
#define CS_0    (1<<6)
#define CS_1    ((1<<6) | (1<<2))
#define SCK_0   (1<<5)
#define SCK_1   ((1<<5) | (1<<1))
#define SIO_0   (1<<4)
#define SIO_1   ((1<<4) | (1<<0))
#define SIO_out (1<<4)
#define SIO_in  (1)

#define RTC_PM (1<<6)
#define HOUR_MASK 0x3f

static void
rtcTransaction(uint8 * cmd, uint32 cmdlen, uint8 * res, uint32 reslen)
{
	u8 bit;
	u8 i;

	RTC_CR8 = CS_0 | SCK_1 | SIO_1;
	swiDelay(RTC_DELAY);
	RTC_CR8 = CS_1 | SCK_1 | SIO_1;
	swiDelay(RTC_DELAY);

	for (i = 0; i < cmdlen; i++) {
		for (bit = 0; bit < 8; bit++) {
			RTC_CR8 = CS_1 | SCK_0 | SIO_out | (cmd[i] >> 7);
			swiDelay(RTC_DELAY);

			RTC_CR8 = CS_1 | SCK_1 | SIO_out | (cmd[i] >> 7);
			swiDelay(RTC_DELAY);

			cmd[i] = cmd[i] << 1;
		}
	}

	for (i = 0; i < reslen; i++) {
		res[i] = 0;
		for (bit = 0; bit < 8; bit++) {
			RTC_CR8 = CS_1 | SCK_0;
			swiDelay(RTC_DELAY);
			
			RTC_CR8 = CS_1 | SCK_1;
			swiDelay(RTC_DELAY);

			if (RTC_CR8 & SIO_in)
				res[i] |= (1 << bit);
		}
	}

	RTC_CR8 = CS_0 | SCK_1;
	swiDelay(RTC_DELAY);
}

static u8 
BCDToInt(u8 data)
{
	return ((data & 0xF) + ((data & 0xF0) >> 4) * 10);
}

static u32
get_nds_seconds(u8 * time)
{
	struct Tm tm;
	u8 hours = 0;
	u8 i;

	hours = BCDToInt(time[4] & HOUR_MASK);

	if ((time[4] & RTC_PM) && (hours < 12)) {
		hours += 12;
	}

	for (i = 0; i < 7; i++) {
		time[i] = BCDToInt(time[i]);
	}

	tm.sec  = time[6];
	tm.min  = time[5];
	tm.hour = hours;
	tm.mday = time[2];
	tm.mon  = time[1];
	tm.year = time[0] + 2000;	
	tm.tzoff = -1;

	if(0)
	print("%d:%d:%d %d/%d/%d [HH:MM:SS YY/MM/DD]\n",
			tm.hour, tm.min, tm.sec, tm.year, tm.mon, tm.mday);
	
	return tm2sec(&tm);	
}

ulong
nds_get_time7(void)
{
	u8 command;
	u8 time[8];
	unsigned int seconds;

	command = READ_DATA_REG1;
	rtcTransaction(&command, 1, &(time[1]), 7);

	command = READ_STATUS_REG1;
	rtcTransaction(&command, 1, &(time[0]), 1);

	seconds = get_nds_seconds(&(time[1]));

	return seconds;

}

void
nds_set_time7(ulong secs){
	/* TODO
	 * 1 convert secs to tm (using sec2tm)
	 * 2 fill time[8] with tm
	 * 3 update rtc with time[8]
	 */
};

static void
rtcSetTimeAndDate(uint8 * time)
{
	uint8 cmd[8];
	
	int i;
	for ( i=0; i< 8; i++ ) {
		cmd[i+1] = time[i];
	}
	cmd[0] = WRITE_DATA_REG1;
	rtcTransaction(cmd, 8, 0, 0);
}

static void
rtcSetTime(uint8 * time)
{
	uint8 cmd[4];
	
	int i;
	for ( i=0; i< 3; i++ ) {
		cmd[i+1] = time[i];
	}
	cmd[0] = WRITE_TIME;
	rtcTransaction(cmd, 4, 0, 0);
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
