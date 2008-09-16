#include	"u.h"
#include	"io.h"
#include	"mem.h"
#include	"arm7/card.h"

/*
 * storage support for the r4ds card
 * Copyright (c) 2006 Michael "Chishm" Chisholm
 */

#define DPRINT if(1)

static void
cardWaitReady(ulong flags, uchar *cmd)
{
	int ready = 0;

	do {
		cardWriteCommand(cmd);
		CARD_CR2 = flags;
		do {
			if (CARD_CR2 & CARD_DATA_READY)
				if (!CARD_DATA_RD) ready = 1;
		} while (CARD_CR2 & CARD_BUSY);
	} while (!ready);
}

static void
bytecardPolledTransfer(ulong flags, ulong * dst, ulong len, uchar* cmd) {
	ulong data;
	ulong *target;

	cardWriteCommand(cmd);
	CARD_CR2 = flags;
	target = dst + len;
	do {
		// Read data if available
		if (CARD_CR2 & CARD_DATA_READY) {
			data=CARD_DATA_RD;
			if (dst < target) {
				((uchar*)dst)[0] = data & 0xff;
				((uchar*)dst)[1] = (data >> 8) & 0xff;
				((uchar*)dst)[2] = (data >> 16) & 0xff;
				((uchar*)dst)[3] = (data >> 24) & 0xff;
			}
			dst++;
		}
	} while (CARD_CR2 & CARD_BUSY);
}

static void
LogicCardRead(ulong address, ulong *dst, ulong len)
{
	uchar cmd[8];

	cmd[7] = 0xb9;
	cmd[6] = (address >> 24) & 0xff;
	cmd[5] = (address >> 16) & 0xff;
	cmd[4] = (address >> 8)  & 0xff;
	cmd[3] =  address        & 0xff;
	cmd[2] = 0;
	cmd[1] = 0;
	cmd[0] = 0;
	cardWaitReady(0xa7586000, cmd);
	cmd[7] = 0xba;
	if ((ulong)dst & 0x03)
		bytecardPolledTransfer(0xa1586000, dst, len, cmd);
	else
		bytecardPolledTransfer(0xa1586000, dst, len, cmd);
	//	TODO find out why the line below fails
	//	cardPolledTransfer(0xa1586000, dst, len, cmd);
}

static ulong
ReadCardInfo(void)
{
	uchar cmd[8];
	ulong ret;

	cmd[7] = 0xb0;
	cmd[6] = 0;
	cmd[5] = 0;
	cmd[4] = 0;
	cmd[3] = 0;
	cmd[2] = 0;
	cmd[1] = 0;
	cmd[0] = 0;
	cardPolledTransfer(0xa7586000, &ret, 1, cmd);
	return ret;
}

static void
LogicCardWrite(ulong address, ulong *source, ulong len)
{
	uchar cmd[8];
	ulong data = 0;
	ulong * target;

	cmd[7] = 0xbb;
	cmd[6] = (address >> 24) & 0xff;
	cmd[5] = (address >> 16) & 0xff;
	cmd[4] = (address >> 8)  & 0xff;
	cmd[3] =  address        & 0xff;
	cmd[2] = 0;
	cmd[1] = 0;
	cmd[0] = 0;

	cardWriteCommand(cmd);
	CARD_CR2 = 0xe1586000;
	target = source + len;
	do {
		// Write data if ready
		if (CARD_CR2 & CARD_DATA_READY) {
			if (source < target) {
				if ((ulong)source & 0x03)
					data = ((uchar*)source)[0] | (((uchar*)source)[1] << 8) | (((uchar*)source)[2] << 16) | (((uchar*)source)[3] << 24);
				else
					data = *source;
			}
			source++;
			CARD_DATA_RD = data;
		}
	} while (CARD_CR2 & CARD_BUSY);
	cmd[7] = 0xbc;
	cardWaitReady(0xa7586000, cmd);
}

static int
r4tf_init(void)
{
	ulong CardInfo;

	CardInfo = ReadCardInfo();
	return ((CardInfo & 0x07) == 0x04);
}

static int
r4tf_read(ulong sector, uchar numSecs, void* buffer)
{
	ulong *ulong_buffer = (ulong*)buffer, i;

	for (i = 0; i < numSecs; i++) {
		LogicCardRead(sector << 9, ulong_buffer, 128);
		sector++;
		ulong_buffer += 128;
	}
	return 1;
}

static int
r4tf_write(ulong sector, uchar numSecs, void* buffer)
{
	ulong *ulong_buffer = (ulong*)buffer, i;

	for (i = 0; i < numSecs; i++) {
		LogicCardWrite(sector << 9, ulong_buffer, 128);
		sector++;
		ulong_buffer += 128;
	}
	return 1;
}

static int
r4tf_clrstat(void)
{
	return 1;
}

static Ioifc io_r4tf = {
	"R4TF",
	Cread|Cwrite|Cslotnds,

	r4tf_init,
	r4tf_init,
	(void*)r4tf_read,
	(void*)r4tf_write,
	r4tf_clrstat,
	r4tf_clrstat,
};

void
ior4tflink(void)
{
	addioifc(&io_r4tf);
}
