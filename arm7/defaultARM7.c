#include <u.h>
#include "../mem.h"
#include "nds.h"

s32 getFreeSoundChannel();
void VblankHandler(void);
void VcountHandler();

/* need to figure out the interrupt vector g __inter*/
int 
main(int argc, char ** argv) 
{
//	readFirmware(0x03FE00,PersonalData,sizeof(PersonalData));
	poweron(POWER_SOUND);
	SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7F);
	for(;;) {
		VblankHandler();
		VcountHandler();
	}
	irqInit();
	initclkirq();
	irqset(IRQ_VBLANK, VblankHandler);
	setytrig(80);
	irqset(IRQ_VCOUNT, VcountHandler);
	irqen( IRQ_VBLANK | IRQ_VCOUNT);
	while (1)
		swiWaitForVBlank();
}

void 
startSound(int sampleRate, const void* data, u32 bytes, u8 channel, u8 vol,  u8 pan, u8 format) 
{
	SCHANNEL_TIMER(channel)  = SOUND_FREQ(sampleRate);
	SCHANNEL_SOURCE(channel) = (u32)data;
	SCHANNEL_LENGTH(channel) = bytes >> 2 ;
	SCHANNEL_CR(channel)     = SCHANNEL_ENABLE | SOUND_ONE_SHOT | SOUND_VOL(vol) | SOUND_PAN(pan) | (format==1?SOUND_8BIT:SOUND_16BIT);
}
s32 
getFreeSoundChannel()
{
	int i;
	for (i=0; i<16; i++) {
		if ( (SCHANNEL_CR(i) & SCHANNEL_ENABLE) == 0 ) return i;
	}
	return -1;
}


void
VcountHandler()
{
	touchPosition tp = {0,0,0,0,0, 0};
	static int lastpress = -1;
	uint16 press=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0;
	press = REG_KEYXY;
	if (!((press^lastpress)&Pendown)) {
		touchReadXY(&tp);
		if ( tp.x == 0 || tp.y == 0 ) {
			press |= Pendown;
			lastpress = press;
		} else {
			x = tp.x;
			y = tp.y;
			xpx = tp.px;
			ypx = tp.py;
			z1 = tp.z1;
			z2 = tp.z2;
		}
	} else {
		lastpress = press;
		press |= Pendown;
	}
	IPC->touchX			= x; // x/14-24
	IPC->touchY			= y; // y/18-12
	IPC->touchXpx		= xpx;
	IPC->touchYpx		= ypx;
	IPC->touchZ1		= z1;
	IPC->touchZ2		= z2;
	IPC->buttons		= press;
}

void
VblankHandler(void) {
	u32 i;
	TransferSound *snd = IPC->soundData;
	IPC->soundData = 0;
	if (snd) {
		for (i=0; i<snd->count; i++) {
			s32 chan = getFreeSoundChannel();
			if (chan >= 0) {
				startSound(snd->data[i].rate, snd->data[i].data, snd->data[i].len, chan, snd->data[i].vol, snd->data[i].pan, snd->data[i].format);
			}
		}
	}
}
