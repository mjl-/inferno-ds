#
# based on contrib/zswanch/morse.c, by 20h
# modified limbo version, by saoret.one

implement Morse;

include "sys.m";
	sys: Sys;
include "draw.m";
include "arg.m";
	arg: Arg;
include "string.m";
	str: String;
include "sh.m";

Morse: module
{
	DITLEN:	con 1;
	DAHLEN:	con 3;
	WRDLEN:	con 50;
	CSILLEN:	con 3;
	WSILLEN:	con 7;
	
	DEC, AUD, INP: con 1<<iota;

	Sample: adt
	{
		d:	array of byte;
		n: 	int;

		mksample:	fn(s: self ref Sample, morse: string);
		addsound:	fn(s: self ref Sample, units,timeunit: int);
		addsilence:	fn(s: self ref Sample, units,timeunit: int);
	};

	init:	fn(ctxt: ref Draw->Context, argv: list of string);
	morenc:	fn(in, out: ref Sys->FD);
	mordec:	fn(in, out: ref Sys->FD);
	morplay:fn(in: ref Sys->FD);
};

# TODO
# while (not bored){
#	revise it;
# 	test it;
# 	use it;
# 	document it;
# }

all := array[] of {
				 "A", ".-",		"B", "-...",	"C", "-.-.",	"D", "-..",
				 "E", ".",		"F", "..-.",	"G", "--.",		"H", "....",
				 "I", "..",		"J", ".---",	"K", "-.-",		"L", ".-..",
				 "M", "--",		"N", "-.",		"O", "---",		"P", ".--.",
				 "Q", "--.-",	"R", ".-.",		"S", "...",		"T", "-",
				 "U", "..-",	"V", "...-",	"W", ".--",		"X", "-..-",
				 "Y", "-.--",	"Z", "--..",
				 "1", ".----",	"2", "..---",	"3", "...--",	"4", "....-",
				 "5", ".....",	"6", "-....",	"7", "--...",	"8", "---..",
				 "9", "----.",	"0", "-----",
				 "Á", ".--.-",	"Ä", ".-.-",	"É", "..-..",	"Ñ", "--.--",
				 "Ö", "---.",	"Ü", "..--",
				 ".", ".-.-.-",	",", "--..--",	"?", "..--..",	"'", ".----.",
				 "!", "-.-.--",	"/", "-..-.",	"-", "-....-",	"(", "-.--.",
				 ")", "-.--.-",	"&", "-.-.-",	":", "---...",	";", "-.-.-.",
				 "=", "-...-",	"/", "-..-.",	"-", "-....-",	"_", "..--.-",
				 "\"", ".-..-.","@", ".--.-.",	"+", ".-.-.",	"$", "...-..-",
				 "*", ".-...",	"#", "-...-.-",
				 "SOS", "...---...",
				 "\n", "...-.-"," ", " ",
				 nil, nil };

Sample.mksample(s: self ref Sample, morse: string)
{
	wpm: int = 30;
	c: int;
	comp: real;
	extra: int;
	slen: int;
	i : int;
	timeunit: int = 1000*60/(wpm*WRDLEN);
	
	if(timeunit > 100){
		# if the characters are to be sent slowly (less than 12wpm)
		# compress the elements, and stretch the gap between the characters
		
		# to keep the wpm the same
		comp = (real timeunit / real 100) - 1.0;
		timeunit = 100;
	}else{
		comp = 0.0;
	}
	
	slen = 0;
	extra = 0;
	for (i = 0; i < len morse; i++){
		c = morse[i];
		if (c == '.'){
			extra += DITLEN + DITLEN;
			slen += DITLEN + DITLEN;
		}else if (c == '-'){
			extra += DAHLEN + DITLEN;
			slen += DAHLEN + DITLEN;
		}else if (c == ' '){
			extra += CSILLEN - DITLEN;
			slen += CSILLEN - DITLEN + int (real extra * comp);
		}else if (c == '/'){
			extra += WSILLEN - DITLEN;
			slen += WSILLEN - DITLEN + int (real extra * comp);
		}
	}
	
	s.n = 0;
	s.d = array[slen*8*timeunit] of { * => byte 0};
	
	extra = 0;
	for (i = 0; i < len morse; i++){
		c = morse[i];
		if (c == '.'){
			extra += DITLEN + DITLEN;
			s.addsound(DITLEN, timeunit);
			s.addsilence(DITLEN, timeunit);
		}else if (c == '-'){
			extra += DAHLEN + DITLEN;
			s.addsound(DAHLEN, timeunit);
			s.addsilence(DITLEN, timeunit);
		}else if (c == ' '){
			extra += CSILLEN - DITLEN;
			s.addsilence(CSILLEN - DITLEN + int (real extra * comp), timeunit);
			extra = 0;
		}else if (c == '/'){
			extra += WSILLEN - DITLEN;
			s.addsilence(WSILLEN - DITLEN + int (real extra * comp), timeunit);
			extra = 0;
		}
	}
}

