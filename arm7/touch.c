#include <u.h>
#include "../mem.h"
#include "../io.h"
#include "dat.h"
#include "spi.h"
#include "touch.h"

static uchar wastouched = 0;
static uchar nrange1 = 0;
static uchar nrange2 = 0;
static uchar range = 20;
static uchar minrange = 20;

static long abs(long a);

static long
abs(long a)
{
	return a>0 ? a : -a;
}

uchar
chkstylus(void)
{
	busywait();
	SPIREG->ctl = Spiena|Spi2mhz|SpiDevtouch|Spicont; //0x8A01;
	SPIREG->data = Tscgettemp1;
	busywait();
	SPIREG->data = 0;
	busywait();
	SPIREG->ctl = Spiena|Spi2mhz|SpiDevtouch; //0x8201;
	SPIREG->data = 0;
	busywait();
	if(wastouched){
		if( !(KEYREG->xy & 0x40) )
			return 1;
		else{
			SPIREG->ctl = Spiena|Spi2mhz|SpiDevtouch|Spicont;
			SPIREG->data = Tscgettemp1;
			busywait();
			SPIREG->data = 0;
			busywait();
			SPIREG->ctl = Spiena|Spi2mhz|SpiDevtouch;
			SPIREG->data = 0;
			busywait();
			return !(KEYREG->xy & 0x40) ? 2 : 0;
		}
	}else{
		return !(KEYREG->xy & 0x40) ? 1 : 0;
	}
}
ushort
touchRead(ulong cmd) 
{
	ushort res, res2;
	ulong oldIME = INTREG->ime;
	INTREG->ime=0;
	busywait();
	SPIREG->ctl=Spiena|Spi2mhz|SpiDevtouch|Spicont; //0x8A01;
	SPIREG->data=cmd;
	busywait();
	SPIREG->data=0;
	busywait();
	res=SPIREG->data;
	SPIREG->ctl=Spiena|0x201;
	SPIREG->data=0;
	busywait();
	res2=SPIREG->data>>3;
	INTREG->ime=oldIME;
	return ((res & 0x7F) << 5) | res2;
}

ulong
touchReadTemperature(int * t1, int * t2) {
	*t1 = touchRead(Tscgettemp1);
	*t2 = touchRead(Tscgettemp2);
	return 8490 * (*t2 - *t1) - 273*4096;
}

short
readtsc(ulong cmd, short *dmax, uchar *err){
	short values[5];
	int aux1, aux2, aux3, dist, dist2, res = 0;
	char i, j, k;
	*err = 1;
	busywait();
	SPIREG->ctl=Spiena|Spi2mhz|SpiDevtouch|Spicont;
	SPIREG->data=cmd;
	busywait();
	for(i=0; i<5; i++){
		SPIREG->data = 0;
		busywait();
		aux1 = SPIREG->data;
		aux1 = aux1 & 0xFF;
		aux1 = aux1 << 16;
		aux1 = aux1 >> 8;
		values[4-i] = aux1;
		SPIREG->data = cmd;
		busywait();
		aux1 = SPIREG->data;
		aux1 = aux1 & 0xFF;
		aux1 = aux1 << 16;
		aux1 = values[4-i] | (aux1 >> 16);
		values[4-i] = ((aux1 & 0x7FF8) >> 3);
	}
	SPIREG->ctl = Spiena | Spi2mhz | SpiDevtouch;
	SPIREG->data = 0;
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
UpdateRange(uchar *r, short lastdmax, uchar data_error, uchar wastouched)
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
	short dmaxy, dmaxx, dmax;
	uchar error, errloc, usedstylus, i;
	ulong oldIME;
	UserInfo *pu=UserInfoAddr;
	short px, py;

	static char tscinit=0;
	static int xscale, yscale;
	static int xoffset, yoffset;

	if (!tscinit){
		xscale = ((pu->calX2px - pu->calX1px) << 19) / ((pu->calX2) - (pu->calX1));
		yscale = ((pu->calY2px - pu->calY1px) << 19) / ((pu->calY2) - (pu->calY1));
                
		xoffset = ((pu->calX1 + pu->calX2) * xscale  - ((pu->calX1px + pu->calX2px) << 19))/2;
		yoffset = ((pu->calY1 + pu->calY2) * yscale  - ((pu->calY1px + pu->calY2px) << 19))/2;
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

		SPIREG->ctl=Spiena|Spi2mhz|SpiDevtouch|Spicont;
		for(i=0; i<12; i++){
			SPIREG->data = 0;
			busywait();
		}
		SPIREG->ctl = Spiena|Spi2mhz|SpiDevtouch;
		SPIREG->data = 0;
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
