/*
DS linux network driver
Copyright (C) 2006 Bthaeler - bthaeler@aol.com

Parts of this code were adapted from
// DS Wifi interface code
// wifi_arm7.c - arm7 wifi interface code
// wifi_arm9.c - arm9 wifi support code
// Copyright (C) 2005 Stephen Stair - sgstair@akkit.org - http://www.akkit.org

DSWifi Lib and test materials are licenced under the MIT open source licence:
Copyright (c) 2005 Stephen Stair
*/

#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "dat.h"
#include "fns.h"

#include "spi.h"
#include "wifi.h"

#define	DPRINT	if(1)print

Wifi_Data wifi_data;

nds_rx_packet *rx_packet = nil;
static int chdata_save5 = 0;

static void Wifi_Stop(void);
static void Wifi_DisableTempPowerSave(void);
static void Wifi_RFInit(void);
static void Wifi_RxSetup(void);
static void Wifi_TxSetup(void);
static void Wifi_CopyMacAddr(volatile void *dest, volatile void *src);
static void Wifi_SendOpenSystemAuthPacket(void);
static void Wifi_SendSharedKeyAuthPacket(void);
static void Wifi_SendBackChallengeText(uchar * challenge);
static void Wifi_SendAssocPacket(void);
static void Wifi_SetChannel(int channel);
static void Wifi_SetMode(int wifimode);
static void Wifi_Start(void);
static void Wifi_Stop(void);

static int Wifi_ProcessReceivedFrame(int macbase, int framelen);
static int Wifi_ProcessBeaconFrame(int macbase, int framelen);
static int Wifi_ProcessReassocResponse(int macbase, int framelen);
static int Wifi_ProcessAuthenticationFrame(int macbase, int framelen);
static int Wifi_ProcessDeAuthenticationFrame(int macbase, int framelen);

static void wifi_interrupt(void*);
static void wifi_timer_handler(void*);

//////////////////////////////////////////////////////////////////////////
//
//  Flash support functions
//

static uchar FlashData[512];

static int ReadFlashByte(int address)
{
	if (address < 0 || address > 511)
		return 0;
	return FlashData[address];
}

static int ReadFlashBytes(int address, int numbytes)
{
	int dataout = 0;
	int i;
	for (i = 0; i < numbytes; i++) {
		dataout |= ReadFlashByte(i + address) << (i * 8);
	}
	return dataout;
}

static int ReadFlashHWord(int address)
{
	if (address < 0 || address > 510)
		return 0;
	return ReadFlashBytes(address, 2);
}

void ReadFlashData(void)
{
	read_firmware(0, FlashData, sizeof(FlashData));
	memmove(wifi_data.MacAddr, FlashData+0x36, sizeof(wifi_data.MacAddr));	
}

//////////////////////////////////////////////////////////////////////////
//
//  Other support
//

static int Wifi_BBRead(int a)
{
	while (WIFI_BBSIOBUSY & 1) ;
	WIFI_BBSIOCNT = a | 0x6000;
	while (WIFI_BBSIOBUSY & 1) ;
	return WIFI_BBSIOREAD;
}

static int Wifi_BBWrite(int a, int b)
{
	int i;
	i = 0x2710;
	while ((WIFI_BBSIOBUSY & 1)) {
		if (!i--)
			return -1;
	}
	WIFI_BBSIOWRITE = b;
	WIFI_BBSIOCNT = a | 0x5000;
	i = 0x2710;
	while ((WIFI_BBSIOBUSY & 1)) {
		if (!i--)
			return 0;
	}
	return 0;
}

static void Wifi_RFWrite(int writedata)
{
	while (WIFI_RFSIOBUSY & 1) ;
	WIFI_RFSIODATA1 = writedata;
	WIFI_RFSIODATA2 = writedata >> 16;
	while (WIFI_RFSIOBUSY & 1) ;
}

#define LED_LONGBLINK	1
#define LED_SHORTBLINK	3
#define LED_ON			0
void SetLedState(int state) {
	int i;
	static int led_state=0;

	if(state>3 || state<0)
		return;
	if(state!=led_state) {
		led_state=state;
		i=power_read(POWER_CONTROL);
		i=i&0xCF;
		i |= state<<4;
		power_write(POWER_CONTROL,i);
	}
}

//////////////////////////////////////////////////////////////////////////
//
//  Main functionality
//

static int RF_Reglist[] = { 0x146, 0x148, 0x14A, 0x14C, 0x120, 0x122, 0x154, 0x144, 0x130, 0x132, 0x140, 0x142, 0x38, 0x124, 0x128, 0x150 };

static void Wifi_RFInit(void)
{
	int i,j;
	int channel_extrabits;
	int numchannels;
	int channel_extrabytes;
	int temp;
	for(i=0;i<16;i++) {
		WIFI_REG(RF_Reglist[i])=ReadFlashHWord(0x44+i*2);
	}
	numchannels=ReadFlashByte(0x42);
	channel_extrabits=ReadFlashByte(0x41);
	channel_extrabytes=(channel_extrabits+7)/8;
	WIFI_REG(0x184)=((channel_extrabits>>7)<<8) | (channel_extrabits&0x7F);
	j=0xCE;
	if(ReadFlashByte(0x40)==3) {
		for(i=0;i<numchannels;i++) {
			Wifi_RFWrite(ReadFlashByte(j++)|(i<<8)|0x50000);
		}
	} else if(ReadFlashByte(0x40)==2) {
		for(i=0;i<numchannels;i++) {
			temp = ReadFlashBytes(j,channel_extrabytes);
			Wifi_RFWrite(temp);
			j+=channel_extrabytes;
			if( (temp>>18)==9 ) {
				chdata_save5 = temp&(~0x7C00);
			}
		}
	} else {
		for(i=0;i<numchannels;i++) {
			Wifi_RFWrite(ReadFlashBytes(j,channel_extrabytes));
			j+=channel_extrabytes;
		}
	}
}

static void Wifi_BBInit(void)
{
	int i;
	WIFI_REG(0x160) = 0x0100;
	for (i = 0; i < 0x69; i++) {
		Wifi_BBWrite(i, ReadFlashByte(0x64 + i));
	}
}

// 22 entry list
static int MAC_Reglist[] =
    { 0x04, 0x08, 0x0A, 0x12, 0x10, 0x254, 0xB4, 0x80, 0x2A, 0x28, 0xE8, 0xEA,
	0xEE, 0xEC, 0x1A2, 0x1A0, 0x110, 0xBC, 0xD4, 0xD8, 0xDA, 0x76
};
static int MAC_Vallist[] =
    { 0, 0, 0, 0, 0xffff, 0, 0xffff, 0, 0, 0, 0, 0, 1, 0x3F03, 1, 0, 0x0800, 1,
	3, 4, 0x0602, 0
};

static void Wifi_MacInit(void)
{
	int i;
	for (i = 0; i < 22; i++) {
		WIFI_REG(MAC_Reglist[i]) = MAC_Vallist[i];
	}
}

static void Wifi_TxSetup(void)
{
	/*	switch(WIFI_REG(0x8006)&7) {
	case 0: //
	// 4170,  4028, 4000
	// TxqEndData, TxqEndManCtrl, TxqEndPsPoll
	WIFI_REG(0x4024)=0xB6B8;
	WIFI_REG(0x4026)=0x1D46;
	WIFI_REG(0x416C)=0xB6B8;
	WIFI_REG(0x416E)=0x1D46;
	WIFI_REG(0x4790)=0xB6B8;
	WIFI_REG(0x4792)=0x1D46;
	WIFI_REG(0x80AE) = 1;
	break;
	case 1: //
	// 4AA0, 4958, 4334
	// TxqEndData, TxqEndManCtrl, TxqEndBroadCast
	// 4238, 4000
	WIFI_REG(0x4234)=0xB6B8;
	WIFI_REG(0x4236)=0x1D46;
	WIFI_REG(0x4330)=0xB6B8;
	WIFI_REG(0x4332)=0x1D46;
	WIFI_REG(0x4954)=0xB6B8;
	WIFI_REG(0x4956)=0x1D46;
	WIFI_REG(0x4A9C)=0xB6B8;
	WIFI_REG(0x4A9E)=0x1D46;
	WIFI_REG(0x50C0)=0xB6B8;
	WIFI_REG(0x50C2)=0x1D46;
	//...
	break;
	case 2:
	// 45D8, 4490, 4468
	// TxqEndData, TxqEndManCtrl, TxqEndPsPoll

	WIFI_REG(0x4230)=0xB6B8;
	WIFI_REG(0x4232)=0x1D46;
	WIFI_REG(0x4464)=0xB6B8;
	WIFI_REG(0x4466)=0x1D46;
	WIFI_REG(0x448C)=0xB6B8;
	WIFI_REG(0x448E)=0x1D46;
	WIFI_REG(0x45D4)=0xB6B8;
	WIFI_REG(0x45D6)=0x1D46;
	WIFI_REG(0x4BF8)=0xB6B8;
	WIFI_REG(0x4BFA)=0x1D46;
	*/
	WIFI_REG(0x80AE)=0x000D;
	//	}
}