Sample.addsound(s: self ref Sample, units,timeunit: int)
{
	for (i:=0; i<units*timeunit; i++){
		s.d[s.n++] = byte 16rA7;
		s.d[s.n++] = byte 16r81;
		s.d[s.n++] = byte 16rA7;
		s.d[s.n++] = byte 16r00;
		s.d[s.n++] = byte 16r59;
		s.d[s.n++] = byte 16r7F;
		s.d[s.n++] = byte 16r59;
		s.d[s.n++] = byte 16r00;		
	}
}

Sample.addsilence(s: self ref Sample, units,timeunit: int)
{
	for(i:=0; i< units*timeunit; i++){
		s.d[s.n++] = byte 16r00;
		s.d[s.n++] = byte 16r00;
		s.d[s.n++] = byte 16r00;
		s.d[s.n++] = byte 16r00;
		s.d[s.n++] = byte 16r00;
		s.d[s.n++] = byte 16r00;
		s.d[s.n++] = byte 16r00;
		s.d[s.n++] = byte 16r00;
	}
}

audioctl(ctl: string)
{
	cfd := sys->open("/dev/audioctl", Sys->ORDWR);
	sys->write(cfd, array of byte ctl, len ctl);
}

morenc(in, out: ref sys->FD)
{
	c := array[1] of { * => byte 0};
	i := 0;

	while(sys->readn(in, c, 1) > 0) {
		C := str->toupper(string c);
		for(i = 0; all[i] != nil; i += 2) {
			if(C == all[i]) {
				sys->fprint(out, "%s ", all[i + 1]);
				break;
			}
		}
	}
	sys->fprint(out, "\n");
}

mordec(in, out: ref sys->FD)
{
	cc := array[16] of { * => byte 0};
	c := array[1] of { * => byte 0};

	b := 0;
	i := 0;
	
	while(sys->readn(in, c, 1) > 0) {
		if((c[0] == byte '.' || c[0] == byte '-') && i < len cc)
			cc[i++] = c[0];
		if(c[0] == byte ' ') {
			if(i == 0) {
				if(b >= 1) {
					sys->fprint(out, " ");
					b = 0;
				} else
					b++;
				continue;
			}
			CC := string cc[0:i];
			for(i = 1; all[i] != nil; i += 2) {
				if(CC == all[i]) {
					sys->fprint(out, "%s", all[i - 1]);
					break;
				}
			}
			i = 0;
			cc = array[16] of { * => byte 0};
		}
	}
}

morplay(in: ref sys->FD)
{
	buf := array[128] of byte;
	sample := ref Sample;
	
	audioctl("rate 8000\n	bits 8\n	chans 1\n");

	afd := sys->open("/dev/audio", Sys->ORDWR);
	while(sys->read(in, buf, len buf) > 0){
		sys->print("buf: %s\n", string buf);
		sample.mksample(string buf);
		if (sys->write(afd, sample.d, len sample.d) < 0)
			sys->print("bad write\n");
	}
}

morinput(in: ref sys->FD)
{
	t, d: int;
	c := array[1] of { * => byte 0};

	while(sys->readn(in, c, 1) > 0) {
		d = sys->millisec() - t;
		if (d < 40)
			c[0] = byte '-';
		else if (d < 160)
			c[0] = byte '.';
		else
			c[0] = byte ' ';
		
		sys->print("%c", int c[0]);
		
		t = sys->millisec();
	}
	sys->print("\n");
}

usage()
{
	sys->print("usage: morse [-d] [-a]\n");
	exit;
}

init(nil: ref Draw->Context, args: list of string)
{
	sys = load Sys Sys->PATH;
	arg = load Arg Arg->PATH;
	str = load String String->PATH;

	mode := 0;
	arg->init(args);
	while((c := arg->opt()) != 0)
		case c {
		'a' => mode |= AUD;
		'd' => mode |= DEC;
		'i' => mode |= INP;
		* => usage();
	}
	args = arg->argv();

	stdin := sys->fildes(0);
	stdout := sys->fildes(1);

	if (mode & INP)
		morinput(stdin);
	if (mode & AUD)
		morplay(stdin);
	if (mode & DEC)
		mordec(stdin, stdout);
	else
		morenc(stdin, stdout);

	exit;
}
