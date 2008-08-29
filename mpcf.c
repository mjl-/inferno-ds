#include	"u.h"
#include	"io.h"
#include	"mem.h"
#include	"arm7/card.h"

// CF Addresses
#define REG_CF_STS		((volatile ushort*)0x098C0000)	// Status of the CF Card / Device control
#define REG_CF_CMD		((volatile ushort*)0x090E0000)	// Commands sent to control chip and status return
#define REG_CF_ERR		((volatile ushort*)0x09020000)	// Errors / Features

#define REG_CF_SEC		((volatile ushort*)0x09040000)	// Number of sector to transfer
#define REG_CF_LBA1		((volatile ushort*)0x09060000)	// 1st byte of sector address
#define REG_CF_LBA2		((volatile ushort*)0x09080000)	// 2nd byte of sector address
#define REG_CF_LBA3		((volatile ushort*)0x090A0000)	// 3rd byte of sector address
#define REG_CF_LBA4		((volatile ushort*)0x090C0000)	// last nibble of sector address | 0xE0

#define REG_CF_DATA		((volatile ushort*)0x09000000)	// Pointer to buffer of CF data transered from card

// CF Card status
#define CF_STS_INSERTED		0x50
#define CF_STS_REMOVED		0x00
#define CF_STS_READY		0x58

#define CF_STS_DRQ			0x08
#define CF_STS_BUSY			0x80

// CF Card commands
#define CF_CMD_LBA			0xE0
#define CF_CMD_READ			0x20
#define CF_CMD_WRITE		0x30

#define CF_CARD_TIMEOUT	10000000

static int
mpcf_init(void)
{
	// See if there is a read/write register
	ushort temp = *REG_CF_LBA1;
	*REG_CF_LBA1 = (~temp & 0xFF);
	temp = (~temp & 0xFF);
	if (!(*REG_CF_LBA1 == temp)) {
		return 0;
	}
	// Make sure it is 8 bit
	*REG_CF_LBA1 = 0xAA55;
	if (*REG_CF_LBA1 == 0xAA55) {
		return 0;
	}
	return 1;
}

static int
mpcf_isin(void)
{
	// Change register, then check if value did change
	*REG_CF_STS = CF_STS_INSERTED;
	return ((*REG_CF_STS & 0xff) == CF_STS_INSERTED);
}


static int
mpcf_clrstat(void)
{
	int i;
	
	// Wait until CF card is finished previous commands
	i=0;
	while ((*REG_CF_CMD & CF_STS_BUSY) && (i < CF_CARD_TIMEOUT)) {
		i++;
	}
	
	// Wait until card is ready for commands
	i = 0;
	while ((!(*REG_CF_STS & CF_STS_INSERTED)) && (i < CF_CARD_TIMEOUT)) {
		i++;
	}
	if (i >= CF_CARD_TIMEOUT)
		return 0;

	return 1;
}

static int
mpcf_read(ulong sector, ulong numSectors, void* buffer)
{
	int i;

	ushort *buff = (ushort*)buffer;
	uchar *buff_uchar = (uchar*)buffer;
	int temp;

	// Wait until CF card is finished previous commands
	i=0;
	while ((*REG_CF_CMD & CF_STS_BUSY) && (i < CF_CARD_TIMEOUT)) {
		i++;
	}
	
	// Wait until card is ready for commands
	i = 0;
	while ((!(*REG_CF_STS & CF_STS_INSERTED)) && (i < CF_CARD_TIMEOUT)) {
		i++;
	}
	if (i >= CF_CARD_TIMEOUT)
		return 0;
	
	// Set number of sectors to read
	*REG_CF_SEC = (numSectors < 256 ? numSectors : 0);	// Read a maximum of 256 sectors, 0 means 256	
	
	// Set read sector
	*REG_CF_LBA1 = sector & 0xFF;						// 1st byte of sector number
	*REG_CF_LBA2 = (sector >> 8) & 0xFF;					// 2nd byte of sector number
	*REG_CF_LBA3 = (sector >> 16) & 0xFF;				// 3rd byte of sector number
	*REG_CF_LBA4 = ((sector >> 24) & 0x0F )| CF_CMD_LBA;	// last nibble of sector number
	
	// Set command to read
	*REG_CF_CMD = CF_CMD_READ;
	
	
	while (numSectors--)
	{
		// Wait until card is ready for reading
		i = 0;
		while (((*REG_CF_STS & 0xff)!= CF_STS_READY) && (i < CF_CARD_TIMEOUT))
		{
			i++;
		}
		if (i >= CF_CARD_TIMEOUT)
			return 0;
		
		// Read data
		i=256;
		if ((ulong)buff_uchar & 0x01) {
			while(i--)
			{
				temp = *((volatile ushort*)0x09000000);
				*buff_uchar++ = temp & 0xFF;
				*buff_uchar++ = temp >> 8;
			}
		} else {
			while(i--)
				*buff++ = *((volatile ushort*)0x09000000); 
		}
	}

	return 1;
}

static int
mpcf_write(ulong sector, ulong numSectors, void* buffer)
{
	int i;

	ushort *buff = (ushort*)buffer;
	uchar *buff_uchar = (uchar*)buffer;
	int temp;
	
	// Wait until CF card is finished previous commands
	i=0;
	while ((*REG_CF_CMD & CF_STS_BUSY) && (i < CF_CARD_TIMEOUT))
	{
		i++;
	}
	
	// Wait until card is ready for commands
	i = 0;
	while ((!(*REG_CF_STS & CF_STS_INSERTED)) && (i < CF_CARD_TIMEOUT))
	{
		i++;
	}
	if (i >= CF_CARD_TIMEOUT)
		return 0;
	
	// Set number of sectors to write
	*REG_CF_SEC = (numSectors < 256 ? numSectors : 0);	// Write a maximum of 256 sectors, 0 means 256	
	
	// Set write sector
	*REG_CF_LBA1 = sector & 0xFF;							// 1st byte of sector number
	*REG_CF_LBA2 = (sector >> 8) & 0xFF;					// 2nd byte of sector number
	*REG_CF_LBA3 = (sector >> 16) & 0xFF;					// 3rd byte of sector number
	*REG_CF_LBA4 = ((sector >> 24) & 0x0F )| CF_CMD_LBA;	// last nibble of sector number
	
	// Set command to write
	*REG_CF_CMD = CF_CMD_WRITE;
	
	while (numSectors--)
	{
		// Wait until card is ready for writing
		i = 0;
		while (((*REG_CF_STS & 0xff) != CF_STS_READY) && (i < CF_CARD_TIMEOUT))
		{
			i++;
		}
		if (i >= CF_CARD_TIMEOUT)
			return 0;
		
		// Write data
		i=256;
		if ((ulong)buff_uchar & 0x01) {
			while(i--)
			{
				temp = *buff_uchar++;
				temp |= *buff_uchar++ << 8;
				*((volatile ushort*)0x09000000) = temp;
			}
		} else {
		while(i--)
			*((volatile ushort*)0x09000000) = *buff++; 
		}
	}
	
	return 1;
}

static Ioifc io_mpcf = {
	"MPCF",
	Cread|Cwrite|Cslotgba,

	mpcf_init,
	mpcf_isin,
	(void*)mpcf_read,
	(void*)mpcf_write,
	mpcf_clrstat,
	mpcf_clrstat,
};

void
mpcflink(void)
{
	addioifc(&io_mpcf);
}