static void Wifi_RxSetup(void)
{
	WIFI_REG(0x8030) = 0x8000;
	/*	switch(WIFI_REG(0x8006)&7) {
	case 0:
	WIFI_REG(0x8050) = 0x4794;
	WIFI_REG(0x8056) = 0x03CA;
	// 17CC ?
	break;
	case 1:
	WIFI_REG(0x8050) = 0x50C4;
	WIFI_REG(0x8056) = 0x0862;
	// 0E9C ?
	break;
	case 2:
	WIFI_REG(0x8050) = 0x4BFC;
	WIFI_REG(0x8056) = 0x05FE;
	// 1364 ?
	break;
	case 3:
	WIFI_REG(0x8050) = 0x4794;
	WIFI_REG(0x8056) = 0x03CA;
	// 17CC ?
	break;
	}
	*/
	WIFI_REG(0x8050) = 0x4C00;
	WIFI_REG(0x8056) = 0x0600;

	WIFI_REG(0x8052) = 0x5F60;
	WIFI_REG(0x805A) = (WIFI_REG(0x8050)&0x3FFF)>>1;
	WIFI_REG(0x8062) = 0x5F5E;
	WIFI_REG(0x8030) = 0x8001;
}

static void Wifi_WakeUp(void)
{
	ulong i;
	WIFI_REG(0x8036) = 0;
	
	swiDelay( 67109 ); // 8ms delay
	
	WIFI_REG(0x8168) = 0;

	i = Wifi_BBRead(1);
	Wifi_BBWrite(1, i & 0x7f);
	Wifi_BBWrite(1, i);
	
	swiDelay( 335544 ); // 40ms delay

	Wifi_RFInit();
}

/* turn off the device */
static void Wifi_Shutdown(void)
{
	int a;
	if (ReadFlashByte(0x40) == 2) {
		Wifi_RFWrite(0xC008);
	}
	a = Wifi_BBRead(0x1E);
	Wifi_BBWrite(0x1E, a | 0x3F);
	WIFI_REG(0x168) = 0x800D;
	WIFI_REG(0x36) = 1;
}

static void Wifi_CopyMacAddr(volatile void *dest, volatile void *src)
{
	((ushort *) dest)[0] = ((ushort *) src)[0];
	((ushort *) dest)[1] = ((ushort *) src)[1];
	((ushort *) dest)[2] = ((ushort *) src)[2];
}

int Wifi_CmpMacAddr(volatile void *mac1, volatile void *mac2)
{
	return (((ushort *) mac1)[0] == ((ushort *) mac2)[0]) && (((ushort *) mac1)[1] == ((ushort *) mac2)[1]) && (((ushort *) mac1)[2] == ((ushort *) mac2)[2]);
}

//////////////////////////////////////////////////////////////////////////
//
//  MAC Copy functions
//

static ushort Wifi_MACRead(ulong MAC_Base, ulong MAC_Offset)
{
	MAC_Base += MAC_Offset;

	if (MAC_Base >= (WIFI_REG(0x52) & 0x1FFE))
		MAC_Base -=
		    ((WIFI_REG(0x52) & 0x1FFE) - (WIFI_REG(0x50) & 0x1FFE));

	return WIFI_REG(0x4000 + MAC_Base);
}

static void Wifi_MACCopy(ushort * dest, ulong MAC_Base, ulong MAC_Offset, ulong length)
{
	int endrange, subval;
	int thislength;
	endrange = (WIFI_REG(0x52) & 0x1FFE);
	subval = ((WIFI_REG(0x52) & 0x1FFE) - (WIFI_REG(0x50) & 0x1FFE));
	MAC_Base += MAC_Offset;
	if (MAC_Base >= endrange)
		MAC_Base -= subval;
	while (length > 0) {
		thislength = length;
		if (thislength > (endrange - MAC_Base))
			thislength = endrange - MAC_Base;
		length -= thislength;
		while (thislength > 0) {
			*(dest++) = WIFI_REG(0x4000 + MAC_Base);
			MAC_Base += 2;
			thislength -= 2;
		}
		MAC_Base -= subval;
	}
}

static void Wifi_MACWrite(ushort * src, ulong MAC_Base, ulong MAC_Offset, ulong length)
{
	int endrange,subval;
	int thislength;

	endrange = (WIFI_REG(0x52)&0x1FFE);
	subval=((WIFI_REG(0x52)&0x1FFE)-(WIFI_REG(0x50)&0x1FFE));
	MAC_Base += MAC_Offset;

	if(MAC_Base>=endrange)
		MAC_Base -= subval;

	while(length>0) {
		thislength=length;
		if(length>(endrange-MAC_Base)) length=endrange-MAC_Base;
		length-=thislength;
		while(thislength>0) {
			WIFI_REG(0x4000+MAC_Base) = *(src++);
			MAC_Base+=2;
			thislength-=2;
		}
		MAC_Base-=subval;
	}
}

int Wifi_QueueRxMacData(ulong base, ulong len)
{
	int macofs;

	macofs = 0;

	/* If we don't know where to buffer recieved data yet
	 * we have no choice but to throw the packet away :( */
	if (!rx_packet)
		return 0;

	if (len > sizeof(rx_packet->data)) {
		wifi_data.stats[WF_STAT_RXOVERRUN]++;
		return 0;
	}

	wifi_data.stats[WF_STAT_RXPKTS]++;
	wifi_data.stats[WF_STAT_RXDATABYTES] += len;

	Wifi_MACCopy((ushort *) rx_packet->data, base, macofs, len);
	rx_packet->len = len;
	IPCREG->ctl |= Ipcgenirq|I7WFrxdone;

	/* XOXOXO Disable rx interrupts */

	return 1;
}

static void wifi_send_raw(ushort* data, ushort length)
{
	length = (length + 3) & (~3);
	Wifi_MACWrite(data, 0, 0, length);
	WIFI_REG(0xB8) = 0x0001;
	WIFI_REG(0xA8) = 0x8000;

	wifi_data.stats[WF_STAT_TXPKTS]++;
	wifi_data.stats[WF_STAT_TXBYTES] += length;
	wifi_data.stats[WF_STAT_TXDATABYTES] += length - 12;
}

//////////////////////////////////////////////////////////////////////////
//
//  Wifi Interrupts
//

static void Wifi_Intr_RxEnd(void)
{
	int base;
	int packetlen;
	int full_packetlen;
	// int dest_offset;
	// int i, cut, temp;
	int cut;
	int tIME;
	int temp;

	tIME = INTREG->ime;

	INTREG->ime = 0;
	cut = 0;

/*
	wifi_data.stats[WF_STAT_DBG3] = WIFI_REG(0x54);
	wifi_data.stats[WF_STAT_DBG4] = WIFI_REG(0x5A);
*/

	while (WIFI_REG(0x54) != WIFI_REG(0x5A)) {
		base = WIFI_REG(0x5A) << 1;
		packetlen = Wifi_MACRead(base, 8);
		full_packetlen = 12 + ((packetlen + 3) & (~3));

		wifi_data.stats[WF_STAT_RXRAWPKTS]++;
		wifi_data.stats[WF_STAT_RXBYTES] += full_packetlen;
		wifi_data.stats[WF_STAT_RXDATABYTES] += full_packetlen - 12;

		// process packet here
		temp = Wifi_ProcessReceivedFrame(base, full_packetlen);	// returns packet type
		if ((wifi_data.state & WIFI_STATE_ASSOCIATED) && temp == WFLAG_PACKET_DATA) {	// if packet type is requested, forward it to the rx queue
			if (wifi_data.state & (WIFI_STATE_RXPENDING))
				break;
			if (!Wifi_QueueRxMacData(base, full_packetlen)) {
				wifi_data.state |= WIFI_STATE_RXPENDING;
				wifi_data.stats[WF_STAT_RXPKTS]++;
				// failed, ignore for now.
			}
		}

		base += full_packetlen;

		if (base >= (WIFI_REG(0x52) & 0x1FFE))
			base -= ((WIFI_REG(0x52) & 0x1FFE) - (WIFI_REG(0x50) & 0x1FFE));
		WIFI_REG(0x5A) = base >> 1;

		if (cut++ > 5)
			break;
	}
	INTREG->ime = tIME;
}

