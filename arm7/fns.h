void	intrmask(int v, int tbdf);
void	intrunmask(int v, int tbdf);
void	intrclear(int v, int tbdf);
void	intrenable(int v, void (*r)(void*), void *a, int tbdf);
void 	trapinit(void);

int	nbfifoput(ulong cmd, ulong v);
void	fifoput(ulong cmd, ulong v);

void	swiSoftReset(void);
void	swiWaitForVBlank(void);
void	swiDelay(ulong);
