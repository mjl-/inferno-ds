# convert a 24-bit bmp to a format that can directly be written to nintendo-ds memory.

# the 54-byte header is skipped altogether, the width of a row is necessary to correctly.
# if your bmp-file has a different encoding, e.g. color depth or compression (not sure
# whether that's possible), gibberish will be generated.

implement Rawimage;

include "sys.m";
include "draw.m";
include "arg.m";

sys: Sys;

sprint, fprint, print: import sys;

Pixelsize: con 3;

Rawimage: module {
	init:	fn(nil: ref Draw->Context, nil: list of string);
};

init(nil: ref Draw->Context, args: list of string)
{
	sys = load Sys Sys->PATH;
	arg := load Arg Arg->PATH;

	arg->init(args);
	arg->setusage(arg->progname()+" width bmpfile");
	while((c := arg->opt()) != 0)
		case c {
		* =>	arg->usage();
		}

	args = arg->argv();
	if(len args != 2)
		arg->usage();
	width := int hd args;
	path := hd tl args;

	fd := sys->open(path, Sys->OREAD);
	if(fd == nil)
		fail(sprint("open %q: %r", path));

	(ok, dir) := sys->fstat(fd);
	if(ok != 0)
		fail(sprint("fstat: %r"));

	rows := int ((dir.length - big 54) / (big Pixelsize*big width));
	if((dir.length - big 54) % (big Pixelsize * big width) != big 0)
		fail(sprint("stray pixels.  length %bd, image data %bd, width %d, rows ~%d", dir.length, dir.length-big 54, width, rows));

	fprint(sys->fildes(2), "writing %d rows, image data %bd\n", rows, dir.length-big 54);

	buf := array[Pixelsize*width] of byte;
	dst := array[2*width] of byte;
	for(i := rows-1; i >= 0; i -= 1) {
		rowoff := big (54 + Pixelsize*width*i);
		#warn(sprint("reading row at offset %bd", rowoff));
		n := sys->pread(fd, buf, len buf, rowoff);
		if(n != len buf)
			fail(sprint("wanted %d, have %d", len buf, n));
		for(j := 0; j < len buf/Pixelsize; j++) {
			# ff 00 00      blue
			# 00 ff 00      green
			# 00 00 ff      red

			off := j*Pixelsize;
			#warn(sprint("pixel rgb %d %d %d", int buf[off+2], int buf[off+1], int buf[off+0]));

			r := int buf[off+2]>>3;
			g := int buf[off+1]>>3;
			b := int buf[off+0]>>3;
			v := r|(g<<5)|(b<<10)|(1<<15);
			dstoff := j*2;
			dst[dstoff+0] = byte v;
			dst[dstoff+1] = byte (v>>8);
		}

		n = sys->write(sys->fildes(1), dst, len dst);
		if(n != len dst)
			fail(sprint("writing: %r"));
	}
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
