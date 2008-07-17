#define Scrwidth	256
#define Scrheight	192

#define Tscgettemp1    0x84
#define TscgetY        0x90
#define Tscgetbattery  0xA4
#define TscgetZ1       0xB4
#define TscgetZ2       0xC4
#define TscgetX        0xD0
#define Tscgetaux      0xE4
#define Tscgettemp2    0xF4

typedef struct touchPosition touchPosition;
struct touchPosition {
	short	x;
	short	y;
	short	px;
	short	py;
	short	z1;
	short	z2;
};

void touchReadXY(touchPosition *tp);

ushort touchRead(ulong cmd);
ulong touchReadTemperature(int * t1, int * t2);
short readtsc(ulong cmd, short *dmax, uchar *err);

