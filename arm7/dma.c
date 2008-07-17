#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>

static void 
dmaCopyWords(char channel, const void* src, void* dest, ulong size, uchar sync)
{
	DmaReg *dmareg;
	if (channel < 0 || channel > 3)
		return;

	dmareg = DMAREG + channel;
	dmareg->src = (ulong)src;
	dmareg->dst = (ulong)dest;
	dmareg->ctl = Dmatxwords | (size>>2);
	if (sync)
		while(dmareg->ctl & Dmabusy);
}

static void
dmaCopyHalfWords(char channel, const void* src, void* dest, ulong size, uchar sync)
{
	DmaReg *dmareg;
	if (channel < 0 || channel > 3)
		return;

	dmareg = DMAREG + channel;
	dmareg->src = (ulong)src;
	dmareg->dst = (ulong)dest;
	dmareg->ctl = Dmatxhwords | (size>>1);
	if (sync)
		while(dmareg->ctl & Dmabusy);
}

#define dmaCopy(src, dest, size, sync)	dmaCopyWords(3, src, dest, size, sync)

static void 
dmaFillWords(const void* src, void* dest, ulong size)
{
	DmaReg *dmareg;

	dmareg = DMAREG + 3;
	dmareg->src = (ulong)src;
	dmareg->dst = (ulong)dest;
	dmareg->ctl = Dmasrcfix | Dmatxwords | (size>>2);
	while(dmareg->ctl & Dmabusy);
}

static void 
dmaFillHalfWords(const void* src, void* dest, ulong size)
{
	DmaReg *dmareg;

	dmareg = DMAREG + 3;
	dmareg->src = (ulong)src;
	dmareg->dst = (ulong)dest;
	dmareg->ctl = Dmasrcfix | Dmatxhwords | (size>>1);
	while(dmareg->ctl & Dmabusy);
}

static  int
dmaBusy(char channel)
{
	DmaReg *dmareg;
	if (channel < 0 || channel > 3)
		return 0;

	dmareg = DMAREG + channel;
	return (dmareg->ctl & Dmabusy)>>31;
}
