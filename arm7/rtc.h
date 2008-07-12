// RTC registers
#define WRITE_STATUS_REG1  0x60
#define READ_STATUS_REG1   0x61

#define STATUS_POC       (1<<7)	// RO, cleared by reading (1 if just powered on)
#define STATUS_BLD       (1<<6)	// RO, cleared by reading (1 if power dropped below the safety threshold)
#define STATUS_INT2      (1<<5)	// RO, INT2 has occured
#define STATUS_INT1      (1<<4)	// RO, INT1 has occured
#define STATUS_SC1       (1<<3)	// R/W scratch bit
#define STATUS_SC0       (1<<2)	// R/W scratch bit
#define STATUS_24HRS     (1<<1)	// 24 hour mode when 1, 12 hour mode when 0
#define STATUS_RESET     (1<<0)	// WO, reset when 1 written

#define WRITE_STATUS_REG2  0x62
#define READ_STATUS_REG2   0x63

#define STATUS_TEST      (1<<7)	// R/W Test Mode
#define STATUS_INT2AE    (1<<6)	// INT2 enable
#define STATUS_SC3       (1<<5)	// R/W scratch bit
#define STATUS_SC2       (1<<4)	// R/W scratch bit

#define STATUS_32kE      (1<<3)	// Interrupt mode bits
#define STATUS_INT1AE    (1<<2)   
#define STATUS_INT1ME    (1<<1)   
#define STATUS_INT1FE    (1<<0)   

// full 7 bytes for time and date
#define WRITE_DATA_REG1	0x64
#define READ_DATA_REG1	0x65

// last 3 bytes of current time
#define WRITE_TIME    0x66
#define READ_TIME	  0x67

#define WRITE_INT_REG1     0x68
#define READ_INT_REG1      0x69

#define READ_INT_REG2      0x6A
#define WRITE_INT_REG2     0x6B

// clock-adjustment register
#define READ_CLOCK_ADJUST_REG  0x6C
#define WRITE_CLOCK_ADJUST_REG 0x6D

#define READ_FREE_REG      0x6E
#define WRITE_FREE_REG     0x6F

ulong nds_get_time7(void);
void nds_set_time7(ulong);

