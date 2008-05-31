#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "dat.h"
#include "fns.h"
#include "nds.h"

#include "../wifi.h"
#include "wifi.h"

/* So we can initialize our own data section and bss */
extern char bdata[];
extern char edata[];
extern char end[];

int ndstype;

int
nbfifoput(ulong cmd, ulong v)
{
	if(FIFOREG->ctl & FifoTfull)
		return 0;
	FIFOREG->send = (cmd|v<<Fcmdwidth);
	return 1;
}

void
fifoput(ulong cmd, ulong v)
{
	FIFOREG->send = (cmd|v<<Fcmdwidth);
}

void
nds_fifo_send(ulong v)
{
	if(FIFOREG->ctl & FifoTfull)
		return;
	FIFOREG->send = v;
}

void
fiforecvintr(void)
{
	ulong v, vv;
	uchar buf[1];

	while(!(FIFOREG->ctl & FifoRempty)) {
		vv = FIFOREG->recv;
		v = vv>>Fcmdwidth;
		switch(vv&Fcmdmask) {
		case F9brightness:
			read_firmware(FWconsoletype, buf, sizeof buf);
			ndstype = buf[0];
			if(ndstype == Dslite || ndstype == Ds)
				power_write(POWER_BACKLIGHT, v&Brightmask);
			break;
		case F9poweroff:
			power_write(POWER_CONTROL, POWER0_SYSTEM_POWER);
			break;
		case F9reboot:
			swiSoftReset();	// TODO: doesn't work
			break;
		case F9leds:
			power_write(POWER_CONTROL, v); // BUG: messes bligth bits
			break;
		case F9getrtc:
			*(ulong*)v = nds_get_time7();
			break;
		case F9setrtc:
			nds_set_time7(*(ulong*)v);
			break;
		}
	}

	intrclear(FRECVbit, 0);
}

enum
{
	MaxRetry = 5,
	MaxRange = 30,
};

void vblankintr(void);
void vblankaudio(void);

int 
main(void)
{
	int16 dmax;
	u8 err;
	
	INTREG->ime = 0;

	memset(edata, 0, end-edata); 		/* clear the BSS */

	read_firmware(0x03FE00, PersonalData, sizeof(PersonalData));
	POWERREG->pcr |= 1<<POWER_SOUND;
	SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7F);

	/* dummy read to enable the touchpad PENIRQ */
	dmax = MaxRetry; err = MaxRange;
	readtsc(TscgetX, &dmax, &err);

	wifi_init();
	
	trapinit();

	FIFOREG->ctl = (FifoRirq|Fifoenable|FifoTflush);
	intrenable(FRECVbit, fiforecvintr, 0);

	intrenable(VBLANKbit, vblankintr, 0);

	intrenable(TIMER0bit, wifi_timer_handler, 0);
	intrenable(WIFIbit, wifi_interrupt, 0);

	// keep the ARM7 out of main RAM
	while (1)
		swiWaitForVBlank();
	return 0;
}

void
vblankintr(void)
{
	static int heartbeat = 0;
	touchPosition tp = {0,0,0,0,0, 0};
	ulong bst, cbst, bup, bdown;
	static ulong obst, opx, opy;
	int i;
	int t1, t2;
#ifdef notyet
	uint32 temp;
	ushort batt, aux;
	ushort x=0, y=0, xpx=0, ypx=0, z1=0, z2=0;
#endif

	heartbeat++;

	/* check buttons state */
	bst = KEYREG->in & Btn9msk;
	bst |= (KEYREG->xy & Btn7msk) << (Xbtn-Xbtn7);

	if(~bst & 1<<Pdown) {
		touchReadXY(&tp);
		if (opx != tp.px || opy != tp.py)
			nbfifoput(F7mousedown, tp.px|tp.py<<8|(tp.z1+tp.z2)<<16);
		opx = tp.px;
		opy = tp.py;
	} else if(~obst & 1<<Pdown) {
		nbfifoput(F7mouseup, 0);
	}

	cbst = bst^obst;
	bdown = bup = 0;
	for(i = 0; cbst && i < Maxbtns; i++) {
		if(cbst & (1<<i)) {
			if(bst & (1<<i))
				bup |= (1<<i);
			else
				bdown |= (1<<i);
			cbst &= ~(1<<i);
		}
	}
	obst = bst;

	/* skip bogus keypresses at start */
	if(heartbeat == 1)
		return;

	if(bup)
		nbfifoput(F7keyup, bup);
	if(bdown)
		nbfifoput(F7keydown, bdown);

	vblankaudio();

/*
	if((press^lastpress) & Pendown) {
		lastpress = press;
		press |= Pendown;
	} else {
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
	}
*/
	
	// Read the temperature
//	batt = touchRead(Tscgetbattery);
//	aux  = touchRead(Tscgetaux);
//	temp = touchReadTemperature(&t1, &t2);
//	if(heartbeat % 180 == 0)
//		 print("batt %d aux %d temp %d\n", batt, aux, temp);

	intrclear(VBLANKbit, 0);
}

void 
startSound(int sampleRate, const void* data, u32 bytes, u8 channel, u8 vol,  u8 pan, u8 format) 
{
	SCHANNEL_TIMER(channel) = SOUND_FREQ(sampleRate);
	SCHANNEL_SOURCE(channel) = (u32)data;
	SCHANNEL_LENGTH(channel) = bytes >> 2 ;
	SCHANNEL_CR(channel)     = SCHANNEL_ENABLE | SOUND_ONE_SHOT | SOUND_VOL(vol) | SOUND_PAN(pan) | (format==1?SOUND_8BIT:SOUND_16BIT);
}

s32 
getFreeSoundChannel(void)
{
	int i;
	for (i=0; i<16; i++) {
		if ( (SCHANNEL_CR(i) & SCHANNEL_ENABLE) == 0 ) return i;
	}
	return -1;
}

void
vblankaudio(void)
{
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

	// intrclear(VBLANKbit, 0);
}

