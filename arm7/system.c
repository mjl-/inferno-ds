#include <u.h>
#include "../mem.h"
#include "nds.h"

void setytrig(int Yvalue) {
	REG_DISPSTAT = (REG_DISPSTAT & 0x007F ) | (Yvalue << 8) | (( Yvalue & 0x100 ) >> 2) ;
}

void poweron(int on) { REG_POWERCNT |= on;}

void powerSET(int on) { REG_POWERCNT = on;}
void powerOFF(int off) { REG_POWERCNT &= ~off;}
void lcdSwap(void) { REG_POWERCNT ^= POWER_SWAP_LCDS; }
void lcdMainOnTop(void) { REG_POWERCNT |= POWER_SWAP_LCDS; }
void lcdMainOnBottom(void) { REG_POWERCNT &= ~POWER_SWAP_LCDS; }