void wifi_rx_q_complete(void)
{
	wifi_data.state &= ~WIFI_STATE_RXPENDING;

	if (wifi_data.state & WIFI_STATE_UP)
		Wifi_Intr_RxEnd();
}

#define CNT_STAT_START WF_STAT_HW_1B0
#define CNT_STAT_NUM 18
ushort count_ofs_list[CNT_STAT_NUM] = {
	0x1B0, 0x1B2, 0x1B4, 0x1B6, 0x1B8, 0x1BA, 0x1BC, 0x1BE, 0x1C0, 0x1C4, 0x1D0, 0x1D2, 0x1D4, 0x1D6, 0x1D8, 0x1DA, 0x1DC, 0x1DE
};
static void Wifi_Intr_CntOverflow(void)
{
	int i;
	int s,d;
	s=CNT_STAT_START;
	for (i = 0; i < CNT_STAT_NUM; i++) {
		d = WIFI_REG(count_ofs_list[i]);
		wifi_data.stats[s++] += (d&0xFF);
		wifi_data.stats[s++] += ((d>>8)&0xFF);
	}
}

static void Wifi_Intr_StartTx(void)
{
	// attempt to ensure packet is received
}

static void Wifi_Intr_TxEnd(void)
{
	/* signal that current packet has been transmitted */
	if ((wifi_data.state & WIFI_STATE_TXPENDING)
	    && !(WIFI_REG(0xA0) & 0x8000)) {
		wifi_data.state &= ~WIFI_STATE_TXPENDING;
		IPCREG->ctl |= Ipcgenirq|I7WFtxdone;
	}
}

static void Wifi_Intr_TBTT(void)
{
	ushort rdyslots;
	
	rdyslots = 0;
	if (WIFI_REG(0xA0) & 0x8000)
		rdyslots |= 1;
	if (WIFI_REG(0xA4) & 0x8000)
		rdyslots |= 4;
	if (WIFI_REG(0xA8) & 0x8000)
		rdyslots |= 8;

	if (rdyslots)
		WIFI_REG(0xAE) = rdyslots;
}

static void Wifi_Intr_DoNothing(void)
{
}

static void Wifi_Intr_TxErr(void)
{
	wifi_data.state |= WIFI_STATE_SAW_TX_ERR;
}

static void
wifi_interrupt(void*)
{
	int wIF;

	if (!(wifi_data.state & WIFI_STATE_UP))
		return;

	while ((wIF = WIFI_IE & WIFI_IF) != 0){
		wifi_data.stats[WF_STAT_DBG6]++;
		wifi_data.stats[WF_STAT_DBG1] = WIFI_IE;
		wifi_data.stats[WF_STAT_DBG2] = WIFI_IF;
		
		// now that we've cleared the wireless IF, kill the bit in regular IF.
		intrclear(WIFIbit, 0);

		if(wIF& 0x0001) { WIFI_IF=0x0001;  Wifi_Intr_RxEnd();  } // 0) Rx End
		if(wIF& 0x0002) { WIFI_IF=0x0002;  Wifi_Intr_TxEnd();  } // 1) Tx End
		if(wIF& 0x0004) { WIFI_IF=0x0004;  Wifi_Intr_DoNothing();  } // 2) Rx Cntup
		if(wIF& 0x0008) { WIFI_IF=0x0008;  Wifi_Intr_TxErr();  }  // 3) Tx Err
		if(wIF& 0x0010) { WIFI_IF=0x0010;  Wifi_Intr_CntOverflow();  } // 4) Count Overflow
		if(wIF& 0x0020) { WIFI_IF=0x0020;  Wifi_Intr_CntOverflow();  } // 5) AckCount Overflow
		if(wIF& 0x0040) { WIFI_IF=0x0040;  Wifi_Intr_DoNothing();  } // 6) Start Rx
		if(wIF& 0x0080) { WIFI_IF=0x0080;  Wifi_Intr_StartTx();  } // 7) Start Tx
		if(wIF& 0x0100) { WIFI_IF=0x0100;  Wifi_Intr_DoNothing();  } // 8) 
		if(wIF& 0x0200) { WIFI_IF=0x0200;  Wifi_Intr_DoNothing();  } // 9)
		if(wIF& 0x0400) { WIFI_IF=0x0400;  Wifi_Intr_DoNothing();  } //10)
		if(wIF& 0x0800) { WIFI_IF=0x0800;  Wifi_Intr_DoNothing();  } //11) RF Wakeup
		if(wIF& 0x1000) { WIFI_IF=0x1000;  Wifi_Intr_DoNothing();  } //12) MP End
		if(wIF& 0x2000) { WIFI_IF=0x2000;  Wifi_Intr_DoNothing();  } //13) ACT End
		if(wIF& 0x4000) { WIFI_IF=0x4000;  Wifi_Intr_TBTT();  } //14) TBTT
		if(wIF& 0x8000) { WIFI_IF=0x8000;  Wifi_Intr_DoNothing();  } //15) PreTBTT
	}
}

//////////////////////////////////////////////////////////////////////////
//
//  Wifi User-called Functions
//

/* Turn on device and get it broadcasting */
void wifi_open(void)
{
	int i;
	
	if (wifi_data.state & WIFI_STATE_UP)
		return;
	
	wifi_data.state = 0;
	wifi_data.curChannel = 1;
	wifi_data.reqChannel = 1;
	wifi_data.wepkeyid = 0;
	memset(wifi_data.ssid, 0, sizeof(wifi_data.ssid));
	memset(wifi_data.wepkey, 0, sizeof(wifi_data.wepkey));
	memmove(wifi_data.MacAddr, FlashData+0x36, sizeof(wifi_data.MacAddr));	

	intrenable(WIFIbit, wifi_interrupt, nil, 0);
	intrenable(TIMERWIFIbit, wifi_timer_handler, nil, 0);

	POWERREG->pcr |= (1<<POWER_WIFI);	// enable power for the wifi
	*((volatile ushort *)0x04000206) = 0x30;	// ???

	// TODO ReadFlashData() should be done only here
	
	// reset/shutdown wifi:
	WIFI_REG(0x4) = 0xffff;
	Wifi_Stop();
	Wifi_Shutdown();	// power off wifi
	
	for (i = 0x4000; i < 0x6000; i += 2) WIFI_REG(i) = 0;

	WIFI_IE = 0;
	Wifi_WakeUp();

	Wifi_MacInit();
	Wifi_RFInit();
	Wifi_BBInit();

	// Set Default Settings
	WIFI_MACADDR[0] = ((ushort *) wifi_data.MacAddr)[0];
	WIFI_MACADDR[1] = ((ushort *) wifi_data.MacAddr)[1];
	WIFI_MACADDR[2] = ((ushort *) wifi_data.MacAddr)[2];

	WIFI_RETRLIMIT = 7;
	Wifi_SetMode(2);
	Wifi_SetWepMode(WEPMODE_NONE);

	Wifi_SetChannel(1);

	Wifi_BBWrite(0x13, 0x00);
	Wifi_BBWrite(0x35, 0x1F);

	Wifi_Start();
	
	wifi_data.state |= WIFI_STATE_UP;
	wifi_data.state &= ~(WIFI_STATE_ASSOCIATED | WIFI_STATE_ASSOCIATING);
}

/* turn off the wifi device */
void wifi_close(void)
{
	if (!(wifi_data.state & WIFI_STATE_UP))
		return;

	wifi_data.state &= ~WIFI_STATE_UP;

	Wifi_Stop();
	POWERREG->pcr &= ~(1<<POWER_WIFI);

	SetLedState(LED_ON);
}

