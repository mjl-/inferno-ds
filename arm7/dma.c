#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "jtypes.h"
#include "spi.h"

static void 
dmaCopyWords(int8 channel, const void* src, void* dest, uint32 size, uchar sync)
{
	DmaReg *dmareg;
	if (channel < 0 || channel > 3)
		return;

	dmareg = DMAREG + channel;
	dmareg->src = (uint32)src;
	dmareg->dst = (uint32)dest;
	dmareg->ctl = Dmatxwords | (size>>2);
	if (sync)
		while(dmareg->ctl & Dmabusy);
}

static void
dmaCopyHalfWords(int8 channel, const void* src, void* dest, uint32 size, uchar sync)
{
	DmaReg *dmareg;
	if (channel < 0 || channel > 3)
		return;

	dmareg = DMAREG + channel;
	dmareg->src = (uint32)src;
	dmareg->dst = (uint32)dest;
	dmareg->ctl = Dmatxhwords | (size>>1);
	if (sync)
		while(dmareg->ctl & Dmabusy);
}

#define dmaCopy(src, dest, size, sync)	dmaCopyWords(3, src, dest, size, sync)

static void 
dmaFillWords(const void* src, void* dest, uint32 size)
{
	DmaReg *dmareg;

	dmareg = DMAREG + 3;
	dmareg->src = (uint32)src;
	dmareg->dst = (uint32)dest;
	dmareg->ctl = Dmasrcfix | Dmatxwords | (size>>2);
	while(dmareg->ctl & Dmabusy);
}

static void 
dmaFillHalfWords(const void* src, void* dest, uint32 size)
{
	DmaReg *dmareg;

	dmareg = DMAREG + 3;
	dmareg->src = (uint32)src;
	dmareg->dst = (uint32)dest;
	dmareg->ctl = Dmasrcfix | Dmatxhwords | (size>>1);
	while(dmareg->ctl & Dmabusy);
}

static  int
dmaBusy(int8 channel)
{
	DmaReg *dmareg;
	if (channel < 0 || channel > 3)
		return 0;

	dmareg = DMAREG + channel;
	return (dmareg->ctl & Dmabusy)>>31;
}
