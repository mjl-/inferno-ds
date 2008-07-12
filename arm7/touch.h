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

void touchReadXY(touchPosition *tp);

uint16 touchRead(uint32 cmd);
uint32 touchReadTemperature(int * t1, int * t2);
int16 readtsc(uint32 cmd, int16 *dmax, u8 *err);

