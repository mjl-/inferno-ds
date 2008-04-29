enum {
	FWconsoletype =	0x1d,	/* 1 byte */
	  FWds =	0xff,
	  FWdslite =	0x20,
	  FWique =	0x43,
};

enum {
	Ds =		FWds,
	Dslite =	FWdslite,
	Dsique =	FWique,
};

extern int ndstype;


/* keys */
enum {
	Xbtn =		10,
	Ybtn =		11,
	Dbgbtn =	13,
	Pdown =		16,
	Lclose =	17,
	Lmax,

	Xbtn7 =		0,
	Ybtn7 =		1,
	Dbgbtn7 =	3,
	Pdown7 =	6,
	Lclose7 =	7,
};
