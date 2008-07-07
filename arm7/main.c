#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "dat.h"
#include "fns.h"
#include "nds.h"

#include "wifi.h"

/* So we can initialize our own data section and bss */
extern char bdata[];
extern char edata[];
extern char end[];

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
	uchar ndstype[1];

	while(!(FIFOREG->ctl & FifoRempty)) {
		vv = FIFOREG->recv;
		v = vv>>Fcmdwidth;
		switch(vv&Fcmdmask) {
		case F9brightness:
			read_firmware(FWconsoletype, ndstype, sizeof ndstype);
			if(ndstype[0] == Dslite || ndstype[0] == Ds)
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
			(*(ulong*)v) = nds_get_time7();
			break;
		case F9setrtc:
			nds_set_time7(*(ulong*)v);
			break;

		case F9WFmacqry:
			// duplicated code: wifi_mac_query(void)
			memmove((void*)v, wifi_data.MacAddr, 6);
			break;
		case F9WFstats:
			wifi_data.stats = (volatile ulong *)v;
			break;
		case F9WFapquery:
			wifi_data.aplist = (Wifi_AccessPoint*)v;
			break;
		case F9WFrxpkt:
			rx_packet = (nds_rx_packet*)v;

if(1){
	wifi_open();
	wifi_start_scan();
}

		break;
		case F9WFtxpacket:
			break;

		default:
			print("fiforecv7: unhandled msg: %lux\n", vv);
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

void
vblankintr(void)
{
	int i;
	static int heartbeat = 0;
	touchPosition tp = {0,0,0,0,0, 0};
	ulong bst, cbst, bup, bdown;
	static ulong obst, opx, opy;
	int t1, t2;

	heartbeat++;

	/* check buttons state */
	bst = KEYREG->in & Btn9msk;
	bst |= (KEYREG->xy & Btn7msk) << (Xbtn-Xbtn7);

	/* skip bogus keypresses at start */
	if (heartbeat == 1)
		obst = bst;
	
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

	if(bup)
		nbfifoput(F7keyup, bup);
	if(bdown)
		nbfifoput(F7keydown, bdown);

	vblankaudio();
	
	// Read the temperature
	IPC->aux = touchRead(Tscgetaux);
	IPC->battery  = touchRead(Tscgetbattery);
	IPC->temperature = touchReadTemperature(&t1, &t2);;
	//print("batt %d aux %d temp %d\n", IPC->battery, IPC->aux, IPC->temperature);

	intrclear(VBLANKbit, 0);
}

void
arm7intr(void)
{
	print("arm7intr\n");
	intrclear(ARM7bit, 0);	
}

int 
main(void)
{
	int16 dmax;
	u8 err;
	
	INTREG->ime = 0;

	memset(edata, 0, end-edata); 		/* clear the BSS */

	read_firmware(0x03FE00, PersonalData, sizeof(PersonalData));

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
	intrenable(ARM7bit, arm7intr, 0);

	// keep the ARM7 out of main RAM
	while (1)
		swiWaitForVBlank();
	return 0;
}

