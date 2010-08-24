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

	if(snd->inuse && nrs++ < snd->n){
		switch(snd->flags & (AFlagsigned|AFlag8bit)){
		case (AFlag8bit):
			((uchar*)snd->d)[nrs] = mic_read(Tscgetmic8);
			break;
		case (AFlagsigned|AFlag8bit):
			((uchar*)snd->d)[nrs] = mic_read(Tscgetmic8) ^ 0x80;
			break;
		case (AFlagsigned):
			((ushort*)snd->d)[nrs] = (mic_read(Tscgetmic12) - 2048) << 4; // ^ 0x8000;
			break;
		default:
			((ushort*)snd->d)[nrs] = mic_read(Tscgetmic12);
			break;
		}
	}else
		audiorec(snd, 0);

	intrclear(TIMERAUDIObit, 0);
}

int 
audiorec(TxSound *snd, int on)
{
	TimerReg *t = TMRREG + AUDIOtimer;

	if(!snd || snd->d == nil)
		return 0;

	if(on){
		nrs = 0;
		snd->inuse = 1;

		t->data = TIMER_BASE(Tmrdiv1) / snd->rate;
		t->ctl = Tmrena | Tmrdiv1 | Tmrirq;
		intrenable(TIMERAUDIObit, recintr, snd, 0);
	}else{
		t->ctl &= ~Tmrena;
		intrmask(TIMERAUDIObit, 0);
		
		snd->inuse = 0;
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
	schan->tmr = SCHAN_BASE / snd->rate;
	schan->src = (ulong) snd->d;
	schan->wlen = snd->flags & AFlag8bit? snd->n>>1: snd->n>>2;
	schan->cr.vol = snd->vol;
	schan->cr.pan = snd->pan;
	schan->cr.ctl |= SCena | SCrep1shot | (snd->flags & AFlag8bit? SCpcm8bit: SCpcm16bit);
	return 1;
}

void
audiopower(int dir, int l)
{
	uchar pwr;

	pwr=power_read(POWER_CONTROL);
	switch(dir){	/* audio in/out */
	default:
		break;
	case F9Aupowerout:
		if(l){
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
		if(l){
			power_write(POWER_CONTROL, pwr | POWER_SOUND_AMP);

			power_write(POWER_MIC_AMPL, POWER_MIC_AMPLON);
			power_write(POWER_MIC_GAIN, l);
		}else{
			power_write(POWER_CONTROL, pwr & ~POWER_SOUND_AMP);

			power_write(POWER_MIC_AMPL, POWER_MIC_AMPLOFF);
			power_write(POWER_MIC_GAIN, l);
		}
		break;
	}
}