static void Wifi_Start(void)
{
	int i, tIME;

	tIME = INTREG->ime;
	INTREG->ime = 0;
	Wifi_Stop();

	//	Wifi_WakeUp();

	WIFI_REG(0x8032) = 0x8000;
	WIFI_REG(0x8134) = 0xFFFF;
	WIFI_REG(0x802A) = 0;
	WIFI_AIDS           = 0;
	WIFI_REG(0x80E8) = 1;
	WIFI_REG(0x8038) = 0x0000;
	WIFI_REG(0x20) = 0x0000;
	WIFI_REG(0x22) = 0x0000;
	WIFI_REG(0x24) = 0x0000;

	Wifi_TxSetup();
	Wifi_RxSetup();

	WIFI_REG(0x8030) = 0x8000;
	/*
	switch(WIFI_REG(0x8006)&7) {
	case 0: // infrastructure mode?
	WIFI_IF=0xFFFF;
	WIFI_IE=0x003F;
	WIFI_REG(0x81AE)=0x1fff;
	//WIFI_REG(0x81AA)=0x0400;
	WIFI_REG(0x80D0)=0xffff;
	WIFI_REG(0x80E0)=0x0008;
	WIFI_REG(0x8008)=0;
	WIFI_REG(0x800A)=0;
	WIFI_REG(0x80E8)=0;
	WIFI_REG(0x8004)=1;
	//SetStaState(0x40);
	break;
	case 1: // ad-hoc mode? -- beacons are required to be created!
	WIFI_IF=0xFFF;
	WIFI_IE=0x703F;
	WIFI_REG(0x81AE)=0x1fff;
	WIFI_REG(0x81AA)=0; // 0x400
	WIFI_REG(0x80D0)=0x0301;
	WIFI_REG(0x80E0)=0x000D;
	WIFI_REG(0x8008)=0xE000;
	WIFI_REG(0x800A)=0;
	WIFI_REG(0x8004)=1;
	//??
	WIFI_REG(0x80EA)=1;
	WIFI_REG(0x80AE)=2;
	break;
	case 2: // DS comms mode?
	*/
	WIFI_IF=0xFFFF;
	//WIFI_IE=0xE03F;
	WIFI_IE=0x40B3 | 0x0008;
	WIFI_REG(0x81AE)=0x1fff;
	WIFI_REG(0x81AA)=0; //0x68
	WIFI_BSSID[0]=0xFFFF;
	WIFI_BSSID[1]=0xFFFF;
	WIFI_BSSID[2]=0xFFFF;
	WIFI_REG(0x80D0)=0x0181; // 0x181
	WIFI_REG(0x80E0)=0x000B;
	WIFI_REG(0x8008)=0;
	WIFI_REG(0x800A)=0;
	WIFI_REG(0x8004)=1;
	WIFI_REG(0x80E8)=1;
	WIFI_REG(0x80EA)=1;
	//SetStaState(0x20);
	/*
	break;
	case 3:
	case 4:
	break;
	}
	*/

	WIFI_REG(0x8048)=0x0000;
	Wifi_DisableTempPowerSave();
	//WIFI_REG(0x80AE)=0x0002;
	WIFI_POWERSTATE |= 2;
	WIFI_REG(0xAC) = 0xFFFF;
	i=0xFA0;
	while(i!=0 && !(WIFI_REG(0x819C)&0x80)) i--;
	INTREG->ime = tIME;
}

/* silence the device */
static void Wifi_Stop(void)
{
	int tIME;
	tIME = INTREG->ime;
	WIFI_IE = 0;
	WIFI_REG(0x8004) = 0;
	WIFI_REG(0x80EA) = 0;
	WIFI_REG(0x80E8) = 0;
	WIFI_REG(0x8008) = 0;
	WIFI_REG(0x800A) = 0;

	WIFI_REG(0x80AC) = 0xFFFF;
	WIFI_REG(0x80B4) = 0xFFFF;
//      Wifi_Shutdown();
	INTREG->ime = tIME;
}

static void Wifi_SetChannel(int channel) {
	int i,n,l;
	if(channel<1 || channel>13) return;
	channel-=1;

	switch(ReadFlashByte(0x40)) {
	case 2:
	case 5:
		Wifi_RFWrite(ReadFlashBytes(0xf2+channel*6,3));
		Wifi_RFWrite(ReadFlashBytes(0xf5+channel*6,3));

		swiDelay( 12583 ); // 1500 us delay

		if(chdata_save5 & 0x10000)
		{
			if(chdata_save5 & 0x8000) break;
			n = ReadFlashByte(0x154+channel);
			Wifi_RFWrite( chdata_save5 | ((n&0x1F)<<10) );
		} else {
			Wifi_BBWrite(0x1E, ReadFlashByte(0x146+channel));
		}

		break;
	case 3:
		n=ReadFlashByte(0x42);
		n+=0xCF;
		l=ReadFlashByte(n-1);
		for(i=0;i<l;i++) {
			Wifi_BBWrite(ReadFlashByte(n),ReadFlashByte(n+channel+1));
			n+=15;
		}
		for(i=0;i<ReadFlashByte(0x43);i++) {
			Wifi_RFWrite( (ReadFlashByte(n)<<8) | ReadFlashByte(n+channel+1) | 0x050000 );
			n+=15;
		}

		swiDelay( 12583 ); // 1500 us delay

		break;
	default:
		break;
	}
	wifi_data.curChannel=channel+1;
}

void Wifi_RequestChannel(int channel)
{
	wifi_data.reqChannel = channel;
	if (wifi_data.state & WIFI_STATE_CHANNEL_SCANNING)
		return;
	if (!(wifi_data.state & WIFI_STATE_UP))
		return;

	Wifi_SetChannel(channel);
}

static void wifi_try_to_associate(void)
{
	int i, j;

	wifi_data.state &= ~(WIFI_STATE_ASSOCIATED | WIFI_STATE_ASSOCIATING);

	SetLedState(LED_LONGBLINK);

	for (i = 0; i < WIFI_MAX_AP; i++) {
		if (wifi_data.ssid[0] != wifi_data.aplist[i].ssid_len)
			continue;
		if(strcmp(&wifi_data.ssid[1], wifi_data.aplist[i].ssid) == 0)
			break;
	}

	if (i == WIFI_MAX_AP) {
		/* haven't see it yet, just wait */
		wifi_data.state |= WIFI_STATE_ASSOCIATING;
		return;
	}

	Wifi_CopyMacAddr(wifi_data.bssid, wifi_data.aplist[i].bssid);
	Wifi_CopyMacAddr(wifi_data.apmac, wifi_data.aplist[i].macaddr);
	for (j = 0; j < 16; j++)
		wifi_data.baserates[j] = wifi_data.aplist[i].base_rates[j];

	Wifi_SetWepKey((void*) wifi_data.wepkey);
	Wifi_SetWepMode(wifi_data.wepmode);
	
	WIFI_BSSID[0] = ((ushort *) wifi_data.bssid)[0];
	WIFI_BSSID[1] = ((ushort *) wifi_data.bssid)[1];
	WIFI_BSSID[2] = ((ushort *) wifi_data.bssid)[2];
	//WIFI_REG(0xD0) &= ~0x0400;
	WIFI_REG(0xD0) |= 0x0400;

	wifi_data.reqChannel = wifi_data.aplist[i].channel;
	Wifi_SetChannel(wifi_data.reqChannel);

	if (wifi_data.aplist[i].flags & WFLAG_APDATA_ADHOC)
		wifi_data.curMode = WIFI_AP_ADHOC;
	else
		wifi_data.curMode = WIFI_AP_INFRA;

	if (wifi_data.curMode == WIFI_AP_ADHOC) {
		wifi_data.state |= WIFI_STATE_ASSOCIATED;
		SetLedState(LED_SHORTBLINK);
	} else {
		wifi_data.state &= ~WIFI_STATE_AUTHENTICATED;
		if (wifi_data.wepmode == WEPMODE_NONE)
			Wifi_SendOpenSystemAuthPacket();
		else
			Wifi_SendSharedKeyAuthPacket();
	}
}

void Wifi_SetSSID(int off, char b1, char b2)
{
#ifdef NOTDEF
	wifi_data.ssid[(off * 2) + 1] = b1;
	wifi_data.ssid[(off * 2) + 2] = b2;

	if (!b1)
		wifi_data.ssid[0] = off * 2;
	else if (!b2)
		wifi_data.ssid[0] = off * 2 + 1;
#endif
	if (!b1 || !b2) {
		/* once we see the null terminator, we are done */
		if (wifi_data.state & WIFI_STATE_UP) {
			wifi_try_to_associate();
		}
	}
}

