#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "interp.h"

#include "ds.root.h"

ulong ndevs = 12;
extern Dev rootdevtab;
extern Dev consdevtab;
extern Dev mntdevtab;
extern Dev progdevtab;
Dev* devtab[12]={
	&rootdevtab,
	&consdevtab,
	&mntdevtab,
	&progdevtab,
	nil,
};

void links(void){
}

extern void sysmodinit(void);
void modinit(void){
	sysmodinit();
}

	int main_pool_pcnt = 60;
	int heap_pool_pcnt = 40;
	int image_pool_pcnt = 0;
	int cflag = 0;
	int consoleprint = 1;
	int redirectconsole = 1;
	char debug_keys = 1;
	int panicreset = 0;
	void pseudoRandomBytes(uchar *a, int n){memset(a, 0, n);}
	int srvf2c(){return -1;}	/* dummy */
	Type *Trdchan;
	Type *Twrchan;
char* conffile = "ds";
ulong kerndate = KERNDATE;
