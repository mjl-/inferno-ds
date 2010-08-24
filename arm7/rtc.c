#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "fns.h"

// RTC registers
#define WRITE_STATUS_REG1	0x60
#define READ_STATUS_REG1	0x61

// full 7 bytes for time and date
#define WRITE_DATA_REG1	0x64
#define READ_DATA_REG1	0x65

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
rtcTransaction(uchar *cmd, ulong cmdlen, uchar *res, ulong reslen)
{
	uchar bit;
	uchar i;

	KEYREG->rtccr = CS_0 | SCK_1 | SIO_1;
	swiDelay(RTC_DELAY);
	KEYREG->rtccr = CS_1 | SCK_1 | SIO_1;
	swiDelay(RTC_DELAY);

	for (i = 0; i < cmdlen; i++) {
		for (bit = 0; bit < 8; bit++) {
			KEYREG->rtccr = CS_1 | SCK_0 | SIO_out | (cmd[i] >> 7);
			swiDelay(RTC_DELAY);

			KEYREG->rtccr = CS_1 | SCK_1 | SIO_out | (cmd[i] >> 7);
			swiDelay(RTC_DELAY);

			cmd[i] = cmd[i] << 1;
		}
	}

	for (i = 0; i < reslen; i++) {
		res[i] = 0;
		for (bit = 0; bit < 8; bit++) {
			KEYREG->rtccr = CS_1 | SCK_0;
			swiDelay(RTC_DELAY);
			
			KEYREG->rtccr = CS_1 | SCK_1;
			swiDelay(RTC_DELAY);

			if (KEYREG->rtccr & SIO_in)
				res[i] |= (1 << bit);
		}
	}

	KEYREG->rtccr = CS_0 | SCK_1;
	swiDelay(RTC_DELAY);
}

static uchar
BCDToInt(uchar data)
{
	return ((data & 0xF) + ((data & 0xF0) >> 4) * 10);
}

static ulong
get_nds_seconds(uchar *time)
{
	struct Tm tm;
	uchar hours;
	uchar i;

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

	if(0)print("%d:%d:%d %d/%d/%d [HH:MM:SS YY/MM/DD]\n",
			tm.hour, tm.min, tm.sec, tm.year, tm.mon, tm.mday);
	
	return tm2sec(&tm);	
}

ulong
nds_get_time7(void)
{
	uchar cmd;
	uchar time[8];

	cmd = READ_DATA_REG1;
	rtcTransaction(&cmd, 1, &(time[1]), 7);

	cmd = READ_STATUS_REG1;
	rtcTransaction(&cmd, 1, &(time[0]), 1);

	return get_nds_seconds(&(time[1]));

}

static void sec2tm(ulong secs, Tm *tm);

void
nds_set_time7(ulong secs){
	/* TODO: revise */
	struct Tm tm;
	uchar time[8];

	sec2tm(secs, &tm);

	time[6+1] = tm.sec;
	time[5+1] = tm.min;
	time[4+1] = tm.hour;
	time[2+1] = tm.mday;
	time[1+1] = tm.mon;
	time[0+1] = tm.year - 2000;
	tm.tzoff = -1;

	time[0] = WRITE_DATA_REG1;
	rtcTransaction(time, 8, 0, 0);
};

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
	if( (yr % 4 == 0) && (yr % 100 != 0 || yr % 400 == 0))
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

static void
sec2tm(ulong secs, Tm *tm)
{
	int d;
	long hms, day;
	int *d2m;

	/*
	 * break initial number into days
	 */
	hms = secs % SEC2DAY;
	day = secs / SEC2DAY;
	if(hms < 0) {
		hms += SEC2DAY;
		day -= 1;
	}

	/*
	 * generate hours:minutes:seconds
	 */
	tm->sec = hms % 60;
	d = hms / 60;
	tm->min = d % 60;
	d /= 60;
	tm->hour = d;

	/*
	 * year number
	 */
	if(day >= 0)
		for(d = 1970; day >= *yrsize(d); d++)
			day -= *yrsize(d);
	else
		for (d = 1970; day < 0; d--)
			day += *yrsize(d-1);
	tm->year = d;

	/*
	 * generate month
	 */
	d2m = yrsize(tm->year);
	for(d = 1; day >= d2m[d]; d++)
		day -= d2m[d];
	tm->mday = day + 1;
	tm->mon = d;

	return;
}
