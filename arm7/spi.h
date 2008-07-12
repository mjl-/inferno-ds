// 'Networking'
#define REG_RCNT	(*(vuint16*)0x04000134)
#define REG_KEYXY	(*(vuint16*)0x04000136)
#define RTC_CR		(*(vuint16*)0x04000138)
#define RTC_CR8		(*( vuint8*)0x04000138)

#define REG_SIOCNT	(*(vuint16*)0x04000128)

#define SIO_DATA8       (*(vuint8*)0x0400012A)
#define SIO_DATA32      (*(vuint32*)0x04000120)


// Fixme: Does the hardware still support 16 bit comms mode?
// BIOS makes use of 32 bit mode, so some regs still exist
#define SIO_MULTI_0     (*(vuint16*)0x04000120)
#define SIO_MULTI_1     (*(vuint16*)0x04000122)
#define SIO_MULTI_2     (*(vuint16*)0x04000124)
#define SIO_MULTI_3     (*(vuint16*)0x04000126)
#define SIO_MULTI_SEND  (*(vuint16*)0x0400012A)

/*
 * Power management registers
 */
enum power_reg
{
	POWER_CONTROL		= 0,
	POWER_BATTERY		= 1,
	POWER_MIC_CONTROL	= 2,
	POWER_MIC_GAIN		= 3,
	POWER_BACKLIGHT		= 4,

	PM_READ_REGISTER 	= (1<<7),
};

// power control register bits
enum
{
	POWER0_SOUND_AMP	= (1<<0),	/* Power the sound hardware (needed to hear stuff in GBA mode too) */
	POWER0_SOUND_SPK	= (1<<1),	/* Power the main speakers, headphone output will still work. */
	POWER0_LOWER_BACKLIGHT	= (1<<2),	/* Enable the top backlight if set */
	POWER0_UPPER_BACKLIGHT	= (1<<3),	/* Enable the bottom backlight if set */
	POWER0_LED_BLINK	= (1<<4),
	POWER0_LED_FAST		= (1<<5),
	POWER0_SYSTEM_POWER	= (1<<6),	/* Turn the power *off* if set */
};

// PM control register bits - LED control
#define PM_LED_CONTROL(m)    ((m)<<4)   ?
#define PM_LED_ON     (0<<4)   /* Steady on */
#define PM_LED_SLEEP  (2<<4)   /* Blinking, mostly off */
#define PM_LED_BLINK  (3<<4)   /* Blinking, mostly on */

enum {
	Brightlow,
	Brightmedium,
	Brighthigh,
	Brightmax,
	Brightmask =	0x3,
};

#define PM_AMP_OFFSET  2
#define PM_AMP_ON      1
#define PM_AMP_OFF     0
#define FIRMWARE_WREN 0x06
#define FIRMWARE_WRDI 0x04
#define FIRMWARE_RDID 0x9F
#define FIRMWARE_RDSR 0x05
#define FIRMWARE_READ 0x03
#define FIRMWARE_PW   0x0A
#define FIRMWARE_PP   0x02
#define FIRMWARE_FAST 0x0B
#define FIRMWARE_PE   0xDB
#define FIRMWARE_SE   0xD8
#define FIRMWARE_DP   0xB9
#define FIRMWARE_RDP  0xAB

/*
 Warning: These functions use the SPI chain, and are thus 'critical'
 sections, make sure to disable interrupts during the call if you've
 got a VBlank IRQ polling the touch screen, etc...
 */

uchar power_read(int reg);
uchar power_write(int reg, int cmd);

void busywait(void);

void read_firmware(ulong, void *, ulong);

