#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "dat.h"
#include "fns.h"
#include "spi.h"
#include "rtc.h"
#include "audio.h"

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

static void
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
			if(0)print("F9S %lux\n", vv & Fcmdsmask);
			switch(vv&Fcmdsmask){
			case F9Sysbright:
				read_firmware(FWconsoletype, ndstype, sizeof ndstype);
				if(ndstype[0] == Dslite || ndstype[0] == Ds)
					power_write(POWER_BACKLIGHT, v);
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
			if(0)print("F9W %lux\n", vv & Fcmdmask);
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
			case F9WFrxdone:
				wifi_rx_q_complete();
				break;
			}
			break;

		case F9TAudio:
			if(0)print("F9A %lux\n", vv & Fcmdsmask);
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

enum {
	MaxRetry = 5,
	MaxRange = 30,
};

static int tscinit = 0;
static int xscale, yscale;
static int xoffset, yoffset;

#define inball(ov, v, d) ((ov - d < v) && (v < ov + d))

static void 
updatetouch(ulong bst)
{
	short x, y, z, px, py;
	static uchar opx = 0, opy = 0;
	UserInfo *pu=UINFOREG;

	if (!tscinit){
		xscale = ((pu->adc.xpx2 - pu->adc.xpx1) << 19) / (pu->adc.x2 - pu->adc.x1);
		yscale = ((pu->adc.ypx2 - pu->adc.ypx1) << 19) / (pu->adc.y2 - pu->adc.y1);
		xoffset = ((pu->adc.x1 + pu->adc.x2) * xscale  - ((pu->adc.xpx1 + pu->adc.xpx2) << 19))/2;
		yoffset = ((pu->adc.y1 + pu->adc.y2) * yscale  - ((pu->adc.ypx1 + pu->adc.ypx2) << 19))/2;
		tscinit=1;
	}

	if(~bst & 1<<Pdown){
		x = touch_read_value(TscgetX, MaxRetry, MaxRange);
		y = touch_read_value(TscgetY, MaxRetry, MaxRange);
		z = touch_read_value(TscgetZ1, MaxRetry, MaxRange);

		px = (x * xscale - xoffset + xscale/2 ) >>19;
		py = (y * yscale - yoffset + yscale/2 ) >>19;

		if(px < 0) px=0;
		if(py < 0) py=0;
		if(px > Scrwidth-1) px=Scrwidth-1;
		if(py > Scrheight-1) py=Scrheight-1;

		if (inball(opx, px, 6) && inball(opy, py, 6))
			nbfifoput(F7mousedown, px|py<<8|z<<16);
		opx = px;
		opy = py;
	} else {
		if (opy != 255)
			nbfifoput(F7mouseup, 0);
		opx = 255;
		opy = 255;
	}
}

static ulong
touch_read_temp(int *t1, int *t2)
{
	*t1 = touch_read_value(Tscgettemp1, MaxRetry, MaxRange);
	*t2 = touch_read_value(Tscgettemp2, MaxRetry, MaxRange);
	return 8490 * (*t2 - *t1) - 273*4096;
}

static void
vblankintr(void*)
{
	int i;
	static int hbt = 0;
	ulong bst, cbst, bup, bdown;
	static ulong obst;

	hbt++;

	if (hbt % 2 == 0){
		/* check buttons state */
		bst = KEYREG->in & Btn9msk;
		bst |= (KEYREG->xy & Btn7msk) << (Xbtn-Xbtn7);
	
		/* skip bogus keypresses at start */
		if (hbt == 2)
			obst = bst;
	
		updatetouch(bst);
		
		cbst = bst^obst;
		obst = bst;
		bup = cbst & bst;
		bdown = cbst & ~bst;

		if(bup)
			nbfifoput(F7keyup, bup);
		if(bdown)
			nbfifoput(F7keydown, bdown);
	
		IPC->batt = touch_read_value(Tscgetbattery, MaxRetry, MaxRange);
		IPC->temp = touch_read_temp(&IPC->td1, &IPC->td2);
		//if(hbt%120 == 0) print("batt %d aux %d temp %d\n", IPC->batt, IPC->aux, IPC->temp);
	}
	
	intrclear(VBLANKbit, 0);
}

int 
main(void)
{
	INTREG->ime = 0;
	memset(edata, 0, end-edata); 		/* clear the BSS */
	read_firmware(0x03FE00, (ulong*)UINFOMEM, sizeof(UserInfo));
	
	/* dummy read to enable the touchpad PENIRQ */
	touch_read_value(TscgetX, MaxRetry, MaxRange);

	wifi_init();
	trapinit();

	intrenable(VBLANKbit, vblankintr, nil, 0);

	FIFOREG->ctl = (FifoRirq|Fifoenable|FifoTflush);
	intrenable(FRECVbit, fiforecvintr, nil, 0);

	intrenable(TIMERWIFIbit, wifi_timer_handler, nil, 0);
	intrenable(WIFIbit, wifi_interrupt, nil, 0);

	// keep the ARM7 out of main RAM
	while (1)
		swiWaitForVBlank();
	return 0;
}

