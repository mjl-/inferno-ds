void	intrmask(int v, int tbdf);
void	intrunmask(int v, int tbdf);
void	intrclear(int v, int tbdf);
void	intrenable(int v, void (*r)(void*), void *a, int tbdf);
void 	trapinit(void);

ulong	nds_get_time7(void);
void	nds_set_time7(ulong);

int	nbfifoput(ulong cmd, ulong v);
void	fifoput(ulong cmd, ulong v);

void	swiSoftReset(void);
void	swiWaitForVBlank(void);
void	swiDelay(ulong);
void	swiSleep(void);
void	swidebug(char *);
