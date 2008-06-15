/* 
 * audio code based on libnds and dslinux audio drivers
 * using gbatek.txt as source of documentation
 */

#include <u.h>
#include "../mem.h"
#include "../io.h"
#include "nds.h"
#include "fns.h"


// TODO use SPIREG from io.h
void 
PM_SetAmp(u8 control) 
{
	busywait();
	REG_SPICNT = Spien | SPI_DEVICE_POWER | SPI_BAUD_1MHz | Spicont;
	REG_SPIDATA = PM_AMP_OFFSET;
	busywait();
	REG_SPICNT = Spien | SPI_DEVICE_POWER | SPI_BAUD_1MHz;
	REG_SPIDATA = control;
}

u8 
MIC_ReadData(void) {
	u16 res, res2;
	busywait();
	REG_SPICNT = Spien | SPI_DEVICE_MICROPHONE | Spi2mhz | Spicont;
	REG_SPIDATA = 0xEC;	// Touchscreen cmd format for AUX
	busywait();
	REG_SPIDATA = 0x00;
	busywait();
	res = REG_SPIDATA;
	REG_SPICNT = Spien | Spitouch | Spi2mhz;
	REG_SPIDATA = 0x00;
	busywait();
	res2 = REG_SPIDATA;
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
	TimerReg *t = TIMERREG + 1;

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
	schan->src = (u32)d;
	schan->len = bytes >> 2;
	schan->cr.vol = vol;
	schan->cr.pan = pan;
	schan->cr.ctl |= SCena | SC1shot | (pcm16bit? SCpcm16bit: SCpcm8bit);
}

s32 
getFreeSoundChannel(void)
{
	int i;
	for (i=0; i<16; i++) {
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
		s32 chan = getFreeSoundChannel();
		if (chan >= 0)
			startSound(snd->d[i].rate, snd->d[i].data, snd->d[i].len, chan, snd->d[i].vol, snd->d[i].pan, snd->d[i].fmt);
	}

	// intrclear(VBLANKbit, 0);
}

