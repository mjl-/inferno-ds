#include <u.h>
#include "../mem.h"
#include "nds.h"

void
busywait() 
{
	while (REG_SPICNT & SPI_BUSY)
		swiDelay(1);
}