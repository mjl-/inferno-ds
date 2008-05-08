#include <u.h>
#include "../mem.h"
#include "../io.h"
#include "nds.h"

void setytrig(int Yvalue) {
	LCDREG->lcsr = (LCDREG->lcsr & 0x007F ) | (Yvalue << 8) | (( Yvalue & 0x100 ) >> 2) ;
}

void poweron(int on) { POWERREG->pcr |= on;}

void powerSET(int on) { POWERREG->pcr = on;}
void powerOFF(int off) { POWERREG->pcr &= ~off;}
void lcdSwap(void) { POWERREG->pcr ^= POWER_SWAP_LCDS; }
void lcdMainOnTop(void) { POWERREG->pcr |= POWER_SWAP_LCDS; }
void lcdMainOnBottom(void) { POWERREG->pcr &= ~POWER_SWAP_LCDS; }
