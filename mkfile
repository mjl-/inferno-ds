<../../mkconfig

#Configurable parameters

CONF=ds				#default configuration
CONFLIST=ds

SYSTARG=$OSTARG
#SYSTARG=Inferno
OBJTYPE=arm
#OBJTYPE=thumb
INSTALLDIR=$ROOT/Inferno/$OBJTYPE/bin	#path of directory where kernel is installed
#end configurable parameters

<$ROOT/mkfiles/mkfile-$SYSTARG-$OBJTYPE	#set vars based on target system

BIN=$ROOT/MacOSX/386/bin

CC=$BIN/5c
AS=$BIN/5a
LD=$BIN/5l

#CC=$BIN/tc
#AS=$BIN/5a -t
#LD=$BIN/5l -t

<| $SHELLNAME ../port/mkdevlist $CONF	#sets $IP, $DEVS, $ETHERS, $VGAS, $PORT, $MISC, $LIBS, $OTHERS

KTZERO=0x02008010
ARM7ZERO=0x03800000
#KDZERO=0x02008010 #8010

OBJ=\
#	rom.$O\
	l.$O\
	clock.$O\
	div.$O\
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

LIBNAMES=${LIBS:%=lib%.a}
LIBDIRS=$LIBS

HFILES=\
	mem.h\
	dat.h\
	fns.h\
	io.h\
	fpi.h\

CFLAGS=-wFV -I$ROOT/Inferno/$OBJTYPE/include -I$ROOT/include -I$ROOT/libinterp -r
KERNDATE=`{ndate}

# default:V: i$CONF.gz i$CONF.p9.gz
default:V: i$CONF.rom

install:V: $INSTALLDIR/i$CONF $INSTALLDIR/i$CONF.gz $INSTALLDIR/i$CONF.p9.gz $INSTALLDIR/i$CONF.raw


i$CONF: $OBJ $CONF.c $CONF.root.h $LIBNAMES
	$CC $CFLAGS '-DKERNDATE='$KERNDATE $CONF.c
	$LD -o $target  -H4  -T$KTZERO    -l $OBJ $CONF.$O $LIBFILES # -t 
#	$LD -o o.out    -T$KTZERO    -l $OBJ $CONF.$O $LIBFILES # -t 
#	./mksymtab o.out && rm o.out
	cd arm7; mk
#	$CC $CFLAGS arm7/arm7.c
#	$LD -o ids7  -H4 -R0 -T$ARM7ZERO   7.c  arm7.$O # -t 
			
# -D$KDZERO	

trap.t: trap.5
	cp trap.5 trap.t
trap.5: trap.c
	5c $CFLAGS trap.c



i$CONF.rom: i$CONF
	ndstool -g INFR NE  INFERNODS 0.1 -9 ids -7 arm7/o.out -e9 $KTZERO -e7  0x03800000 -c ids.nds
#	dsbuild ids.nds -o ids.nds.gba
#	open ids.nds
#	scp ids.nds noah-e@rayserv44:~/
	
size: $OBJ $CONF.c $CONF.root.h $LIBNAMES
	$CC $CFLAGS '-DKERNDATE='$KERNDATE $CONF.c
	$LD -o $target -R0 -T$KTZERO -D$KDZERO   -l $OBJ $CONF.$O $LIBFILES >/dev/null # -t 
	ksize size
	rm size


<../port/portmkfile

../init/$INIT.dis:	../init/$INIT.b
		cd ../init; mk $INIT.dis

clock.$O:	$ROOT/Inferno/$OBJTYPE/include/ureg.h
devether.$O:	$ROOT/Inferno/$OBJTYPE/include/ureg.h
devsapcm.$O:	$ROOT/Inferno/$OBJTYPE/include/ureg.h
fault386.$O:	$ROOT/Inferno/$OBJTYPE/include/ureg.h
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
	
syms:   $OBJ $CONF.c $CONF.root.h $LIBNAMES
	$CC $CFLAGS -a '-DKERNDATE='$KERNDATE $CONF.c >syms
	cd arm7; mk syms
