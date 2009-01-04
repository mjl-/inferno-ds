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

	while (!ready){
		cardWriteCommand(cmd);
		CARD_CR2 = flags;
		if(!(CARD_CR2 & CARD_BUSY))
			continue;
		if(!(CARD_CR2 & CARD_DATA_READY))
			continue;
		if (!CARD_DATA_RD)
			ready = 1;
	} 
}

static void
bytecardPolledTransfer(ulong flags, ulong * dst, ulong len, uchar* cmd) {
	ulong i, data;
	uchar *p;
	
	cardWriteCommand(cmd);
	CARD_CR2 = flags;
	for (i=0; i < len; i++){
		if (!(CARD_CR2 & CARD_BUSY))
			break;
		while (!(CARD_CR2 & CARD_DATA_READY));
		data = CARD_DATA_RD;
		p = (uchar*)&dst[i];

		*p++ = data & 0xff;
		*p++ = (data >> 8) & 0xff;
		*p++ = (data >> 16) & 0xff;
		*p++ = (data >> 24) & 0xff;
	}
}

static void
LogicCardRead(ulong addr, ulong *dst, ulong len)
{
	uchar cmd[8];

	cmd[7] = 0xb9;
	cmd[6] = (addr >> 24) & 0xff;
	cmd[5] = (addr >> 16) & 0xff;
	cmd[4] = (addr >> 8)  & 0xff;
	cmd[3] =  addr        & 0xff;
	cmd[2] = 0;
	cmd[1] = 0;
	cmd[0] = 0;
	cardWaitReady(0xa7586000, cmd);
	cmd[7] = 0xba;
	if ((ulong)dst & 0x03)
		bytecardPolledTransfer(0xa1586000, dst, len, cmd);
	else
		cardPolledTransfer(0xa1586000, dst, len, cmd);
}

static ulong
ReadCardInfo(void)
{
	uchar cmd[8] = {0, 0, 0, 0, 0, 0, 0, 0xb0};
	ulong ret;

	cardPolledTransfer(0xa7586000, &ret, 1, cmd);
	return ret;
}

static void
LogicCardWrite(ulong addr, ulong *src, ulong len)
{
	uchar cmd[8];
	ulong i;

	cmd[7] = 0xbb;
	cmd[6] = (addr >> 24) & 0xff;
	cmd[5] = (addr >> 16) & 0xff;
	cmd[4] = (addr >> 8)  & 0xff;
	cmd[3] =  addr        & 0xff;
	cmd[2] = 0;
	cmd[1] = 0;
	cmd[0] = 0;

	cardWriteCommand(cmd);
	CARD_CR2 = 0xe1586000;

	if ((ulong)src & 0x03){
		for(i=0; i<len; i++){
			 if(!(CARD_CR2 & CARD_BUSY))
			 	break;
			while(!(CARD_CR2 & CARD_DATA_READY));
			CARD_DATA_RD = ((uchar*)src[i])[0] | 
					(((uchar*)src[i])[1] << 8) |
					(((uchar*)src[i])[2] << 16) |
					(((uchar*)src[i])[3] << 24);
		}
	}else{
		for(i=0; i<len; i++){
			 if(!(CARD_CR2 & CARD_BUSY))
			 	break;
			while(!(CARD_CR2 & CARD_DATA_READY));
			CARD_DATA_RD = src[i];
		}
	}
	
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

enum
{
	Blkbits = 9,
	Blksize = 1<<9,
};

static int
r4tf_read(ulong start, ulong n, void* d)
{
	ulong *ud = (ulong*)d, i;

	for (i = 0; i < n; i++) {
		LogicCardRead((start+i)<<Blkbits, ud, (Blksize/4));
		ud += (Blksize/4);
	}
	return 1;
}

static int
r4tf_write(ulong start, ulong n, void* d)
{
	ulong *ud = (ulong*)d, i;

	for (i = 0; i < n; i++) {
		LogicCardWrite((start+i)<<Blkbits, ud, (Blksize/4));
		ud += (Blksize/4);
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
