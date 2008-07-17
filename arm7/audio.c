/* 
 * audio code based on libnds and dslinux audio drivers
 * using gbatek.txt as source of documentation
 */

#include <u.h>
#include "../mem.h"
#include "../io.h"
#include "dat.h"
#include "fns.h"
#include "audio.h"
#include "spi.h"

// TODO use SPIREG from io.h
static void 
pm_setamp(uchar control) 
{
	busywait();
	SPIREG->ctl = Spiena | SpiDevpower | Spi1mhz | Spicont;
	SPIREG->data = PM_AMP_OFFSET;
	busywait();
	SPIREG->ctl = Spiena | SpiDevpower | Spi1mhz;
	SPIREG->data = control;
}

static uchar 
mic_readdata(void) {
	ushort res[2];

	busywait();
	SPIREG->ctl = Spiena | SpiDevmic | Spi2mhz | Spicont;
	SPIREG->data = 0xEC;	// Touchscreen cmd format for AUX
	busywait();
	SPIREG->data = 0x00;
	busywait();
	res[1] = SPIREG->data;
	SPIREG->ctl = Spiena | SpiDevtouch | Spi2mhz;
	SPIREG->data = 0x00;
	busywait();
	res[0] = SPIREG->data;

	return (((res[1] & 0x7F) << 1) | ((res[0] >> 7) & 0x01));
}

static uchar *micbuf = 0;
static int micbuflen = 0;
static int curlen = 0;

static void
clock1intr(void*)
{
	if(micbuf && micbuflen > 0) {
		*micbuf++ = mic_readdata() ^ 0x80;
		--micbuflen;
		curlen++;
	}
	intrclear(TIMERAUDIObit, 0);
}

void 
startrecording(uchar* buffer, int length) 
{
	TimerReg *t = TIMERREG + AUDIOtimer;

	micbuf = buffer;
	micbuflen = length;
	curlen = 0;
	pm_setamp(PM_AMP_ON);
	
	// Setup a 16kHz timer
	t->data = TIMER_BASE(Tmrdiv1) / 16000;
	t->ctl = Tmrena | Tmrdiv1 | Tmrirq;
	intrenable(TIMERAUDIObit, clock1intr, 0);
}

int 
stoprecording(void) 
{
	TimerReg *t = TIMERREG + AUDIOtimer;

	t->ctl &= ~Tmrena;
	intrmask(TIMERAUDIObit, 0);

	pm_setamp(PM_AMP_OFF);
	micbuf = 0;
	return curlen;
}

static void 
startSound(int hz, const void* d, ulong bytes, uchar ch, uchar vol, uchar pan, uchar pcm16bit) 
{
	SChanReg *schan = SCHANREG + ch;

	POWERREG->pcr |= 1<<POWER_SOUND;
	SNDREG->cr.ctl = Sndena | Maxvol;

	schan->tmr = SCHAN_BASE / hz;
	schan->src = (ulong)d;
	schan->wlen = bytes >> 2;
	schan->cr.vol = vol;
	schan->cr.pan = pan;
	schan->cr.ctl |= SCena | SC1shot | (pcm16bit? SCpcm16bit: SCpcm8bit);
}

static int
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
	ulong i;
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

