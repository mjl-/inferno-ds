#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "dat.h"
#include "fns.h"

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
	do{
		i = ival % base;
		ival /= base;
		ar[n++] = conv[i];
	}while(ival);

	for (i=0; i<n; i++)
		a[i] = ar[n-i-1];
	a[n] = '\0';
	return n;
}

static int
vsprint(char *s, char *fmt, va_list ap)
{
	char *p, *bs, *sval;
	int ival, n, dofmt;
	ulong uval;
	char buf[32];

	bs = s;
	for (p = fmt; *p; p++) {
		dofmt = 0;
		if (*p == '%')
			dofmt = 1;
		else
			*s++ = *p;

		while(dofmt)
		switch (*++p) {
		case 'd':
			ival = va_arg(ap, int);
			n = itoa(ival, buf, 10);
			s = strcpy(s, buf) + n;
			dofmt = 0;
			break;

		case 'x':
			uval = va_arg(ap, ulong);
			n = itoa(uval, buf, 16);
			s = strcpy(s, buf) + n;
			dofmt = 0;
			break;

		case 's':
			sval = va_arg(ap, char *);
			s = strcpy(s, sval) + strlen(sval);
			dofmt = 0;
			break;

		case 'l':
		case 'u':
			break; /* ignored */

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

/* TODO: SData seems overwritten */
enum { PRINTSIZE = 256 };
#define SData ((volatile Sdata*)((char*)IPC + sizeof(IPC)))
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

	while(!nbfifoput(F7print, (ulong)sd->s));
	return sd->n;
}
