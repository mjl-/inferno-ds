
#define Tscgettemp1	0x84
#define TscgetY		0x90
#define Tscgetbattery	0xA4
#define TscgetZ1	0xB4
#define TscgetZ2	0xC4
#define TscgetX		0xD0
#define Tscgetmic12	0xE4	// Touchscreen cmd sample format: 8, 12 bits
#define Tscgetmic8	0xEC
#define Tscgettemp2	0xF4

typedef struct TouchPos TouchPos;
struct TouchPos {
	short	x;
	short	y;
	short	px;
	short	py;
	short	z1;
	short	z2;
};

void touchReadXY(TouchPos *tp);

ushort touchRead(ulong cmd);
ulong touchReadTemperature(int * t1, int * t2);
short readtsc(ulong cmd, short *dmax, uchar *err);

