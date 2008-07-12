/* 
 * audio code based on libnds and dslinux audio drivers
 * using gbatek.txt as source of documentation
 */

#include <u.h>
#include "../mem.h"
#include "../io.h"
#include "dat.h"
#include "fns.h"
#include "jtypes.h"
#include "ipc.h"
#include "audio.h"
#include "spi.h"

// TODO use SPIREG from io.h
void 
PM_SetAmp(u8 control) 
{
	busywait();
	SPIREG->ctl = Spiena | SpiDevpower | Spi1mhz | Spicont;
	SPIREG->data = PM_AMP_OFFSET;
	busywait();
	SPIREG->ctl = Spiena | SpiDevpower | Spi1mhz;
	SPIREG->data = control;
}

u8 
MIC_ReadData(void) {
	u16 res, res2;
	busywait();
	SPIREG->ctl = Spiena | SpiDevmic | Spi2mhz | Spicont;
	SPIREG->data = 0xEC;	// Touchscreen cmd format for AUX
	busywait();
	SPIREG->data = 0x00;
	busywait();
	res = SPIREG->data;
	SPIREG->ctl = Spiena | SpiDevtouch | Spi2mhz;
	SPIREG->data = 0x00;
	busywait();
	res2 = SPIREG->data;
	return (((res & 0x7F) << 1) | ((res2>>7)&1));
}

static u8* micbuf = 0;
static int micbuflen = 0;
static int curlen = 0;

static void
clock1intr(void)
{
	if(micbuf && micbuflen > 0) {
		*micbuf++ = MIC_ReadData() ^ 0x80;
		--micbuflen;
		curlen++;
	}
	intrclear(TIMER1bit, 0);
}

void 
StartRecording(u8* buffer, int length) 
{
	TimerReg *t = TIMERREG + 1;

	micbuf = buffer;
	micbuflen = length;
	curlen = 0;
	PM_SetAmp(PM_AMP_ON);
	
	// Setup a 16kHz timer
	t->data = TIMER_BASE(Tmrdiv1) / 16000;
	t->ctl = Tmrena | Tmrdiv1 | Tmrirq;
	intrenable(TIMER1bit, clock1intr, 0);
}

int 
StopRecording(void) 
{
	TimerReg *t = TIMERREG + AUDIOtimer;

	t->ctl &= ~Tmrena;
	intrmask(TIMER1bit, 0);

	PM_SetAmp(PM_AMP_OFF);
	micbuf = 0;
	return curlen;
}

void 
startSound(int rate, const void* d, u32 bytes, u8 ch, u8 vol, u8 pan, u8 pcm16bit) 
{
	SChanReg *schan = SCHANREG + ch;

	POWERREG->pcr |= 1<<POWER_SOUND;
	SNDREG->cr.ctl = Sndena | 0x7F;

	schan->tmr = SCHAN_FREQ(rate);
	schan->src = (ulong)d;
	schan->wlen = bytes >> 2;
	schan->cr.vol = vol;
	schan->cr.pan = pan;
	schan->cr.ctl |= SCena | SC1shot | (pcm16bit? SCpcm16bit: SCpcm8bit);
}

int
getFreeSoundChannel(void)
{
	int i;
	for (i=0; i<NSChannels; i++) {
		if (((SCHANREG+i)->cr.ctl & SCena) == 0)
			return i;
	}
	return -1;
}

// TODO use dma
void
vblankaudio(void)
{
	u32 i;
	TransferSound *snd = IPC->soundData;

	IPC->soundData = 0;

	if (snd)
	for (i=0; i<snd->count; i++) {
		int chan = getFreeSoundChannel();
		if (chan >= 0)
			startSound(snd->d[i].rate, snd->d[i].data, snd->d[i].len, chan, snd->d[i].vol, snd->d[i].pan, snd->d[i].fmt);
	}

	// intrclear(VBLANKbit, 0);
}