void Wifi_SetWepKey(void *wepkey){
	int i;
	for (i = 0; i <16; i++) {
		WIFI_WEPKEY0[i] = ((ushort *) wepkey)[i];
		WIFI_WEPKEY1[i] = ((ushort *) wepkey)[i];
		WIFI_WEPKEY2[i] = ((ushort *) wepkey)[i];
		WIFI_WEPKEY3[i] = ((ushort *) wepkey)[i];
	}
}

void Wifi_SetWepMode(int wepmode)
{
	if (wepmode < 0 || wepmode > 7)
		return;

	wifi_data.wepmode = wepmode;

        if(wepmode==0) {
                WIFI_REG(0x32)=0x0000;
        } else {
                WIFI_REG(0x32)=0x8000;
        }

	if(wepmode==0) wepmode=1;
	WIFI_MODE_WEP = (WIFI_MODE_WEP & 0xFFC7) | (wepmode << 3);
}

void Wifi_SetWepKeyID(int key)
{
	wifi_data.wepkeyid = key;
}

#ifdef NOTDEF
static void Wifi_SetBeaconPeriod(int beacon_period)
{
	if (beacon_period < 0x10 || beacon_period > 0x3E7)
		return;
	WIFI_REG(0x8C) = beacon_period;
}
#endif

static void Wifi_SetMode(int wifimode)
{
	if (wifimode > 3 || wifimode < 0)
		return;
	WIFI_MODE_WEP = (WIFI_MODE_WEP & 0xfff8) | wifimode;
}

#ifdef NOTDEF
static void Wifi_SetPreambleType(int preamble_type)
{
	if (preamble_type > 1 || preamble_type < 0)
		return;
	WIFI_REG(0x80BC) = (WIFI_REG(0x80BC) & 0xFFBF) | (preamble_type << 6);
}
#endif

static void Wifi_DisableTempPowerSave(void)
{
	WIFI_REG(0x8038) &= ~2;
	WIFI_REG(0x8048) = 0;
}

/* handle a query from kernel for wifi address */
void wifi_stats_query(void)
{
	wifi_data.stats[WF_STAT_DBG3] = WIFI_REG(0xA8);
	wifi_data.stats[WF_STAT_DBG4] = WIFI_REG(0xB8);

	wifi_data.stats[WF_STAT_DBG5] = wifi_data.state;

	wifi_data.state &= ~WIFI_STATE_SAW_TX_ERR;

//	nds_fifo_send(FIFO_WIFI_CMD(FIFO_WIFI_CMD_STATS_QUERY, 0));
}

//////////////////////////////////////////////////////////////////////////
//
//  802.11b system, tied in a bit with the :

static int Wifi_GenMgtHeader(uchar * data, ushort headerflags)
{
	// tx header
	((ushort *) data)[0] = 0;
	((ushort *) data)[1] = 0;
	((ushort *) data)[2] = 0;
	((ushort *) data)[3] = 0;
	((ushort *) data)[4] = 0;
	((ushort *) data)[5] = 0;
	// fill in most header fields
	((ushort *) data)[7] = 0x0000;
	Wifi_CopyMacAddr(data + 16, wifi_data.apmac);
	Wifi_CopyMacAddr(data + 22, wifi_data.MacAddr);
	Wifi_CopyMacAddr(data + 28, wifi_data.bssid);
	((ushort *) data)[17] = 0;

	// fill in wep-specific stuff
	if (headerflags & 0x4000) {
		((ulong *)data)[9]=((WIFI_RANDOM ^ (WIFI_RANDOM<<7) ^ (WIFI_RANDOM<<15))&0x0FFF) | (wifi_data.wepkeyid<<30); // I'm lazy and certainly haven't done this to spec.
		((ushort *) data)[6] = headerflags;
		return 28 + 12;
	} else {
		((ushort *) data)[6] = headerflags;
		return 24 + 12;
	}
}

static void Wifi_SendOpenSystemAuthPacket(void)
{
	// max size is 12+24+4+6 = 46
	uchar data[64];
	int i;

	DPRINT("wsaup\n");

	i=Wifi_GenMgtHeader(data,0x00B0);

	((ushort *)(data+i))[0]=0; // Authentication algorithm number (0=open system)
	((ushort *)(data+i))[1]=1; // Authentication sequence number
	((ushort *)(data+i))[2]=0; // Authentication status code (reserved for this message, =0)

	((ushort *)data)[4]=0x000A;
	((ushort *)data)[5]=i+6-12+4;

	wifi_send_raw((ushort*)data, i+6);
}

static void Wifi_SendSharedKeyAuthPacket(void)
{
	// max size is 12+24+4+6 = 46
	uchar data[64];
	int i;
	i=Wifi_GenMgtHeader(data,0x00B0);

	((ushort *)(data+i))[0]=1; // Authentication algorithm number (1=shared key)
	((ushort *)(data+i))[1]=1; // Authentication sequence number
	((ushort *)(data+i))[2]=0; // Authentication status code (reserved for this message, =0)

	((ushort *)data)[4]=0x000A;
	((ushort *)data)[5]=i+6-12+4;

	wifi_send_raw((ushort*)data, i+6);
}

static void Wifi_SendBackChallengeText(uchar * challenge)
{
	// size is 12+28+4+6+130
	uchar data[180];
	int i;
	int j;
	ushort *fixed_params;
	uchar *tagged_params;

	i = Wifi_GenMgtHeader(data, 0x40B0);	// Auth + WEP

	fixed_params = (ushort *) (data + i);

	fixed_params[0] = 1;	// Authentication algorithm number (1=shared key)
	fixed_params[1] = 3;	// Authentication sequence number
	fixed_params[2] = 0;	// Authentication status code (reserved for this message, =0)

	tagged_params = data + i + 6;

	// send back the challenge text
	for (j = 0; j < challenge[1] + 2; j++) {
		tagged_params[j] = challenge[j];
	}

	((ushort *) data)[4] = 0x000A;	// 1Mbit
	((ushort *) data)[5] = i + 6 - 12 + 4 + 130 + 4;	// length

	wifi_send_raw((ushort*)data, i + 6 + 130);
}

static void Wifi_SendAssocPacket(void)
{
	// max size is 12+24+4+34+4 = 66
	uchar data[96];
	int i, j, numrates;

	DPRINT("wsasp\n");

	i = Wifi_GenMgtHeader(data, 0x0000);

	if (wifi_data.wepmode) {
		((ushort *) (data + i))[0] = 0x0011;	// CAPS info
	} else {
		((ushort *) (data + i))[0] = 0x0001;	// CAPS info
	}

	((ushort *) (data + i))[1] = WIFI_REG(0x8E);	// Listen interval
	i += 4;
	data[i++] = 0;		// SSID element
	data[i++] = wifi_data.ssid[0];
	for (j = 0; j < wifi_data.ssid[0]; j++)
		data[i++] = wifi_data.ssid[1 + j];

	if ((wifi_data.baserates[0] & 0x7f) != 2) {
		for (j = 1; j < 16; j++)
			wifi_data.baserates[j] = wifi_data.baserates[j - 1];
	}
	wifi_data.baserates[0] = 0x82;
	if ((wifi_data.baserates[1] & 0x7f) != 4) {
		for (j = 2; j < 16; j++)
			wifi_data.baserates[j] = wifi_data.baserates[j - 1];
	}
	wifi_data.baserates[1] = 0x04;

	wifi_data.baserates[15] = 0;
	for (j = 0; j < 16; j++)
		if (wifi_data.baserates[j] == 0)
			break;
	numrates = j;
	for (j = 2; j < numrates; j++)
		wifi_data.baserates[j] &= 0x7F;

	data[i++] = 1;		// rate set
	data[i++] = numrates;
	for (j = 0; j < numrates; j++)
		data[i++] = wifi_data.baserates[j];

	// reset header fields with needed data
	((ushort *) data)[4] = 0x000A;
	((ushort *) data)[5] = i - 12 + 4;

	wifi_send_raw((ushort*)data, i);
}

