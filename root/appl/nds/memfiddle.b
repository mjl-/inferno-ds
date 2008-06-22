implement NdsMemfiddle;

# xxx
# - allow us to get/put arm7 registers.  needs kernel support, probably send/receive the values over the fifo.

# btn  |description
# -----+----------
# +/- (toggle sign)
#	to be added to address, for the next few buttons
# 1	add sign*1
# 4	add sign*4
# 20	add sign*20 (hex)
# K	cycle through known paths with registers, e.g. /lib/registers/display
# 1e2/1e3/1e4/1e5/1e6 (cycle)
#	address (in hex).  added at click on next button
# e	add address of previous button
# l/b/s (cycle)
#	size of address (long, byte, short)
# k	cycle through known addresses
# a	add current address to list
# d	remove current address from list
# ns	toggle displaying names in list
# g	get value for current address
# ga	get values for all addresses in list
# p	write address
# pg	write and read address
# ~	invert all bits
# 1	set all bits to 1
# 0	set all bits to 0


include "sys.m";
include "draw.m";
include "string.m";
include "bufio.m";
include "tk.m";
include "tkclient.m";

sys: Sys;
draw: Draw;
str: String;
bufio: Bufio;
tk: Tk;
tkclient: Tkclient;

Iobuf: import bufio;
Display, Image: import draw;
print, fprint, fildes, sprint: import sys;

NdsMemfiddle: module
{
	init:	fn(ctxt: ref Draw->Context, argv: list of string);
};

Reg: adt {
	addr, size:	int;
	name:		string;
	value, modmask:	big;
};

r := Reg(0, 4, "", big 0, big 0);
mempath: con "/dev/ndsmem";
regpath: con "/lib/registers";
memfd: ref Sys->FD;
sign := 1;
shownames := 1;
eaddr := 0;
eaddrs := array[] of {("1e2", 16r100), ("1e3", 16r1000), ("1e4", 16r10000), ("1e5", 16r100000), ("1e6", 16r1000000)};
sizechars := array[] of {1 => "b", 2 => "s", 4 => "l", * => "?"};
have: list of ref Reg;

known := array[0] of (int, int, string);
knownpos := 0;
knownpathpos := 0;
knownpaths := array[] of {
	"main",
	"div",
	"disp",
	"subdisp",
	"video",
	"wram",
};

tkcmds := array[] of {
"frame .add",
"frame .fiddle",
"frame .listhold",

"label .add.addr -text 0x0000000",
"label .add.name -text <name>",
"pack .add.addr .add.name -side left -in .add",

"frame .fiddle.bytes",
"frame .fiddle.ctl",
"frame .b3",
"frame .b2",
"frame .b1",
"frame .b0",

"frame .g0",
"frame .g1",
"frame .g2",
"frame .g3",

"button .sign -text + -command {send cmd togglesign}",
"button .a1 -text 1 -command {send cmd addaddr 1}",
"button .a2 -text 4 -command {send cmd addaddr 4}",
"button .a3 -text 20 -command {send cmd addaddr 20}",
"button .knownpathcycle -text K -command {send cmd cycleknownpath}",
"button .ecycle -text 1e2 -command {send cmd ecycle}",
"button .eadd -text e -command {send cmd eadd}",
"button .size -text l -command {send cmd cyclesize}",
"button .known -text k -command {send cmd cycleknown}",
"pack .sign .a1 .a2 .a3 .knownpathcycle -in .g0 -side left",
"pack .ecycle .eadd .size .known -in .g1 -side left",

"button .a -text a -command {send cmd add}",
"button .del -text d -command {send cmd del}",
"button .names -text ns -command {send cmd togglenames}",
"button .get -text g -command {send cmd get}",
"button .getall -text ga -command {send cmd getall}",
"pack .a .del .names .get .getall -side left -in .g2",

"button .put -text p -command {send cmd put}",
"button .putget -text pg -command {send cmd put; send cmd get}",
"button .invert -text ~ -command {send cmd invert}",
"button .ones -text 1 -command {send cmd ones}",
"button .zeroes -text 0 -command {send cmd zeroes}",
"pack .put .putget .invert .ones .zeroes -side left -in .g3",

"pack .g0 .g1 .g2 .g3 -in .fiddle.ctl",

"pack .fiddle.bytes -side left -in .fiddle -fill both -expand 1",
"pack .fiddle.ctl -side left -in .fiddle",

"scrollbar .listscroll -command {.list yview}",
"listbox .list -yscrollcommand {.listscroll set}",
"bind .list <ButtonRelease-1> {send cmd memsel}",

"pack .listscroll -side left -in .listhold -fill y",
"pack .list -in .listhold -fill both -expand 1",

"pack .add .fiddle -fill x -expand 1",
"pack .listhold -fill both -expand 1",
};

t: ref Tk->Toplevel;
wmctl: chan of string;


