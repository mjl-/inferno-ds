#include <u.h>
#include "../mem.h"
#include "nds.h"

TransferRegion  *getIPC(void) {
	return (TransferRegion*)(0x027FF000);
}
static  void IPC_SendSync(unsigned int sync) {
	REG_IPC_SYNC = (REG_IPC_SYNC & 0xf0ff) | (((sync) & 0x0f) << 8) | IPC_SYNC_IRQ_REQUEST;
}


static  int IPC_GetSync() {

	return REG_IPC_SYNC & 0x0f;
}
