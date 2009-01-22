#include <u.h>
#include "../mem.h"
#include "../io.h"
#include <kern.h>
#include "fns.h"

/* 
 * simple [s]print routines for debugging.
 * grouped in this file to ease compilation inclusion/exclusion.
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

#define SData ((char*)IPC)

int
print(char *fmt, ...)
{
	static int n = 0;
	va_list ap;
	char *s = SData;

	if(n)
		memset((void*)s, '\0', n);
	va_start(ap, fmt);
	n = vsprint(s, fmt, ap);
	va_end(ap);

	while(!nbfifoput(F7print, (ulong)s));
	return n;
}
