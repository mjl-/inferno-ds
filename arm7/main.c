#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "dat.h"
#include "fns.h"
#include "spi.h"
#include "rtc.h"
#include "jtypes.h"
#include "touch.h"

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
fiforecvintr(void*)
{
	ulong v, vv;
	nds_tx_packet *tx_packet;

	while(!(FIFOREG->ctl & FifoRempty)) {
		vv = FIFOREG->recv;
		v = vv>>Fcmdwidth;
		switch(vv&Fcmdmask) {
		case F9SysCtl:
			if(0)print("F9SysCtl %lux %lux %lux %lux %lux\n",
				v, v & SysCtlbright, v & SysCtlpoweroff, v & SysCtlreboot, v & SysCtlleds);
				
			if (v & SysCtlbright){
				uchar ndstype[1], bright;
				
				bright = v>>SysCtlsz;
				read_firmware(FWconsoletype, ndstype, sizeof ndstype);
				if(ndstype[0] == Dslite || ndstype[0] == Ds)
					power_write(POWER_BACKLIGHT, bright&Brightmask);
			}
			if (v & SysCtlpoweroff)
				power_write(POWER_CONTROL, POWER0_SYSTEM_POWER);
			if (v & SysCtlreboot)
				swiSoftReset();	// TODO: doesn't work
			if (v & SysCtlleds)
				power_write(POWER_CONTROL, v); // BUG: messes bligth bits
			break;
			
		case F9getrtc:
			(*(ulong*)v) = nds_get_time7();
			break;
		case F9setrtc:
			nds_set_time7(*(ulong*)v);
			break;

		case F9WFmacqry:
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
			break;
		case F9WFtxpkt:
			tx_packet = (nds_tx_packet*)v;
			wifi_send_ether_packet(tx_packet->len, tx_packet->data);
			tx_packet = nil;
			break;
		case F9WFctl:
			if(0)print("F9WFctl %lux %lux %lux %lux %lux\n",
				v, v & WFCtlopen, v & WFCtlclose, v & WFCtlscan, v & WFCtlstats);
			
			if(v & WFCtlopen)
				wifi_open();
			if(v & WFCtlclose)
				wifi_close();
			if(v & WFCtlscan)
				wifi_start_scan();
			if(v & WFCtlstats)
				wifi_stats_query();
			break;
				
		default:
			print("fiforecv7: unhandled msg: %lux\n", vv);
			break;
		}
	}

	intrclear(FRECVbit, 0);
}

void
vblankintr(void*)
{
	int i;
	static int hbt = 0;
	touchPosition tp = {0,0,0,0,0, 0};
	ulong bst, cbst, bup, bdown;
	static ulong obst, opx, opy;

	hbt++;

	/* check buttons state */
	bst = KEYREG->in & Btn9msk;
	bst |= (KEYREG->xy & Btn7msk) << (Xbtn-Xbtn7);

	/* skip bogus keypresses at start */
	if (hbt == 1)
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
	IPC->batt = touchRead(Tscgetbattery);
	IPC->temp = touchReadTemperature(&IPC->td1, &IPC->td2);
	//if(hbt%120 == 0) print("batt %d aux %d temp %d\n", IPC->batt, IPC->aux, IPC->temp);

	intrclear(VBLANKbit, 0);
}

void
ipcsyncintr(void*)
{
	print("ipcsyncintr\n");
	intrclear(IPCSYNCbit, 0);	
}

enum {
	MaxRetry = 5,
	MaxRange = 30,
};

int 
main(void)
{
	short dmax;
	uchar err;
	
	INTREG->ime = 0;

	memset(edata, 0, end-edata); 		/* clear the BSS */

	read_firmware(0x03FE00, UserInfoAddr, sizeof(UserInfoAddr));

	/* dummy read to enable the touchpad PENIRQ */
	dmax = MaxRetry; err = MaxRange;
	readtsc(TscgetX, &dmax, &err);

	wifi_init();
	
	trapinit();

	FIFOREG->ctl = (FifoRirq|Fifoenable|FifoTflush);
	intrenable(FRECVbit, fiforecvintr, 0);

	intrenable(VBLANKbit, vblankintr, 0);

	intrenable(TIMERWIFIbit, wifi_timer_handler, 0);
	intrenable(WIFIbit, wifi_interrupt, 0);
	IPCREG->ctl |= Ipcirqena;
	intrenable(IPCSYNCbit, ipcsyncintr, 0);

	// keep the ARM7 out of main RAM
	while (1)
		swiWaitForVBlank();
	return 0;
}

