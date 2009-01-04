#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "dat.h"
#include "fns.h"
#include "spi.h"
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
	uchar ndstype[1];
	TxSound *snd;
	int power, ier;

	while(!(FIFOREG->ctl & FifoRempty)) {
		vv = FIFOREG->recv;
		v = vv>>Fcmdlen;
		switch(vv&Fcmdtmask) {
		case F9TSystem:
			if(0)print("F9S %lux\n", vv & Fcmdsmask);
			switch(vv&Fcmdsmask){
			case F9Sysbright:
				read_firmware(FWconsoletype, ndstype, sizeof ndstype);
				if(ndstype[0] == FWdslite || ndstype[0] == FWds)
					power_write(POWER_BACKLIGHT, v);
				break;
			
			case F9Syspoweroff:
				power_write(POWER_CONTROL, POWER0_SYSTEM_POWER);
				break;
			case F9Syssleep:
				// save current power & int state.
				ier = INTREG->ier;
				power = power_read(POWER_CONTROL);
				power_write(POWER_CONTROL, PM_LED_CONTROL(1));

				// register & sleep for the lid open interrupt.
				INTREG->ier = LIDbit;
				swiSleep();
				swiDelay(838000);	//100ms
		
				// restore power & int state.
				INTREG->ier = ier;
				power_write(POWER_CONTROL, power);
				break;

			case F9Sysreboot:
				swiSoftReset();	// TODO: doesn't work
				break;
			case F9Sysleds:
				power = power_read(POWER_CONTROL);
				power &= ~PM_LED_CONTROL(3);
		                power |= power & 0xFF;
				power_write(POWER_CONTROL, v);
				break;
			case F9Sysrrtc:
				(*(ulong*)v) = nds_get_time7();
				break;
			case F9Syswrtc:
				//nds_set_time7(*(ulong*)v);
				break;
			}
			break;
		
		case F9TWifi:
			if(0)print("F9W %lux\n", vv & Fcmdmask);
			switch(vv&Fcmdsmask){
			case F9WFrmac:
				// mac is needed early by the arm9 in archds.c:/^archether
				read_firmware(0x36, (ulong*)v, sizeof(WifiData->MacAddr));
				break;
			case F9WFinit:
				Wifi_Init(v);
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
	*t1 = touch_read(Tscgettemp1);
	*t2 = touch_read(Tscgettemp2);
	return (*t2 - *t1) * 8568 - 273*4096; /* C = K - 273 */
}

static void
vblankintr(void*)
{
	Wifi_Update();

	/* clear FIFO errors */
	if (FIFOREG->ctl & Fifoerror)
		FIFOREG->ctl |= Fifoenable|Fifoerror;
	
	intrclear(VBLANKbit, 0);
}

static void
vcountintr(void*)
{
	static int hbt = 0;
	ulong bst, cbst, bup, bdown;
	static ulong obst = Btnmsk; /* initial button state */

	hbt++;
	/* check buttons state */
	bst = KEYREG->in & Btn9msk;
	bst |= (KEYREG->xy & Btn7msk) << (Xbtn-Xbtn7);

	updatetouch(bst);
	
	cbst = bst^obst;
	obst = bst;
	bup = cbst & bst;
	bdown = cbst & ~bst;

	if(bdown)
		nbfifoput(F7keydown, bdown);
	if(bup)
		nbfifoput(F7keyup, bup);
	
	IPC->aux = touch_read(Tscgetmic8);
	IPC->temp = touch_read_temp(&IPC->td1, &IPC->td2);
	//if(hbt++%30 == 0) print("aux %d temp %d\n", IPC->aux, IPC->temp);

	intrclear(VCOUNTbit, 0);
}

int 
main(void)
{
	INTREG->ime = 0;
	memset(edata, 0, end-edata); 		/* clear the BSS */
	read_firmware(0x03FE00, (ulong*)UINFOMEM, sizeof(UserInfo));
	
	/* dummy read to enable the touchpad PENIRQ */
	touch_read_value(TscgetX, MaxRetry, MaxRange);

	trapinit();
	intrenable(VBLANKbit, vblankintr, nil, 0);
	intrenable(VCOUNTbit, vcountintr, nil, 0);
	FIFOREG->ctl = FifoRirq|Fifoerror|Fifoenable|FifoTflush;
	intrenable(FRECVbit, fiforecvintr, nil, 0);

	// keep the ARM7 out of main RAM
	while (1)
		swiWaitForVBlank();
	return 0;
}
