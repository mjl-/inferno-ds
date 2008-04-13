implement Touchprint;

include "sys.m";
include "draw.m";
include "devpointer.m";

sys: Sys;
ptr: Devpointer;

Touchprint: module {
	init:	fn(nil: ref Draw->Context, nil: list of string);
};

init(nil: ref Draw->Context, nil: list of string)
{
	sys = load Sys Sys->PATH;
	ptr = load Devpointer Devpointer->PATH;
	ptr->init();

	spawn reader(nil, posn := chan of ref Draw->Pointer, pidc := chan of (int, string));
	(pid, err) := <-pidc;
	sys->print("pid %d err %q\n", pid, err);
	if(err != nil)
		fail(err);
	for(;;) {
		p := <-posn;
		sys->print("x %d y %d buttons %d\n", p.xy.x, p.xy.y, p.buttons);
	}
}

reader(file: string, posn: chan of ref Draw->Pointer, pidc: chan of (int, string))
{
	ptr->reader(file, posn, pidc);
}

fail(s: string)
{
	sys->fprint(sys->fildes(2), "%s\n", s);
	raise "fail:"+s;
}
