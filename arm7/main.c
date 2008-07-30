#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "dat.h"
#include "fns.h"
#include "spi.h"
#include "rtc.h"
#include "audio.h"
#include "touch.h"

#include "wifi.h"

/* So we can initialize our own data section and bss */
extern char bdata[];
extern char edata[];
extern char end[];

int
nbfifoput(ulong cmd, ulong data)
{
	if(FIFOREG->ctl & FifoTfull)
		return 0;
	FIFOREG->send = (data<<Fcmdlen|cmd);
	return 1;
}

void
fifoput(ulong cmd, ulong data)
{
	FIFOREG->send = (data<<Fcmdlen|cmd);
}

void
nds_fifo_send(ulong data)
{
	if(FIFOREG->ctl & FifoTfull)
		return;
	FIFOREG->send = data;
}

void
fiforecvintr(void*)
{
	ulong v, vv;
	nds_tx_packet *tx_packet;
	uchar ndstype[1];
	TxSound *snd;

	while(!(FIFOREG->ctl & FifoRempty)) {
		vv = FIFOREG->recv;
		v = vv>>Fcmdlen;
		switch(vv&Fcmdtmask) {
		case F9TSystem:
			if(0)print("F9S %lux\n", v & Fcmdsmask);
			switch(vv&Fcmdsmask){
			case F9Sysbright:
				read_firmware(FWconsoletype, ndstype, sizeof ndstype);
				if(ndstype[0] == Dslite || ndstype[0] == Ds)
					power_write(POWER_BACKLIGHT, v&Brightmask);
				break;
			
			case F9Syspoweroff:
				power_write(POWER_CONTROL, POWER0_SYSTEM_POWER);
				break;
			case F9Sysreboot:
				swiSoftReset();	// TODO: doesn't work
				break;
			case F9Sysleds:
				power_write(POWER_CONTROL, v); // BUG: messes bligth bits
				break;
			case F9Sysrrtc:
				(*(ulong*)v) = nds_get_time7();
				break;
			case F9Syswrtc:
				nds_set_time7(*(ulong*)v);
				break;
			}
			break;
		
		case F9TWifi:
			if(0)print("F9W %lux\n", v & Fcmdsmask);
			switch(vv&Fcmdsmask){
			case F9WFrmac:
				memmove((void*)v, wifi_data.MacAddr, 6);
				break;
			case F9WFwstats:
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
			case F9WFwstate:
				if(v)
					wifi_open();
				else
					wifi_close();
				break;
			case F9WFscan:
				wifi_start_scan();
				break;
			case F9WFstats:
				wifi_stats_query();
				break;
			case F9WFwap:
				Wifi_SetAPMode(v);
				break;
			case F9WFwssid:
				memmove(&wifi_data.ssid[1], (void*)v, sizeof(wifi_data.ssid));
				wifi_data.ssid[0] = strlen(&wifi_data.ssid[1]);
				Wifi_SetSSID(0, 0, 0);
				if(0)print("%d %s\n", wifi_data.ssid[0], &wifi_data.ssid[1]);
				break;
			case F9WFwchan:
				Wifi_RequestChannel(v);
				break;
			case F9WFwwepmode:
				Wifi_SetWepMode(v);
				break;
			}
			break;

		case F9TAudio:
			if(0)print("F9A %lux\n", v & Fcmdsmask);
			switch(vv&Fcmdsmask){
				case F9Auplay:
					snd = (TxSound*)v;
					if (snd != nil)
						playsound(snd);
				break;
				case F9Aurec:
					snd = (TxSound*)v;
					if (snd != nil)
						startrec(snd);
				break;
				case F9Aupower:
					if(v)
						POWERREG->pcr |= 1<<POWER_SOUND;
					else
						POWERREG->pcr &= ~(1<<POWER_SOUND);
				break;

			}
			break;

		default:
			print("F7rx err %lux\n", vv);
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
	TouchPos tp = {0,0,0,0,0,0};
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

	read_firmware(0x03FE00, (ulong*)UINFOMEM, sizeof(UserInfo));

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

