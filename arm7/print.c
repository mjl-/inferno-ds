#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "fns.h"
#include "nds.h"

/* 
 * simple [s]print routines for debugging,
 * at some point they could be used to bootstrap the print(10) lib.
 */

static int
itoa(ulong ival, char *a, int base)
{

	char *conv;
	int i, n;
	char ar[32]; /* a reversed */

	n = 0;
	conv = "0123456789abcdef";
	while(ival){
		i = ival % base;
		ival /= base;
		ar[n++] = conv[i];
	}

	for (i=0; i<n; i++)
		a[i] = ar[n-i-1];
	a[n] = '\0';
	return n;
}

static int
vsprint(char *s, char *fmt, va_list ap)
{
	char *p, *bs, *sval;
	int ival, n;
	ulong uval;
	char buf[32];

	bs = s;
	for (p = fmt; *p; p++) {
		if (*p != '%') {
			*s++ = *p;
			continue;
		}
		
		switch (*++p) {
		case 'd':
			ival = va_arg(ap, int);
			n = itoa(ival, buf, 10);
			s = strcpy(s, buf) + n;
			break;

		case 'x':
			uval = va_arg(ap, ulong);
			n = itoa(uval, buf, 16);
			s = strcpy(s, buf) + n;
			break;

		case 's':
			sval = va_arg(ap, char *);
			s = strcpy(s, sval) + strlen(sval);
			break;

		default:
			*s++ = *p;
			break;
		}
	}
	
	return (s - bs);
}

int
sprint(char *s, char *fmt, ...)
{
	int n;
	va_list ap;

	va_start(ap, fmt);
	n = vsprint(s, fmt, ap);
	va_end(ap);

	return n;
}

enum { PRINTSIZE = 256 };
#define SData ((Sdata*)((char*)IPC + sizeof(IPC)))
typedef struct Sdata Sdata;
struct Sdata{
	ulong n;
	char  s[PRINTSIZE];
};

int
print(char *fmt, ...)
{
	va_list ap;
	Sdata *sd  = SData;

	sd->n = 0;
	memset((void*)sd->s, '\0', PRINTSIZE);

	va_start(ap, fmt);
	sd->n = vsprint(sd->s, fmt, ap);
	va_end(ap);

	nbfifoput(F7print, (ulong)sd->s);
	return sd->n;
}
