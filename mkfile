<../../mkconfig

#Configurable parameters

CONF=sds				#default configuration
CONFLIST=ds sds dds
CLEANCONFLIST=ds sds dds

SYSTARG=$OSTARG
OBJTYPE=arm #thumb
INSTALLDIR=$ROOT/Inferno/$OBJTYPE/bin	#path of directory where kernel is installed
#end configurable parameters

<$ROOT/mkfiles/mkfile-$SYSTARG-$OBJTYPE	#set vars based on target system

<| $SHELLNAME ../port/mkdevlist $CONF	#sets $IP, $DEVS, $ETHERS, $VGAS, $PORT, $MISC, $LIBS, $OTHERS

KTZERO=0x02000130
KTZERO7=0x03800000

OBJ=\
	l.$O\
	clock.$O\
	fpi.$O\
	fpiarm.$O\
	fpimem.$O\
	defont.$O\
	main.$O\
	trap.$O\
	$CONF.root.$O\
	$IP\
	$DEVS\
	$ETHERS\
	$LINKS\
	$PORT\
	$MISC\
	$OTHERS\

<| $SHELLNAME arm7/mkdeps arm7/mkfile # sets $ARM7SRC

LIBNAMES=${LIBS:%=lib%.a}
LIBDIRS=$LIBS

HFILES=\
	mem.h\
	dat.h\
	fns.h\
	io.h\
	fpi.h\
	screen.h\

CFLAGS=-wFV -I$ROOT/Inferno/$OBJTYPE/include -I$ROOT/include -I$ROOT/libinterp -r
KERNDATE=`{ndate}

default:V: i$CONF.nds i$CONF.p9 i$CONF.SYM

install:V: $INSTALLDIR/i$CONF $INSTALLDIR/i$CONF.gz $INSTALLDIR/i$CONF.p9.gz $INSTALLDIR/i$CONF.raw

i$CONF: $OBJ $CONF.c $CONF.root.h $LIBNAMES
	$CC $CFLAGS '-DKERNDATE='$KERNDATE $CONF.c
	$LD -o $target -H0 -T$KTZERO -l $OBJ $CONF.$O $LIBFILES

arm7/i$CONF arm7/i$CONF.p9: $ARM7SRC
	cd arm7; mk CONF'='$CONF

i$CONF.kfs: root/lib/proto/$CONF'proto'
	emu -c1 /os/ds/root/dis/mkkfs /os/ds/$prereq /os/ds/$target || true

REV=`{svn info | sed -n 's/^Revisi.n: /rev./p'}
i$CONF.nds: i$CONF arm7/i$CONF
	ndstool -g INFR -m ME -c i$CONF.nds -b ds.bmp \
		'Native Inferno Kernel NDS port;inferno-ds '$REV';code.google.com/p/inferno-ds' \
		-7 arm7/i$CONF -r7 $KTZERO7 -e7 $KTZERO7 \
		-9 i$CONF -r9 $KTZERO -e9 $KTZERO
	# append rom data at end of .nds (see root/dis/mkkfs)
	echo -n ROMZERO9 >> i$CONF.nds
#	dlditool misc/R4tf.dldi i$CONF.nds

i$CONF.ds.gba: i$CONF.nds
	dsbuild $prereq -o $target

i$CONF.p9: $OBJ $CONF.c $CONF.root.h $LIBNAMES
	$CC $CFLAGS '-DKERNDATE='$KERNDATE $CONF.c
	$LD -o $target -R4 -T$KTZERO -l $OBJ $CONF.$O $LIBFILES

i$CONF.SYM: i$CONF.p9 arm7/i$CONF.p9
	$SHELLNAME mksymtab $prereq > $target

<../port/portmkfile

../init/$INIT.dis:	../init/$INIT.b
		cd ../init; mk $INIT.dis

clock.$O:	$ROOT/Inferno/$OBJTYPE/include/ureg.h
devether.$O:	$ROOT/Inferno/$OBJTYPE/include/ureg.h
main.$O:	$ROOT/Inferno/$OBJTYPE/include/ureg.h
trap.$O:	$ROOT/Inferno/$OBJTYPE/include/ureg.h

devether.$O $ETHERS:	etherif.h ../port/netif.h
$IP devip.$O:		../ip/ip.h

dummy:V:

# to be moved to port/interp 
bench.h:D: ../../module/bench.m
	rm -f $target && limbo -a -I../../module ../../module/bench.m > $target
benchmod.h:D:  ../../module/bench.m
	rm -f $target && limbo -t Bench -I../../module ../../module/bench.m > $target
devbench.$O: bench.h benchmod.h

devuart.$O:	devuart.c
	$CC $CFLAGS devuart.c
devaudio.$O:	devaudio.c
	$CC $CFLAGS devaudio.c

syms:   $OBJ $CONF.c $CONF.root.h $LIBNAMES
	$CC $CFLAGS -a '-DKERNDATE='$KERNDATE $CONF.c >syms
	cd arm7; mk syms

vclean:V: clean
	rm -f syms
	cd arm7; mk CONF'='$CONF vclean
