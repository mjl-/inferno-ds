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
recintr(void*)
{
	if(micbuf && micbuflen > 0) {
//		print("%x ", *micbuf);
		*micbuf++ = mic_readdata() ^ 0x80;
		--micbuflen;
		curlen++;
	}
	intrclear(TIMERAUDIObit, 0);
}

void 
startrec(TxSound *snd) 
{
	TimerReg *t = TIMERREG + AUDIOtimer;

	micbuf = (uchar*)snd->data;
	micbuflen = snd->len;
	curlen = 0;
	pm_setamp(PM_AMP_ON);
	
	t->data = TIMER_BASE(Tmrdiv1) / snd->rate;
	t->ctl = Tmrena | Tmrdiv1 | Tmrirq;
	intrenable(TIMERAUDIObit, recintr, 0);
}

int 
stoprec(TxSound *snd) 
{
	TimerReg *t = TIMERREG + AUDIOtimer;

	USED(snd);
	t->ctl &= ~Tmrena;
	intrmask(TIMERAUDIObit, 0);

	pm_setamp(PM_AMP_OFF);
	micbuf = 0;
	return curlen;
}

void 
playsound(TxSound *snd)
{
	int ch;
	SChanReg *schan;

	SNDREG->cr.ctl = Sndena | Maxvol;
	SNDREG->bias = 0x200;

	for (ch=0; ch < snd->chans; ch++){
		schan = SCHANREG + ch;

		schan->rpt = 0;
		schan->tmr = SCHAN_BASE / (int)snd->rate;
		schan->src = (ulong) snd->data;
		schan->wlen = snd->fmt? snd->len>>2: snd->len>>1;
		schan->cr.vol = snd->vol;
		schan->cr.pan = snd->pan;
		schan->cr.ctl |= SCena | SCrep1shot | (snd->fmt? SCpcm16bit: SCpcm8bit);
	}
}