static int Wifi_SendNullFrame(void)
{
	// max size is 12+16 = 28
	uchar data[32];
	// tx header
	((ushort *)data)[0]=0;
	((ushort *)data)[1]=0;
	((ushort *)data)[2]=0;
	((ushort *)data)[3]=0;
	((ushort *)data)[4]=wifi_data.maxrate;
	((ushort *)data)[5]=18+4;
	// fill in packet header fields
	((ushort *)data)[6]=0x0148;
	((ushort *)data)[7]=0;
	Wifi_CopyMacAddr(data+16,wifi_data.apmac);
	Wifi_CopyMacAddr(data+22,wifi_data.MacAddr);

	wifi_send_raw((ushort*)data, 30);
	return 0;
}

static int Wifi_SendPSPollFrame(void)
{
	// max size is 12+16 = 28
	uchar data[32];
	// int i;
	// tx header
	((ushort *) data)[0] = 0;
	((ushort *) data)[1] = 0;
	((ushort *) data)[2] = 0;
	((ushort *) data)[3] = 0;
	((ushort *) data)[4] = wifi_data.maxrate;
	((ushort *) data)[5] = 16 + 4;
	// fill in packet header fields
	((ushort *) data)[6] = 0x01A4;
	((ushort *) data)[7] = 0xC000 | WIFI_AIDS;
	Wifi_CopyMacAddr(data + 16, wifi_data.apmac);
	Wifi_CopyMacAddr(data + 22, wifi_data.MacAddr);

	wifi_send_raw((ushort*)data, 28);
	return 0;
}

static int Wifi_ProcessReceivedFrame(int macbase, int framelen)
{
	ushort control_802;

	USED(framelen);
	
	control_802 = Wifi_MACRead(macbase, 12);

	switch ((control_802 >> 2) & 0x3F) {
		// Management Frames
	case 0x20:		// 1000 00 Beacon
		// mine data from the beacon...
		return Wifi_ProcessBeaconFrame(macbase, framelen);
	case 0x04:		// 0001 00 Assoc Response
	case 0x0C:		// 0011 00 Reassoc Response
		// we might have been associated, let's check.
		return Wifi_ProcessReassocResponse(macbase, framelen);
	case 0x00:		// 0000 00 Assoc Request  
	case 0x08:		// 0010 00 Reassoc Request
	case 0x10:		// 0100 00 Probe Request
	case 0x14:		// 0101 00 Probe Response
	case 0x24:		// 1001 00 ATIM
	case 0x28:		// 1010 00 Disassociation
		return WFLAG_PACKET_MGT;
	case 0x2C:		// 1011 00 Authentication
		// check auth response to ensure we're in
		return Wifi_ProcessAuthenticationFrame(macbase, framelen);
	case 0x30:		// 1100 00 Deauthentication
		return Wifi_ProcessDeAuthenticationFrame(macbase, framelen);
		// Control Frames
	case 0x29:		// 1010 01 PowerSave Poll
	case 0x2D:		// 1011 01 RTS
	case 0x31:		// 1100 01 CTS
	case 0x35:		// 1101 01 ACK
	case 0x39:		// 1110 01 CF-End
	case 0x3D:		// 1111 01 CF-End+CF-Ack
		return WFLAG_PACKET_CTRL;
		// Data Frames
	case 0x02:		// 0000 10 Data
	case 0x06:		// 0001 10 Data + CF-Ack
	case 0x0A:		// 0010 10 Data + CF-Poll
	case 0x0E:		// 0011 10 Data + CF-Ack + CF-Poll
		// We like data!

		return WFLAG_PACKET_DATA;
	case 0x12:		// 0100 10 Null Function
	case 0x16:		// 0101 10 CF-Ack
	case 0x1A:		// 0110 10 CF-Poll
	case 0x1E:		// 0111 10 CF-Ack + CF-Poll
		return WFLAG_PACKET_DATA;
	default:		// ignore!
		return 0;
	}
}

