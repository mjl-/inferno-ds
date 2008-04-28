/*---------------------------------------------------------------------------------
	$Id: touch.c,v 1.20 2007/06/25 20:23:35 wntrmute Exp $
	Touch screen control for the ARM7
	Copyright (C) 2005
		Michael Noland (joat)
		Jason Rogers (dovoto)
		Dave Murphy (WinterMute)
	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any
	damages arising from the use of this software.
	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:
	1.	The origin of this software must not be misrepresented; you
			must not claim that you wrote the original software. If you use
			this software in a product, an acknowledgment in the product
			documentation would be appreciated but is not required.
	2.	Altered source versions must be plainly marked as such, and
			must not be misrepresented as being the original software.
	3.	This notice may not be removed or altered from any source
			distribution.
---------------------------------------------------------------------------------*/
#include <u.h>
#include "../mem.h"
#include "../io.h"
#include "nds.h"
#include "dat.h"

static u8 wastouched = 0;
static u8 nrange1 = 0;
static u8 nrange2 = 0;
static u8 range = 20;
static u8 minrange = 20;

static long abs(long a);

static long
abs(long a)
{
	return a>0 ? a : -a;
}

u8
chkstylus(void)
{
	busywait();
	REG_SPICNT = Spien|Spi2mhz|Spitouch|Spicont; //0x8A01;
	REG_SPIDATA = Tscgettemp1;
	busywait();
	REG_SPIDATA = 0;
	busywait();
	REG_SPICNT = Spien|Spi2mhz|Spitouch; //0x8201;
	REG_SPIDATA = 0;
	busywait();
	if(wastouched){
		if( !(REG_KEYXY & 0x40) )
			return 1;
		else{
			REG_SPICNT = Spien|Spi2mhz|Spitouch|Spicont;
			REG_SPIDATA = Tscgettemp1;
			busywait();
			REG_SPIDATA = 0;
			busywait();
			REG_SPICNT = Spien|Spi2mhz|Spitouch;
			REG_SPIDATA = 0;
			busywait();
			return !(REG_KEYXY & 0x40) ? 2 : 0;
		}
	}else{
		return !(REG_KEYXY & 0x40) ? 1 : 0;
	}
}
uint16 
touchRead(uint32 cmd) 
{
	uint16 res, res2;
	uint32 oldIME = INTREG->ime;
	INTREG->ime=0;
	busywait();
	REG_SPICNT=Spien|Spi2mhz|Spitouch|Spicont; //0x8A01;
	REG_SPIDATA=cmd;
	busywait();
	REG_SPIDATA=0;
	busywait();
	res=REG_SPIDATA;
	REG_SPICNT=Spien|0x201;
	REG_SPIDATA=0;
	busywait();
	res2=REG_SPIDATA>>3;
	INTREG->ime=oldIME;
	return ((res & 0x7F) << 5) | res2;
}
uint32 touchReadTemperature(int * t1, int * t2) {
	*t1 = touchRead(Tscgettemp1);
	*t2 = touchRead(Tscgettemp2);
	return 8490 * (*t2 - *t1) - 273*4096;
}

int16 readtsc(uint32 cmd, int16 *dmax, u8 *err){
	int16 values[5];
	int32 aux1, aux2, aux3, dist, dist2, res = 0;
	u8 i, j, k;
	*err = 1;
	busywait();
	REG_SPICNT=Spien|Spi2mhz|Spitouch|Spicont;
	REG_SPIDATA=cmd;
	busywait();
	for(i=0; i<5; i++){
		REG_SPIDATA = 0;
		busywait();
		aux1 = REG_SPIDATA;
		aux1 = aux1 & 0xFF;
		aux1 = aux1 << 16;
		aux1 = aux1 >> 8;
		values[4-i] = aux1;
		REG_SPIDATA = cmd;
		busywait();
		aux1 = REG_SPIDATA;
		aux1 = aux1 & 0xFF;
		aux1 = aux1 << 16;
		aux1 = values[4-i] | (aux1 >> 16);
		values[4-i] = ((aux1 & 0x7FF8) >> 3);
	}
	REG_SPICNT = Spien | Spi2mhz | Spitouch;
	REG_SPIDATA = 0;
	busywait();
	dist = 0;
	for(i=0; i<4; i++){
		aux1 = values[i];
		for(j=i+1; j<5; j++){
			aux2 = values[j];
			aux2 = abs(aux1 - aux2);
			if(aux2>dist) dist = aux2;
		}
	}
	*dmax = dist;
	for(i=0; i<3; i++){
		aux1 = values[i];
		for(j=i+1; j<4; j++){
			aux2 = values[j];
			dist = abs(aux1 - aux2);
			if( dist <= range ){
				for(k=j+1; k<5; k++){
					aux3 = values[k];
					dist2 = abs(aux1 - aux3);
					if( dist2 <= range ){
						res = aux2 + (aux1 << 1);
						res = res + aux3;
						res = res >> 2;
						res = res & (~7);
						*err = 0;
						break;
					}
				}
			}
		}
	}
	if((*err) == 1){
		res = values[0] + values[4];
		res = res >> 1;
		res = res & (~7);
	}
	return (res & 0xFFF);
}

