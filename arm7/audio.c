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
#include "touch.h"

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
mic_auxread(ushort sfmt) {
	ushort res[2];

	busywait();
	SPIREG->ctl = Spiena | SpiDevtouch | Spi2mhz | Spicont;
	SPIREG->data = sfmt;	
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

static int nrs = 0;

static void
recintr(void *a)
{	
	TxSound *snd = a;

	if(snd->d && nrs++ < snd->n){
		snd->d[nrs] = (char) mic_auxread(Tscgetmic8) ^ 0x80;
		if (0 && nrs % 1024 == 0) print("%x[%d]\n", snd->d, nrs);
	}else
		stoprec(snd);

	intrclear(TIMERAUDIObit, 0);
}

void 
startrec(TxSound *snd) 
{
	TimerReg *t = TMRREG + AUDIOtimer;

	pm_setamp(PM_AMP_ON);
	
	nrs = 0;
	t->data = TIMER_BASE(Tmrdiv1) / snd->rate;
	t->ctl = Tmrena | Tmrdiv1 | Tmrirq;
	intrenable(TIMERAUDIObit, recintr, snd, 0);
}

int 
stoprec(TxSound *snd) 
{
	TimerReg *t = TMRREG + AUDIOtimer;

	USED(snd);
	t->ctl &= ~Tmrena;
	intrmask(TIMERAUDIObit, 0);

	pm_setamp(PM_AMP_OFF);

	snd->d = nil;
	return nrs;
}

void 
playsound(TxSound *snd)
{
	SChanReg *schan;

	SNDREG->cr.ctl = Sndena | Maxvol;
	SNDREG->bias = 0x200;

	schan = SCHANREG + snd->chan;

	schan->rpt = 0;
	schan->tmr = SCHAN_BASE / (int)snd->rate;
	schan->src = (ulong) snd->d;
	schan->wlen = snd->fmt? snd->n>>2: snd->n>>1;
	schan->cr.vol = snd->vol;
	schan->cr.pan = snd->pan;
	schan->cr.ctl |= SCena | SCrep1shot | (snd->fmt? SCpcm16bit: SCpcm8bit);
}
