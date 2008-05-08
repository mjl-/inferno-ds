void	brightset(int);

void	intrmask(int v, int tbdf);
void	intrunmask(int v, int tbdf);
void	intrclear(int v, int tbdf);
void	intrenable(int v, void (*r)(void), int tbdf);
void 	trapinit(void);

void nds_fifo_send(ulong v);
void nbfifoput(ulong cmd, ulong v);
void fifoput(ulong cmd, ulong v);

int	print(char *fmt, ...);
