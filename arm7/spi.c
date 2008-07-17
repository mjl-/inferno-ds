#include <u.h>
#include "../mem.h"
#include "../io.h"
#include "dat.h"
#include "fns.h"
#include "spi.h"

uchar
power_read(int reg)
{
	return power_write((reg)|PM_READ_REGISTER, 0);
}

uchar
power_write(int reg, int cmd)
{
	while (SPIREG->ctl & Spibusy);
	SPIREG->ctl = Spiena | Spi1mhz | Spibytetx | Spicont | SpiDevpower;
	SPIREG->data = reg;

	while (SPIREG->ctl & Spibusy);
	SPIREG->ctl = Spiena | Spi1mhz | Spibytetx | SpiDevpower;
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
	SPIREG->ctl = Spiena | Spibytetx | Spicont | SpiDevfirmware;
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
