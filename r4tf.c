#include	"u.h"
#include	"io.h"
#include	"mem.h"
#include 	"arm7/jtypes.h"
#include	"arm7/card.h"

/*
 * r4tf flashcard, based on libfat's disc_io code
 */

#define DPRINT if(1)

static void
cardWaitReady(u32 flags, u8 *cmd)
{
	bool ready = false;

	do {
		cardWriteCommand(cmd);
		CARD_CR2 = flags;
		do {
			if (CARD_CR2 & CARD_DATA_READY)
				if (!CARD_DATA_RD) ready = true;
		} while (CARD_CR2 & CARD_BUSY);
	} while (!ready);
}

static void
bytecardPolledTransfer(uint32 flags, uint32 * dst, uint32 len, uint8 * cmd) {
	u32 data;
	uint32 *target;

	cardWriteCommand(cmd);
	CARD_CR2 = flags;
	target = dst + len;
	do {
		// Read data if available
		if (CARD_CR2 & CARD_DATA_READY) {
			data=CARD_DATA_RD;
			if (dst < target) {
				((uint8*)dst)[0] = data & 0xff;
				((uint8*)dst)[1] = (data >> 8) & 0xff;
				((uint8*)dst)[2] = (data >> 16) & 0xff;
				((uint8*)dst)[3] = (data >> 24) & 0xff;
			}
			dst++;
		}
	} while (CARD_CR2 & CARD_BUSY);
}

static void
LogicCardRead(u32 address, u32 *dst, u32 len)
{
	u8 cmd[8];

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
	if ((u32)dst & 0x03)
		bytecardPolledTransfer(0xa1586000, dst, len, cmd);
	else
		bytecardPolledTransfer(0xa1586000, dst, len, cmd);
	//	TODO find out why the line below fails
	//	cardPolledTransfer(0xa1586000, dst, len, cmd);
}

static u32
ReadCardInfo(void)
{
	u8 cmd[8];
	u32 ret;

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
LogicCardWrite(u32 address, u32 *source, u32 len)
{
	u8 cmd[8];
	u32 data = 0;
	u32 * target;

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
				if ((u32)source & 0x03)
					data = ((uint8*)source)[0] | (((uint8*)source)[1] << 8) | (((uint8*)source)[2] << 16) | (((uint8*)source)[3] << 24);
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

static bool
r4tf_init(void)
{
	u32 CardInfo;

	CardInfo = ReadCardInfo();
	return ((CardInfo & 0x07) == 0x04);
}

static bool
r4tf_read(u32 sector, u8 numSecs, void* buffer)
{
	u32 *u32_buffer = (u32*)buffer, i;

	for (i = 0; i < numSecs; i++) {
		LogicCardRead(sector << 9, u32_buffer, 128);
		sector++;
		u32_buffer += 128;
	}
	return true;
}

static bool
r4tf_write(u32 sector, u8 numSecs, void* buffer)
{
	u32 *u32_buffer = (u32*)buffer, i;

	for (i = 0; i < numSecs; i++) {
		LogicCardWrite(sector << 9, u32_buffer, 128);
		sector++;
		u32_buffer += 128;
	}
	return true;
}

static bool
r4tf_clrstat(void)
{
	return true;
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
r4tflink(void)
{
	addioifc(&io_r4tf);
}