static int Wifi_ProcessBeaconFrame(int macbase, int framelen)
{
	Wifi_RxHeader packetheader;
	uchar data[512];
	uchar wepmode, fromsta;
	uchar segtype, seglen;
	uchar channel;
	uchar wpamode;
	uchar rateset[16];
	ushort ptr_ssid;
	ushort maxrate;
	ushort curloc;
	ulong datalen;
	ushort i, j, compatible;

	USED(framelen);

	/* is ap_list locked? */
	if (wifi_data.state & WIFI_STATE_APQUERYPEND)
		return WFLAG_PACKET_BEACON;

	Wifi_MACCopy((ushort *) & packetheader, macbase, 0, 12);

	datalen = packetheader.byteLength;

	if (datalen > 512)
		datalen = 512;
	Wifi_MACCopy((ushort *) data, macbase, 12, (datalen + 1) & ~1);
	wepmode = 0;
	if (((ushort *) data)[5 + 12] & 0x0010) {	// capability info, WEP bit
		wepmode = 1;
	}
	fromsta = Wifi_CmpMacAddr(data + 10, data + 16);
	curloc = 12 + 24;	// 12 fixed bytes, 24 802.11 header
	compatible = 1;
	ptr_ssid = 0;
	channel = wifi_data.curChannel;
	wpamode = 0;
	maxrate = 0;
	do {
		if (curloc >= datalen)
			break;
		segtype = data[curloc++];
		seglen = data[curloc++];
		switch (segtype) {
		case 0:	// SSID element
			ptr_ssid = curloc - 2;
			break;
		case 1:	// rate set (make sure we're compatible)
			compatible = 0;
			maxrate = 0;
			j = 0;
			for (i = 0; i < seglen; i++) {
				if ((data[curloc + i] & 0x7F) > maxrate)
					maxrate = data[curloc + i] & 0x7F;
				if (j < 15 && (data[curloc + i] & 0x80))
					rateset[j++] = data[curloc + i];
			}
			for (i = 0; i < seglen; i++) {
				if (data[curloc + i] == 0x82
				    || data[curloc + i] == 0x84)
					compatible = 1;	// 1-2mbit, fully compatible
				else if (data[curloc + i] ==
					 0x8B || data[curloc + i] == 0x96)
					compatible = 2;	// 5.5,11mbit, have to fake our way in.
				else if (data[curloc + i] & 0x80) {
					compatible = 0;
					break;
				}
			}
			rateset[j] = 0;
			break;
		case 3:	// DS set (current channel)
			channel = data[curloc];
			break;
		case 48:	// RSN(A) field- WPA enabled.
			wpamode = 1;
			break;

		}		// don't care about the others.
		curloc += seglen;
	} while (curloc < datalen);
	if (wpamode == 1)
		compatible = 0;
/*
	if (1) {
*/
	seglen = 0;
	segtype = 255;
	for (i = 0; i < WIFI_MAX_AP; i++) {
		if (Wifi_CmpMacAddr(wifi_data.aplist[i].bssid, data + 16)) {
			seglen++;
/*
				if(Spinlock_Acquire(wifi_data.aplist[i])==SPINLOCK_OK) {
*/
			wifi_data.aplist[i].timectr = 0;
			wifi_data.aplist[i].flags =
			    WFLAG_APDATA_ACTIVE |
			    (wepmode ? WFLAG_APDATA_WEP
			     : 0) | (fromsta ? 0 : WFLAG_APDATA_ADHOC);
			if (compatible == 1)
				wifi_data.aplist[i].
				    flags |= WFLAG_APDATA_COMPATIBLE;
			if (compatible == 2)
				wifi_data.aplist[i].
				    flags |= WFLAG_APDATA_EXTCOMPATIBLE;
			if (wpamode == 1)
				wifi_data.aplist[i].flags |= WFLAG_APDATA_WPA;
			wifi_data.aplist[i].maxrate = maxrate;

			Wifi_CopyMacAddr(wifi_data.aplist[i].macaddr, data + 10);	// src: +10
			if (ptr_ssid) {
				wifi_data.aplist[i].
				    ssid_len = data[ptr_ssid + 1];
				if (wifi_data.aplist[i].ssid_len > 32)
					wifi_data.aplist[i].ssid_len = 32;
				for (j = 0;
				     j < wifi_data.aplist[i].ssid_len; j++) {
					wifi_data.
					    aplist[i].
					    ssid[j] = data[ptr_ssid + 2 + j];
				}
				wifi_data.aplist[i].ssid[j] = 0;
			}
			if (wifi_data.curChannel == channel) {	// only use RSSI when we're on the right channel
				if (wifi_data.aplist[i].rssi_past[0] == 0) {	// min rssi is 2, heh.
					wifi_data.aplist[i].rssi_past[0]
					    = wifi_data.aplist[i].rssi_past[1]
					    = wifi_data.aplist[i].rssi_past[2]
					    = wifi_data.aplist[i].rssi_past[3]
					    = wifi_data.aplist[i].rssi_past[4]
					    = wifi_data.aplist[i].rssi_past[5]
					    = wifi_data.aplist[i].rssi_past[6]
					    = wifi_data.aplist[i].rssi_past[7]
					    = packetheader.rssi_ & 255;
				} else {
					for (j = 0; j < 7; j++) {
						wifi_data.aplist[i].rssi_past[j]
						    =
						    wifi_data.
						    aplist[i].rssi_past[j + 1];
					}
					wifi_data.aplist[i].rssi_past[7]
					    = packetheader.rssi_ & 255;
				}
			}
			wifi_data.aplist[i].channel = channel;
			for (j = 0; j < 16; j++)
				wifi_data.aplist[i].base_rates[j] = rateset[j];
/*
					Spinlock_Release(wifi_data.aplist[i]);
				} else {
					// couldn't update beacon - oh well :\ there'll be other beacons.
				}
*/
		} else {
			if (wifi_data.aplist[i].flags & WFLAG_APDATA_ACTIVE) {
				wifi_data.aplist[i].timectr++;
			} else {
				if (segtype == 255)
					segtype = i;
			}
		}
	}
	if (seglen == 0) {	// we couldn't find an existing record
		if (segtype == 255) {
			j = 0;
			for (i = 0; i < WIFI_MAX_AP; i++) {
				if (wifi_data.aplist[i].timectr > j) {
					j = wifi_data.aplist[i].timectr;
					segtype = i;
				}
			}
		}
		// stuff new data in
		i = segtype;
/*
			if(Spinlock_Acquire(wifi_data.aplist[i])==SPINLOCK_OK) {
*/
		Wifi_CopyMacAddr(wifi_data.aplist[i].bssid, data + 16);	// bssid: +16
		Wifi_CopyMacAddr(wifi_data.aplist[i].macaddr, data + 10);	// src: +10
		wifi_data.aplist[i].timectr = 0;
		wifi_data.aplist[i].flags =
		    WFLAG_APDATA_ACTIVE | (wepmode ?
					   WFLAG_APDATA_WEP
					   : 0) |
		    (fromsta ? 0 : WFLAG_APDATA_ADHOC);
		if (compatible == 1)
			wifi_data.aplist[i].flags |= WFLAG_APDATA_COMPATIBLE;
		if (compatible == 2)
			wifi_data.aplist[i].flags |= WFLAG_APDATA_EXTCOMPATIBLE;
		if (wpamode == 1)
			wifi_data.aplist[i].flags |= WFLAG_APDATA_WPA;
		wifi_data.aplist[i].maxrate = maxrate;

		if (ptr_ssid) {
			wifi_data.aplist[i].ssid_len = data[ptr_ssid + 1];
			if (wifi_data.aplist[i].ssid_len > 32)
				wifi_data.aplist[i].ssid_len = 32;
			for (j = 0; j < wifi_data.aplist[i].ssid_len; j++) {
				wifi_data.aplist[i].
				    ssid[j] = data[ptr_ssid + 2 + j];
			}
			wifi_data.aplist[i].ssid[j] = 0;
		}
		if (wifi_data.curChannel == channel) {	// only use RSSI when we're on the right channel
			wifi_data.aplist[i].
			    rssi_past[0] =
			    wifi_data.aplist[i].
			    rssi_past[1] =
			    wifi_data.aplist[i].
			    rssi_past[2] =
			    wifi_data.aplist[i].
			    rssi_past[3] =
			    wifi_data.aplist[i].
			    rssi_past[4] =
			    wifi_data.aplist[i].
			    rssi_past[5] =
			    wifi_data.aplist[i].
			    rssi_past[6] =
			    wifi_data.aplist[i].
			    rssi_past[7] = packetheader.rssi_ & 255;
		} else {
			wifi_data.aplist[i].rssi_past[0] = wifi_data.aplist[i].rssi_past[1] = wifi_data.aplist[i].rssi_past[2] = wifi_data.aplist[i].rssi_past[3] = wifi_data.aplist[i].rssi_past[4] = wifi_data.aplist[i].rssi_past[5] = wifi_data.aplist[i].rssi_past[6] = wifi_data.aplist[i].rssi_past[7] = 0;	// update rssi later.
		}
		wifi_data.aplist[i].channel = channel;
		for (j = 0; j < 16; j++)
			wifi_data.aplist[i].base_rates[j] = rateset[j];

		if (wifi_data.state & WIFI_STATE_ASSOCIATING)
			wifi_try_to_associate();

/*
				Spinlock_Release(wifi_data.aplist[i]);
			} else {
				// couldn't update beacon - oh well :\ there'll be other beacons.
			}
*/
	}
/*
	}
*/
	return WFLAG_PACKET_BEACON;
}

static int Wifi_ProcessReassocResponse(int macbase, int framelen)
{
	Wifi_RxHeader packetheader;
	int datalen, i;
	uchar data[64];
	ushort *fixed_params = (ushort *) (data + 24);
	uchar *tagged_params = data + 30;
	ushort status;
	uchar tag_length;

	DPRINT("wprr\n");
	
	USED(framelen);
	
	Wifi_MACCopy((ushort *) & packetheader, macbase, 0, 12);

	datalen = packetheader.byteLength;
	if (datalen > 64)
		datalen = 64;

	Wifi_MACCopy((ushort *) data, macbase, 12, (datalen + 1) & ~1);

	// if packet is not sent to us.
	if (!Wifi_CmpMacAddr(data + 4, wifi_data.MacAddr)) {
		goto out;
	}
	// if packet is not from the base station we're trying to associate to.
	if (!Wifi_CmpMacAddr(data + 16, wifi_data.bssid)) {
		goto out;
	}

	status = fixed_params[1];
	if (status == 0) {	// status code, 0==success
		WIFI_AIDS = fixed_params[2];
		WIFI_REG(0x2A) = fixed_params[2];
		// set max rate
		wifi_data.maxrate = 0xA;	// 1Mbit

		//assert(tagged_params[1] == 1);
		tag_length = tagged_params[1];
		for (i = 0; i < tag_length; i++) {
			uchar rate = tagged_params[2 + i];
			if (rate == 0x84 || rate == 0x04) {
				wifi_data.maxrate = 0x14;	// 2Mbit
			}
		}
		wifi_data.state |= WIFI_STATE_ASSOCIATED;

		SetLedState(LED_SHORTBLINK);

		wifi_data.state &=
		    ~(WIFI_STATE_ASSOCIATING | WIFI_STATE_CANNOTASSOCIATE);
/*
					if(wifi_data.authlevel==WIFI_AUTHLEVEL_AUTHENTICATED || wifi_data.authlevel==WIFI_AUTHLEVEL_DEASSOCIATED) {
						wifi_data.authlevel=WIFI_AUTHLEVEL_ASSOCIATED;
						wifi_data.authctr=0;
						

						
					}
*/
	} else {		// status code = failure!
		wifi_data.state |= WIFI_STATE_CANNOTASSOCIATE;
	}
      out:

	return WFLAG_PACKET_MGT;
}

