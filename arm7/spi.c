#include <u.h>
#include "../mem.h"
#include "../io.h"
#include "dat.h"
#include "fns.h"
#include "spi.h"

static short
touch_read(ulong cmd) 
{
	ushort res;

	busywait();

	SPIREG->ctl=Spiena|Spi2mhz|SpiDevtouch|Spicont; //0x8A01;
	SPIREG->data=cmd;
	busywait();

	SPIREG->data=0;
	busywait();
	res=SPIREG->data;

	SPIREG->ctl=Spiena|SpiDevtouch|Spi2mhz;
	SPIREG->data=0;
	busywait();

	res = ((res & 0x7F) << 5) | (SPIREG->data >> 3);
	SPIREG->ctl = 0;

	return res;
}

/* based on: linux-2.6.x/arch/arm/mach-nds/arm7/spi.c */
int
touch_read_value(int cmd, int retry , int range){
	int i;
	int cv, cr;
	int ov;
	
	cv = 0;
	ov = touch_read(cmd | 1);
	for (i=0; i < retry; i++) {
		cv = touch_read(cmd | 1);
		if ((ov - cv) > 0)
			cr = ov - cv;
		else
			cr = cv - ov;

		if (cr <= range)
			break;
	}

	if (i == range)
		cv = 0;
	return cv;
}

uchar
power_read(int reg)
{
	return power_write((reg)|PM_READ_REGISTER, 0);
}

uchar
power_write(int reg, int cmd)
{
	while (SPIREG->ctl & Spibusy);
	SPIREG->ctl = Spiena|Spi1mhz|Spibytetx|Spicont|SpiDevpower;
	SPIREG->data = reg;

	while (SPIREG->ctl & Spibusy);
	SPIREG->ctl = Spiena|Spi1mhz|Spibytetx|SpiDevpower;
	SPIREG->data = cmd;

	while (SPIREG->ctl & Spibusy);
	return SPIREG->data & 0xFF;
}

void
read_firmware(ulong src, void *dst, ulong sz)
{
	uchar *p;
	
	p = (uchar *)dst;
	while (SPIREG->ctl & Spibusy);
	SPIREG->ctl = Spiena|Spibytetx|Spicont|SpiDevfirmware;
	SPIREG->data = FIRMWARE_READ;
	while (SPIREG->ctl & Spibusy);

	SPIREG->data = (src>>16) & 0xFF;
	while (SPIREG->ctl & Spibusy);
	SPIREG->data = (src>>8) & 0xFF;
	while (SPIREG->ctl & Spibusy);
	SPIREG->data = (src>>0) & 0xFF;
	while (SPIREG->ctl & Spibusy);

	while (sz--) {
		SPIREG->data = 0;
		while (SPIREG->ctl & Spibusy);
		*p++ = (SPIREG->data & 0xFF);
	}

	SPIREG->ctl = 0;
}

void
busywait(void) 
{
	while (SPIREG->ctl & Spibusy)
		swiDelay(1);
}
