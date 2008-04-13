#include <u.h>

uchar
inb(int reg)
{
	return *(uchar*)reg;
}

ushort
insh(int reg)
{
	return *(ushort*)reg;
}

ulong
inl(int reg)
{
	return *(ulong*)reg;
}

void
outb(int reg, uchar b)
{
	*(uchar*)reg = b;
}

void
outsh(int reg, ushort sh)
{
	*(ushort*)reg = sh;
}

void
outl(int reg, ulong l)
{
	*(ulong*)reg = l;
}