init(ctxt: ref Draw->Context, nil: list of string)
{
	sys  = load Sys  Sys->PATH;
	if(ctxt == nil)
		fail("bad context");

	draw = load Draw Draw->PATH;
	str = load String String->PATH;
	bufio = load Bufio Bufio->PATH;
	tk = load Tk Tk->PATH;
	tkclient = load Tkclient Tkclient->PATH;

	memfd = sys->open(mempath, Sys->ORDWR);
	if(memfd == nil)
		fail(sprint("open %q: %r", mempath));

	tkclient->init();
	(t, wmctl) = tkclient->toplevel(ctxt, "", "nds/memfiddle", Tkclient->Appl);

	readknown();

	tkcmdchan := chan of string;
	tk->namechan(t, tkcmdchan, "cmd");
	for (i := 0; i < len tkcmds; i++)
		tk->cmd(t, tkcmds[i]);

	for(by := 3; by >= 0; by--) {
		cmd := "pack ";
		for(bi := 7; bi >= 0; bi--) {
			tk->cmd(t, sprint("button .b%d.bit%d -text ? -command {send cmd bit %d}", by, bi, by*8+bi));
			cmd += sprint(".b%d.bit%d ", by, bi);
		}
		cmd += sprint("-in .b%d -side left -fill x -expand 1", by);
		tk->cmd(t, cmd);
	}
	tk->cmd(t, "pack .b3 .b2 .b1 .b0 -in .fiddle.bytes -fill both -expand 1");
	tk->cmd(t, "pack propagate . 0");
	tkmem();

	tkclient->onscreen(t, nil);
	tkclient->startinput(t, "ptr"::nil);
	for(;;) alt {
	s := <-t.ctxt.ptr =>
		tk->pointer(t, *s);
	s := <-t.ctxt.ctl or s = <-t.wreq =>
		tkclient->wmctl(t, s);
	menu := <-wmctl =>
		if(menu == "exit")
			exit;
		tkclient->wmctl(t, menu);
	cmd := <-tkcmdchan =>
		(nil, tokens) := sys->tokenize(cmd, " ");
		case hd tokens {
		"bit" =>
			bit := int hd tl tokens;
			print("bit %d, value %bux\n", bit, r.value);
			if(bit < 8*r.size) {
				r.value ^= big 1<<bit;
				r.modmask |= big 1<<bit;
			}
			print("new value %08bux\n", r.value);
			tkmem();
		"togglesign" =>
			sign *= -1;
			signchar := "+";
			if(sign < 0)
				signchar = "-";
			tk->cmd(t, ".sign configure -text '"+signchar);
		"addaddr" =>
			r.addr += sign*str->toint(hd tl tokens, 16).t0;
			r.name = nil;
			r.value = r.modmask = big 0;
			tkaddr();
		"ecycle" =>
			eaddr = (eaddr+1) % len eaddrs;
			tk->cmd(t, ".ecycle configure -text '"+eaddrs[eaddr].t0);
		"eadd" =>
			r.addr += sign*eaddrs[eaddr].t1;
			r.name = nil;
			r.value = r.modmask = big 0;
			tkaddr();
		"cyclesize" =>
			if(r.size == 4)
				r.size = 1;
			else if(r.size == 1)
				r.size = 2;
			else
				r.size = 4;
			r.value = r.modmask = big 0;
			tkaddr();
		"cycleknownpath" =>
			knownpathpos = (knownpathpos+1)%len knownpaths;
			readknown();
			if(len known > 0) {
				knownpos = 0;
				r.addr = known[knownpos].t0;
				r.size = known[knownpos].t1;
				r.name = known[knownpos].t2;
				r.value = r.modmask = big 0;
			}
			tkaddr();
		"cycleknown" =>
			knownpos = (knownpos+1) % len known;
			r.addr = known[knownpos].t0;
			r.size = known[knownpos].t1;
			r.name = known[knownpos].t2;
			r.value = r.modmask = big 0;
			tkaddr();
		"add" =>
			if(r.name == nil)
				r.name = sprint("%08ux", r.addr);
			have = ref (r.addr, r.size, r.name, big 0, big 0)::have;
			tklist();
		"del" =>
			new: list of ref Reg;
			for(; have != nil; have = tl have)
				if((hd have).addr != r.addr)
					new = hd have::new;
			for(; new != nil; new = tl new)
				have = hd new::have;
			if(have != nil)
				r = *hd have;
			tklist();
		"togglenames" =>
			shownames ^= 1;
			tklist();
		"get" =>
			get();
			tkmem();
			tklist();
		"getall" =>
			getall();
			tkmem();
			tklist();
		"put" =>
			put();
			tkmem();
			tklist();
		"invert" =>
			r.value = ~r.value;
			r.modmask = ~big 0;
			tkmem();
		"ones" =>
			r.value = ~big 0;
			r.modmask = ~big 0;
			tkmem();
		"zeroes" =>
			r.value = big 0;
			r.modmask = ~big 0;
			tkmem();
		"memsel" =>
			index := int tk->cmd(t, ".list curselection");
			for(l := have; l != nil && index > 0; l = tl l)
				index--;
			if(l != nil) {
				r = *hd l;
				tkaddr();
				tkmem();
			}
		* =>
			print("bad cmd: %q\n", cmd);
		}
		tk->cmd(t, "update");
	}
}

