enum {
	Fcmdwidth =	4,
	Fcmdmask =	(1<<Fcmdwidth)-1,

	/* from arm9 to arm7 */
	F9brightness =	0,

	/* from arm7 to arm9 */
	F7keyup,
	F7keydown,
	F7mousedown,
	F7mouseup,
};


/* todo: this should move to fifo.c, once arm7 can link against fifo.c */
enum {
	Fifoctl =	0x4000184,
	Fifosend =	0x4000188,
	Fiforecv =	0x4100000,

	FifoTempty =	1<<0,
	FifoTfull =	1<<1,
	FifoTirq =	1<<2,
	FifoTflush =	1<<3,
	FifoRempty =	1<<8,
	FifoRfull =	1<<9,
	FifoRirq =	1<<10,
	Fifoerror =	1<<14,
	Fifoenable =	1<<15,
};
