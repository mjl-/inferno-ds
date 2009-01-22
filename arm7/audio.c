/* 
 * audio code based on libnds and dslinux audio drivers
 * using gbatek.txt as source of documentation
 */

#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "fns.h"
#include "audio.h"
#include "spi.h"

static ushort
mic_read(ushort sfmt) {
	ushort res[2];

	busywait();
	SPIREG->ctl = Spiena|SpiDevtouch|Spi2mhz|Spicont;
	SPIREG->data = sfmt;	
	busywait();
	SPIREG->data = 0x00;
	busywait();
	res[1] = SPIREG->data;
	SPIREG->ctl = Spiena|SpiDevtouch|Spi2mhz;
	SPIREG->data = 0x00;
	busywait();
	res[0] = SPIREG->data;

	if(sfmt == Tscgetmic8)
		return (((res[1] & 0x7F) << 1) | ((res[0] >> 7) & 0x01));
	else
		return (((res[1] & 0x7F) << 5) | ((res[0] >> 3) & 0x1F));

}

enum {
	AUDIOtimer = 0,
	TIMERAUDIObit = TIMER0bit+AUDIOtimer,
};

static int nrs = 0;

static void
recintr(void *a)
{	
	TxSound *snd = a;

	//print("snd7 %lx %lx %lx\n", (ulong)snd, (ulong)snd->d, snd->n);
	if(snd->d && nrs++ < snd->n){
		if(snd->fmt)
			((ushort*)snd->d)[nrs] = mic_read(Tscgetmic12);
		else
			((uchar*)snd->d)[nrs] = mic_read(Tscgetmic8);
	}else
		audiorec(snd, 0);

	intrclear(TIMERAUDIObit, 0);
}

int 
audiorec(TxSound *snd, int on)
{
	TimerReg *t = TMRREG + AUDIOtimer;

	if(!snd)
		return 0;

	if(on){
		nrs = 0;
		t->data = TIMER_BASE(Tmrdiv1) / snd->rate;
		t->ctl = Tmrena | Tmrdiv1 | Tmrirq;
		intrenable(TIMERAUDIObit, recintr, snd, 0);
	}else{
		t->ctl &= ~Tmrena;
		intrmask(TIMERAUDIObit, 0);

		snd->d = nil;
		return nrs;
	}
}

int
audioplay(TxSound *snd, int on)
{
	SChanReg *schan;
	
	if(!snd || snd->chan > NSChannels)
		return 0;

	schan = SCHANREG + snd->chan;
	schan->rpt = 0;
	schan->tmr = SCHAN_BASE / (int)snd->rate;
	schan->src = (ulong) snd->d;
	schan->wlen = snd->fmt? snd->n>>2: snd->n>>1;
	schan->cr.vol = snd->vol;
	schan->cr.pan = snd->pan;
	schan->cr.ctl |= SCena | SCrep1shot | (snd->fmt? SCpcm16bit: SCpcm8bit);
	return 1;
}

void
audiopower(int dir, int on)
{
	uchar pwr;

	pwr=power_read(POWER_CONTROL);
	switch(dir){	/* audio in/out */
	default:
		break;
	case F9Aupowerout:
		if(on){
			POWERREG->pcr |= 1<<POWER_SOUND;
			SNDREG->cr.ctl = Sndena | Maxvol;
			SNDREG->bias = 0x200;
			power_write(POWER_CONTROL, pwr & ~POWER_SOUND_SPK);
		}else{
			POWERREG->pcr &= ~(1<<POWER_SOUND);
			SNDREG->cr.ctl &= ~Sndena;
			power_write(POWER_CONTROL, pwr | POWER_SOUND_SPK);
		}
		break;
	case F9Aupowerin:
		if(on){
			power_write(POWER_CONTROL, pwr | POWER_SOUND_AMP);

			power_write(POWER_MIC_AMPL, POWER_MIC_AMPLON);
			power_write(POWER_MIC_GAIN, POWER_MIC_GAIN_20);
		}else{
			power_write(POWER_CONTROL, pwr & ~POWER_SOUND_AMP);

			power_write(POWER_MIC_AMPL, POWER_MIC_AMPLOFF);
			power_write(POWER_MIC_GAIN, POWER_MIC_GAIN_20);
		}
		break;
	}
}