# update the current reg config
tkaddr()
{
	tk->cmd(t, ".add.addr configure -text '"+sprint("0x%08ux", r.addr));
	tk->cmd(t, ".add.name configure -text '"+r.name);
	tk->cmd(t, ".size configure -text '"+sizechars[r.size]);
}

# draw the bits
tkmem()
{
	values := array[] of {"0", "1"};
	colors := array[] of {"gray", "yellow"};
	color, bit: string;
	for(by := 0; by < 4; by++)
		for(bi := 0; bi < 8; bi++) {
			color = colors[int (r.modmask>>(by*8+bi)) & 1];
			bit = "-";
			if(by*8+bi < r.size*8)
				bit = values[int (r.value>>(by*8+bi)) & 1];
			tk->cmd(t, sprint(".b%d.bit%d configure -background %s -text '%s", by, bi, color, bit));
		}
}

# clear the address list and re-add elements
tklist()
{
	tk->cmd(t, ".list delete 0 end");
	index := 0;
	see := 0;
	for(l := have; l != nil; l = tl l) {
		reg := *hd l;
		name := reg.name;
		if(!shownames)
			name = sprint("%08ux", reg.addr);
		s := sprint("%-10s ", name);
		if(reg.size == 4)
			s += sprint("%02ux %02ux %02ux %02ux", 16rff&int reg.value>>24, 16rff&int reg.value>>16, 16rff&int reg.value>>8, 16rff&int reg.value);
		else if(reg.size == 2)
			s += bits(16rff&int reg.value>>8) + " " + bits(16rff&int reg.value);
		else
			s += bits(16rff&int reg.value);
		tk->cmd(t, ".list insert end '"+s);
		if(reg.addr == r.addr)
			see = index;
		index++;
	}
	tk->cmd(t, ".list selection set "+string see);
	tk->cmd(t, ".list see "+string see);
}

# read the value
read(addr, size: int): big
{
	buf := array[4] of {* => byte 0};
	n := sys->pread(memfd, buf[len buf-size:], size, big addr);
	if(n != size)
		warn(sprint("reading %d bytes from 0x%ux: %r", size, addr));
	v := big 0;
	v |= big buf[0]<<24;
	v |= big buf[1]<<16;
	v |= big buf[2]<<8;
	v |= big buf[3]<<0;
	return v;
}

# get a fresh value for r
get()
{
	if(r.addr == 0)
		return;

	print("reading size %d from addr 0x%08ux\n", r.size, r.addr);

	r.value = read(r.addr, r.size);
	r.modmask = big 0;

	for(l := have; l != nil; l = tl l) {
		reg := hd l;
		if(reg.addr == r.addr) {
			reg.value = r.value;
			reg.modmask = r.modmask;
		}
	}
}

# get fresh values for all
getall()
{
	for(l := have; l != nil; l = tl l) {
		reg := hd l;
		reg.value = read(reg.addr, reg.size);
		reg.modmask = big 0;
	}
}

# put/write current value
put()
{
	if(r.addr == 0)
		return;

	print("writing 0x%08bux size %d to addr 0x%08ux\n", r.value, r.size, r.addr);
	
	buf := array[4] of byte;
	buf[0] = byte (r.value>>24);
	buf[1] = byte (r.value>>16);
	buf[2] = byte (r.value>>8);
	buf[3] = byte (r.value>>0);
	n := sys->pwrite(memfd, buf[len buf-r.size:], r.size, big r.addr);
	if(n != r.size)
		warn(sprint("writing %d bytes to 0x%ux: %r", r.size, r.addr));
	r.modmask = big 0;
}

# format a byte as bit string
bits(b: int): string
{
	s: string;
	bits := array[] of {'0', '1'};
	for(i := 7; i >= 0; i--)
		s[len s] = bits[(b>>i) & 1];
	return s;
}

# read known register from file: addr, size, name
readknown()
{
	path := regpath+"/"+knownpaths[knownpathpos];
	b := bufio->open(path, Bufio->OREAD);
	if(b == nil)
		fail(sprint("open %q: %r", path));
	newknown: list of (int, int, string);
	while((s := b.gets('\n')) != nil) {
		s = str->drop(s, " \t");
		if(s != nil && s[0] == '#')
			continue;
		arg0, arg1, arg2: string;
		(arg0, s) = str->splitl(s, " \t");
		(arg1, s) = str->splitl(str->drop(s, " \t"), " \t");
		arg2 = str->drop(s, " \t");
		if(arg2 != nil && arg2[len arg2-1] == '\n')
			arg2 = arg2[:len arg2-1];
		#print("args: %q %q %q\n", arg0, arg1, arg2);
		if(arg0 == nil || arg1 == nil)
			continue;
		newknown = (str->toint(arg0, 16).t0, int arg1, arg2)::newknown;
	}

	known = array[len newknown] of (int, int, string);
	i := len newknown-1;
	for(; newknown != nil; newknown = tl newknown)
		known[i--] = hd newknown;
	#print(sprint("new known read from %q, %d", path, len known));
}

warn(s: string)
{
	fprint(fildes(2), "%s\n", s);
}

fail(s: string)
{
	warn(s);
	raise "fail:"+s;
}