static int Wifi_ProcessAuthenticationFrame(int macbase, int framelen)
{
	Wifi_RxHeader packetheader;
	int datalen;
	// size = 24+6+130
	uchar data[160];
	ushort *fixed_params = (ushort *) (data + 24);
	uchar *tagged_params = data + 30;
	ushort auth_algorithm;
	ushort auth_sequence;
	ushort status;

	DPRINT("wpaf\n");
	
	USED(framelen);
	
	Wifi_MACCopy((ushort *) & packetheader, macbase, 0, 12);

	datalen = packetheader.byteLength;
	if (datalen > 160)
		datalen = 160;

	Wifi_MACCopy((ushort *) data, macbase, 12, (datalen + 1) & ~1);

	// if packet is not sent to us.
	if (!Wifi_CmpMacAddr(data + 4, wifi_data.MacAddr)) {
		goto out;
	}
	// if packet is not from the base station we're trying to associate to.
	if (!Wifi_CmpMacAddr(data + 16, wifi_data.bssid)) {
		goto out;
	}

	auth_algorithm = fixed_params[0];
	auth_sequence = fixed_params[1];
	status = fixed_params[2];
	// open system
	if (auth_algorithm == 0) {
		// a good response
		if (auth_sequence == 2 && status == 0) {
			if (!(wifi_data.state & WIFI_STATE_AUTHENTICATED)) {
				wifi_data.state |= WIFI_STATE_AUTHENTICATED;
				wifi_data.authctr = 0;
				Wifi_SendAssocPacket();
			}
		}
	}
	// shared key
	else if (auth_algorithm == 1) {
		// Challenge text received
		if (auth_sequence == 2 && status == 0) {
			if (!(wifi_data.state & WIFI_STATE_AUTHENTICATED)) {
				Wifi_SendBackChallengeText(tagged_params);
			}
		}
		// Challenge successful
		else if (auth_sequence == 4 && status == 0) {
			if (!(wifi_data.state & WIFI_STATE_AUTHENTICATED)) {
				wifi_data.state |= WIFI_STATE_AUTHENTICATED;
				wifi_data.authctr = 0;
				Wifi_SendAssocPacket();
			}
		}
		// failed
		else {
			//Try open system auth
			Wifi_SendOpenSystemAuthPacket();
		}
	}
      out:
	return WFLAG_PACKET_MGT;
}

static int Wifi_ProcessDeAuthenticationFrame(int macbase, int framelen)
{
	Wifi_RxHeader packetheader;
	int datalen;
	uchar data[64];

	USED(framelen);
	
	Wifi_MACCopy((ushort *) & packetheader, macbase, 0, 12);

	datalen = packetheader.byteLength;
	if (datalen > 64)
		datalen = 64;

	Wifi_MACCopy((ushort *) data, macbase, 12, (datalen + 1) & ~1);

	// if packet is not sent to us.
	if (!Wifi_CmpMacAddr(data + 4, wifi_data.MacAddr)) {
		goto out;
	}
	// if packet is not from the base station we're trying to associate to.
	if (!Wifi_CmpMacAddr(data + 16, wifi_data.bssid)) {
		goto out;
	}
	// bad things! they booted us!.
	// back to square 1.
	if (wifi_data.curMode == WIFI_AP_ADHOC) {
		wifi_data.state |= WIFI_STATE_AUTHENTICATED;
		wifi_data.state &= ~(WIFI_STATE_ASSOCIATED);

		SetLedState(LED_LONGBLINK);

		Wifi_SendAssocPacket();
	} else {
		wifi_data.state &= ~(WIFI_STATE_ASSOCIATED | WIFI_STATE_AUTHENTICATED);

		SetLedState(LED_LONGBLINK);

		Wifi_SendOpenSystemAuthPacket();
	}

      out:
	return WFLAG_PACKET_MGT;
}

/*
 * Strip the 14 byte ether header packet.
 * Add 48 bytes if wifi headers
 */
void wifi_send_ether_packet(ushort length, uchar * data)
{
	int packetlen, framelen, hdrlen;
	ushort framehdr[6 + 12 + 2];
	int i;

	for (i = 0; i < 20; i++)
		framehdr[i] = 0;

	length += 3;
	length &= ~0x03;

	/* raw data + new frame header */
	framelen = length - 14 + 8;

	framehdr[4] = 0;	// rate, will be filled in by the arm7.
	framehdr[4] = wifi_data.maxrate;
	hdrlen = 18;
	framehdr[7] = 0;

	if (wifi_data.curMode == WIFI_AP_ADHOC) {
		framehdr[6] = 0x0008;
		Wifi_CopyMacAddr(framehdr + 14, wifi_data.bssid);
		Wifi_CopyMacAddr(framehdr + 11, wifi_data.MacAddr);
		Wifi_CopyMacAddr(framehdr + 8, ((uchar *) data));
	} else {
		framehdr[6] = 0x0108;
		Wifi_CopyMacAddr(framehdr + 8, wifi_data.bssid);
		Wifi_CopyMacAddr(framehdr + 11, wifi_data.MacAddr);
		Wifi_CopyMacAddr(framehdr + 14, ((uchar *) data));
	}
	if (wifi_data.wepmode != WEPMODE_NONE) {
		framehdr[6] |= 0x4000;
		hdrlen = 20;
		((ulong *)data)[9]=((WIFI_RANDOM ^ (WIFI_RANDOM<<7) ^ (WIFI_RANDOM<<15))&0x0FFF) | (wifi_data.wepkeyid<<30); // I'm lazy and certainly haven't done this to spec.
	}

	framehdr[17] = 0;

	/*
	 * framelen = 8 byte LLC header + data len
	 * hdrlen = tx header (6 words) + ieee header (12 or 14 (wep) words)
	 * 
	 * If wep, add 4 bytes to end of body for IVC
	 * Add 4 bytes to end of everything for CRC
	 */
	packetlen = framelen + hdrlen * 2 - 12 + 4;
	if (wifi_data.wepmode != WEPMODE_NONE) {
		packetlen += 4;
	}
	framehdr[5] = packetlen;

	Wifi_MACWrite(framehdr, TX1_MAC_START, 0, hdrlen * 2);

	framehdr[0] = 0xAAAA;
	framehdr[1] = 0x0003;
	framehdr[2] = 0x0000;
	framehdr[3] = ((ushort *) data)[6];	// frame type

	Wifi_MACWrite(framehdr, TX1_MAC_START, hdrlen * 2, 4 * 2);

	Wifi_MACWrite((ushort *) (data + 14), TX1_MAC_START, hdrlen * 2 + 4 * 2,
			length - 14);

	/* Scheduled to be sent */
	wifi_data.state |= WIFI_STATE_TXPENDING;
	WIFI_REG(0x80A0) = TX1_SETUP_VAL;
/* XOXOXO -- delay for beacon
	WIFI_REG(0x80AE) = 1;
*/

	/* stats update */
	wifi_data.stats[WF_STAT_TXPKTS]++;
	wifi_data.stats[WF_STAT_TXBYTES] += packetlen + 12 - 4;
	wifi_data.stats[WF_STAT_TXDATABYTES] += packetlen - 4;
}

void wifi_ap_query(ushort start_stop)
{
	if (start_stop)
		wifi_data.state |= WIFI_STATE_APQUERYPEND;
	else
		wifi_data.state &= ~WIFI_STATE_APQUERYPEND;

	//nds_fifo_send(FIFO_WIFI_CMD(FIFO_WIFI_CMD_AP_QUERY, 0));
}

void wifi_start_scan(void)
{
	TimerReg *t = TMRREG + WIFItimer;

	wifi_data.state |= WIFI_STATE_CHANNEL_SCANNING;
	wifi_data.scanChannel = 1;
	Wifi_SetChannel(1);

	t->data = TIMER_BASE(Tmrdiv1024) / (1000 / WIFI_CHANNEL_SCAN_DWEL);
	t->ctl = Tmrdiv1024 | Tmrena | Tmrirq;
	intrunmask(TIMERWIFIbit, 0);
}

static void wifi_bump_scan(void)
{
	TimerReg *t = TMRREG + WIFItimer;

	if (wifi_data.scanChannel == 13) {
		intrmask(TIMERWIFIbit, 0);
		wifi_data.state &= ~WIFI_STATE_CHANNEL_SCANNING;
		t->ctl = 0;
		Wifi_SetChannel(wifi_data.reqChannel);
		//nds_fifo_send(FIFO_WIFI_CMD(FIFO_WIFI_CMD_SCAN, 1));
	} else {
		wifi_data.scanChannel++;
		Wifi_SetChannel(wifi_data.scanChannel);
	}
}

static void wifi_timer_handler(void*)
{
	TimerReg *t = TMRREG + WIFItimer;

	if (wifi_data.state & WIFI_STATE_CHANNEL_SCANNING) {
		wifi_bump_scan();
	} else {
		intrmask(TIMERWIFIbit, 0);
		t->ctl = 0;
	}
	intrclear(TIMERWIFIbit, 0);
}

void Wifi_SetAPMode(enum WIFI_AP_MODE mode)
{
	wifi_data.curMode = mode;
}

void Wifi_GetAPMode(void)
{
	//nds_fifo_send(
	//	FIFO_WIFI_CMD(FIFO_WIFI_CMD_GET_AP_MODE, wifi_data.curMode));
}