void 
UpdateRange(uint8 *r, int16 lastdmax, u8 data_error, u8 wastouched)
{
	if(wastouched != 0){
		if( data_error == 0){
			nrange2 = 0;
			if( lastdmax >= ((*r) >> 1)){
				nrange1 = 0;
			}else{
				nrange1++;
				if(nrange1 >= 4){
					nrange1 = 0;
					if((*r) > minrange){
						(*r)--;
						nrange2 = 3;
					}
				}
			}
		}else{
			nrange1 = 0;
			nrange2++;
			if(nrange2 >= 4){
				nrange2 = 0;
				if(*r < 35){
					(*r)++;
				}
			}
		}
	}else{
		nrange2 = 0;
		nrange1 = 0;
	}
}

void
touchReadXY(touchPosition *tp) 
{
	int16 dmaxy, dmaxx, dmax;
	u8 error, errloc, usedstylus, i;
	uint32 oldIME;
	int x, y;
	PERSONAL_DATA *pd=PersonalData;
	s16 px, py;

	static char tscinit=0;
	static int xscale, yscale;
	static int xoffset, yoffset;

	if (!tscinit){
		xscale = ((pd->calX2px - pd->calX1px) << 19) / ((pd->calX2) - (pd->calX1));
		yscale = ((pd->calY2px - pd->calY1px) << 19) / ((pd->calY2) - (pd->calY1));
                
		xoffset = ((pd->calX1 + pd->calX2) * xscale  - ((pd->calX1px + pd->calX2px) << 19))/2;
		yoffset = ((pd->calY1 + pd->calY2) * yscale  - ((pd->calY1px + pd->calY2px) << 19))/2;
		tscinit = 1;
	}

	oldIME = INTREG->ime;
	INTREG->ime = 0;
	usedstylus = chkstylus();
	if(usedstylus){
		errloc=0;
		tp->z1=readtsc(TscgetZ1|1,&dmax,&error);
		tp->z2=readtsc(TscgetZ2|1,&dmax,&error);
		tp->x=readtsc(TscgetX|1,&dmaxx,&error);
		if(error==1)
			errloc+=1;
		tp->y=readtsc(TscgetY|1,&dmaxy,&error);
		if(error==1)
			errloc+=2;

		REG_SPICNT=Spien|Spi2mhz|Spitouch|Spicont;
		for(i=0; i<12; i++){
			REG_SPIDATA = 0;
			busywait();
		}
		REG_SPICNT = Spien|Spi2mhz|Spitouch;
		REG_SPIDATA = 0;
		busywait();
		if(usedstylus == 2) 
			errloc = 3;
		switch(chkstylus()){
		case 0:
			wastouched = 0;
			break;
		case 1:
			wastouched = 1;
			if(dmaxx > dmaxy)
				dmax = dmaxx;
			else
				dmax = dmaxy;
			break;
		case 2:
			wastouched = 0;
			errloc = 3;
			break;
		}

//		px = tp->x;
//		py = tp->y;
		px=(tp->x * xscale - xoffset + xscale / 2)>>19;
		py=(tp->y * yscale - yoffset + yscale / 2)>>19;

		if(px<0)
			px=0;
		if(py<0)
			py=0;
		if(px > Scrwidth-1)
			px=Scrwidth-1;
		if(py > Scrheight-1)
			py=Scrheight-1;

		tp->px=px;
		tp->py=py; 
	}else{
		errloc = 3;
		tp->x = 0;
		tp->y = 0;
		wastouched = 0;
	}
	UpdateRange(&range, dmax, errloc, wastouched);
	INTREG->ime = oldIME;
	return;
}
