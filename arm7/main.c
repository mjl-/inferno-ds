#include <u.h>
#include "../mem.h"
#include <kern.h>
#include "dat.h"
#include "fns.h"
#include "nds.h"
#include "../fifo.h"

s32 getFreeSoundChannel(void);
void vblankintr(void);
void vblankaudio(void);
void poweron(int);

/* So we can initialize our own data section and bss */
extern char bdata[];
extern char edata[];
extern char end[];

int ndstype;

void 
memtest(char *a, char c, int n, int read){
	int i;

	for (i=0; i < n; i++){
		if (read)
			c = a[i];
		else{
			a[i] += 1;
			a[i] &= 0x0F;
			a[i] |= c;
		}
	}
}

enum
{
	MaxRetry = 5,
	MaxRange = 30,
};


void
outsh(ulong addr, ushort v)
{
	*(ushort*)addr = v;
}

void
outl(ulong addr, ulong v)
{
	*(ulong*)addr = v;
}

ushort
insh(ulong addr)
{
	return *(ushort*)addr;
}

ulong
inl(ulong addr)
{
	return *(ulong*)addr;
}

ulong
fifoget(void)
{
	return inl(Fiforecv);
}

void
nbfifoput(ulong cmd, ulong v)
{
	if(insh(Fifoctl) & FifoTfull)
		return;
	outl(Fifosend, cmd|v<<Fcmdwidth);
}


void
fifoput(ulong cmd, ulong v)
{
	outl(Fifosend, cmd|v<<Fcmdwidth);
}

void
fiforecvintr(void)
{
	ulong v, vv;
	uchar buf[1];

	while(!(insh(Fifoctl) & FifoRempty)) {
		vv = fifoget();
		v = vv>>Fcmdwidth;
		switch(vv&Fcmdmask) {
		case F9brightness:
			readFirmware(FWconsoletype, buf, sizeof buf);
			ndstype = buf[0];
			if(ndstype == Dslite)
				brightset(v);
			break;
		}
	}

	REG_IF = IRQ_FIFO_NOT_EMPTY;
}

void
fifoinit(void)
{
	outsh(Fifoctl, FifoRirq|Fifoenable|FifoTflush);
	irqset(IRQ_FIFO_NOT_EMPTY, fiforecvintr);
}


void
trapinit(void)
{
	int16 dmax;
	u8 err;
	REG_IME = 0;

	readFirmware(0x03FE00,PersonalData,sizeof(PersonalData));
	poweron(POWER_SOUND);
	SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7F);

	/* dummy read to enable the touchpad PENIRQ */
	dmax = MaxRetry; err = MaxRange;
	readtsc(TscgetX, &dmax, &err);
	
	irqInit();
	initclkirq();

	fifoinit();

	irqset(IRQ_VBLANK, vblankintr);
	irqen(IRQ_VBLANK);

/*
	setytrig(80);
	irqset(IRQ_VCOUNT, VcountHandler);
	irqen(IRQ_VCOUNT);
*/

	REG_IME = 1;
}


#define DMTEST if(0)memtest
int 
main(int argc, char ** argv)
{
	USED(argc, argv);

	memset(edata, 0, end-edata); 		/* clear the BSS */

	DMTEST((char*)(IPC), 0x10, 1, 0);

	trapinit();

	DMTEST((char*)(IPC), 0x20, 1, 0);

	// keep the ARM7 out of main RAM
	while (1){
		swiWaitForVBlank();

//		swiDelay(50000);
//		DMTEST((char*)(IPC), 0x30, 1, 0);
	}
	return 0;
}

void
vblankintr(void)
{
	static int heartbeat = 0;
	touchPosition tp = {0,0,0,0,0, 0};
	static ushort lastxypress = ~0;
	static ushort lastbpress = ~0;
	ushort xypress, bpress;
	int i;
	ulong mask, changed, up, down;
#ifdef notyet
	uint8 ct[sizeof(IPC->time.curtime)];
	ushort x=0, y=0, xpx=0, ypx=0, z1=0, z2=0, batt, aux;
	int t1, t2;
	uint32 temp;
#endif

	heartbeat++;

	xypress = REG_KEYXY;
	if(~xypress & Pendown) {
		touchReadXY(&tp);
		nbfifoput(F7mousedown, tp.px|tp.py<<8);
	} else if(~lastxypress & Pendown) {
		nbfifoput(F7mouseup, 0);
	}
	lastxypress = xypress;

	bpress = REG_KEYINPUT;
	changed = bpress^lastbpress;
	mask = 1;
	down = up = 0;
	for(i = 0; changed && i < 10; i++) {
		if(mask & changed) {
			if(bpress&(1<<i))
				up |= 1<<i;
			else
				down |= 1<<i;
			changed &= ~(1<<i);
		}
		mask <<= 1;
	}
	lastbpress = bpress;
	if(up)
		nbfifoput(F7keyup, up);
	if(down)
		nbfifoput(F7keydown, down);

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
	
	// Read the time
/*
	rtcGetTime((uint8 *)ct);
	BCDToInteger((uint8 *)&(ct[1]), 7);

	// Read the temperature
	batt = touchRead(Tscgetbattery);
	aux  = touchRead(Tscgetaux);
	temp = touchReadTemperature(&t1, &t2);
*/

	// Update the IPC struct
/*
	IPC->heartbeat 		= heartbeat;
	IPC->touchX			= x; // x/14-24
	IPC->touchY			= y; // y/18-12
	IPC->touchXpx		= xpx;
	IPC->touchYpx		= ypx;
	IPC->touchZ1		= z1;
	IPC->touchZ2		= z2;
	IPC->buttons		= press;
	IPC->battery		= batt;
	IPC->aux			= aux;
	IPC->tdiode1		= t1;
	IPC->tdiode2		= t2;
	IPC->temperature	= temp;
*/
	
/*
	for(i=0; i<sizeof(ct); i++) {
		IPC->time.curtime[i] = ct[i];
	}

	DMTEST((char*)(IPC), 0x60, 1, 0);
*/
	
	// ack. ints
	REG_IF = IRQ_VBLANK;
	*(ulong*)IRQCHECK7 = IRQ_VBLANK;
}

void 
startSound(int sampleRate, const void* data, u32 bytes, u8 channel, u8 vol,  u8 pan, u8 format) 
{
	// TODO: fix freq = - 0x01000000 / sampleRate;
	if (sampleRate == 11127)
		SCHANNEL_TIMER(channel) = SOUND_FREQ(11127);

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

	DMTEST((char*)(IPC), 0x50, 1, 0);
	
	// ack. ints
	REG_IF = IRQ_VBLANK;
	*(ulong*)IRQCHECK7 = IRQ_VBLANK;
}

