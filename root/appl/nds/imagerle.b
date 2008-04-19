# convert the data of an nds 16-bit image to something smaller (hopefully):  a run-length encoded image.
# the format is:
# 	count (1 byte (yes, can be zero))
# 	pixel (2 bytes)

implement Imagerle;

include "sys.m";
include "draw.m";
include "arg.m";
include "bufio.m";

sys: Sys;
bufio: Bufio;

sprint, fprint, print: import sys;
Iobuf: import bufio;

Imagerle: module {
	init:	fn(nil: ref Draw->Context, nil: list of string);
};

init(nil: ref Draw->Context, args: list of string)
{
	sys = load Sys Sys->PATH;
	arg := load Arg Arg->PATH;
	bufio = load Bufio Bufio->PATH;

	arg->init(args);
	arg->setusage(arg->progname());
	while((c := arg->opt()) != 0)
		case c {
		* =>	arg->usage();
		}

	args = arg->argv();
	if(len args != 0)
		arg->usage();


	in := bufio->fopen(sys->fildes(0), Bufio->OREAD);
	if(in == nil)
		fail(sprint("bufio open: %r"));

	out := bufio->fopen(sys->fildes(1), Bufio->OWRITE);
	if(out == nil)
		fail(sprint("bufio open: %r"));

	run := 0;
	runmax: con 255;
	curc0, curc1: byte;

loop:
	for(;;) {
		c0 := in.getb();
		c1 := in.getb();
		if(c0 == Bufio->ERROR || c1 == Bufio->ERROR)
			fail(sprint("reading: %r"));
		if(c0 == Bufio->EOF) {
			if(run > 0) {
				out.putb(byte run);
				out.putb(curc0);
				out.putb(curc1);
			}
			break loop;
		}
		if(c1 == Bufio->EOF)
			fail(sprint("premature eof"));

		if(run > 0 && run < runmax && byte c0 == curc0 && byte c1 == curc1) {
			run += 1;
		} else {
			if(run > 0) {
				out.putb(byte run);
				out.putb(curc0);
				out.putb(curc1);
			}
			curc0 = byte c0;
			curc1 = byte c1;
			run = 1;
		}
	}
	out.flush();
}

warn(s: string)
{
	sys->fprint(sys->fildes(2), "%s\n", s);
}

fail(s: string)
{
	warn(s);
	raise "fail:"+s;
}
