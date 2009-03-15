#
# morse - morse code encoder/decoder
# see the accompanying morse(1) man page
# written by Salva Peiro <saoret.one at gmail.com>
#

implement Morse;

include "sys.m";
	sys: Sys;
include "draw.m";
include "arg.m";
	arg: Arg;
include "string.m";
	str: String;
include "math.m";
	math: Math;

Morse: module
{
	Q20: type fixed(2.0**-20);		# fixed arith, choosen to fit precision=1.0/ARATE.
	Q28: type fixed(2.0**-2, 2.0**28);	# fixed arith, for Goertzel algorithm

	# LENs provided in number of morse elements, using DIT as unit
	DITLEN:	con 1;	# dit element '.' (short pulse)
	DAHLEN:	con 3;	# dah element '-' (long  pulse)
	SPCLEN:	con 3;	# inter element space: ' '
	WSPCLEN:con 7;	# inter word space: '/'
	WORDLEN:con 50;	# length of reference word "PARIS"

	ENC, AUD: con 1<<iota;	# modes

	wpm: int;	# words per minute (words measured as WORDLEN)

	A: int;		# signal's amp
	W: int;		# signal's freq
	P: int;		# signal's phase

	N: int;		# Goertzel samples/block
	T: int;		# slice detection threshold

	DFLTWPM:	con 15;
	DFLTAMP:	con 25;
	DFLTFREQ:	con 600;
	DFLTPHASE:	con 0;
	DFLTBLK:	con 100;
	DFLTTHR:	con 1;
		
	Wave: adt
	{
		d:	array of byte;	# data samples
		n: 	int;		# num samples

		init:	fn(A, W, P: int);
		mk:	fn(s: self ref Wave, morse: string);
		tone:	fn(s: self ref Wave, n, tdit: int);
		space:	fn(s: self ref Wave, n, tdit: int);
	};

	init:		fn(ctxt: ref Draw->Context, argv: list of string);

	morenct:	fn(in,out: ref Sys->FD);	# text to morse
	mordect:	fn(in,out: ref Sys->FD);	# morse to text
	morenca:	fn(in,out: ref Sys->FD);	# morse to audio
	mordeca:	fn(in,out: ref Sys->FD);	# audio to morse
	morkbdi:	fn(in,out: ref Sys->FD);	# keyboard to morse
};

amorse := array[] of {
	("A", ".-"),		("B", "-..."),	("C", "-.-."),	("D", "-.."),
	("E", "."),		("F", "..-."),	("G", "--."),		("H", "...."),
	("I", ".."),		("J", ".---"),	("K", "-.-"),		("L", ".-.."),
	("M", "--"),		("N", "-."),		("O", "---"),		("P", ".--."),
	("Q", "--.-"),	("R", ".-."),		("S", "..."),		("T", "-"),
	("U", "..-"),		("V", "...-"),	("W", ".--"),		("X", "-..-"),
	("Y", "-.--"),	("Z", "--.."),

	("1", ".----"),	("2", "..---"),	("3", "...--"),	("4", "....-"),
	("5", "....."),	("6", "-...."),	("7", "--..."),	("8", "---.."),
	("9", "----."),	("0", "-----"),

	("Á", ".--.-"),	("Ä", ".-.-"),	("É", "..-.."),	("Ñ", "--.--"),
	("Ö", "---."),	("Ü", "..--"),
	(".", ".-.-.-"),	(",", "--..--"),	("?", "..--.."),	("'", ".----."),
	("!", "-.-.--"),	("/", "-..-."),	("-", "-....-"),	("(", "-.--."),
	(")", "-.--.-"),	("&", "-.-.-"),	(":", "---..."),	(";", "-.-.-."),
	("=", "-...-"),	("_", "..--.-"),
	("\"", ".-..-."),("@", ".--.-."),	("+", ".-.-."),	("$", "...-..-"),
	("*", ".-..."),	("#", "-...-.-"),
	("\n", "...-.-"), (" ", " "),
	};

TONELEN: con 16; # bytes
tone := array [TONELEN] of {
	byte 16rA7, byte 16r81, byte 16rA7, byte 16r00,
	byte 16r59, byte 16r7F, byte 16r59, byte 16r00,
};

space := array [TONELEN] of {
	byte 16r00, byte 16r00, byte 16r00, byte 16r00,
	byte 16r00, byte 16r00, byte 16r00, byte 16r00,
};

# http://www.comportco.com/~w5alt/cw/cwindex.php?pg=5
Wave.init(A, W, P: int)
{
	x := Q20(2.0 * math->Pi * real W * (real 1.0 / real ARATE));
	for (i := 0; i < TONELEN; i++)
	{
		tone[i] = byte (Q20(A) * Q20(math->sin(real (x*Q20(i) + Q20(P)))));
		space[i] = byte 16r00;

		# dprints the tone to fix it at compile time
		dprint(sys->sprint("byte 16r%.2X, ", int tone[i]));
		if(i % 4 == 3)
			dprint("\n");
	}
}

Wave.tone(s: self ref Wave, n,tdit: int)
{
	for (i:=0; i<n*tdit; i++){
		s.d[s.n:] = tone;
		s.n += len tone;
	}
}

Wave.space(s: self ref Wave, n,tdit: int)
{
	for(i:=0; i< n*tdit; i++){
		s.d[s.n:] = space;
		s.n += len space;
	}
}

Wave.mk(s: self ref Wave, morse: string)
{
	n, extra: int;
	tdit: int = (60*1000)/(wpm*WORDLEN); # in ms
	
	comp := Q20(0);	# compression factor
	if(wpm <= 12){
		# for low wpm compress the elements,
		# stretching the silences to maintain wpm
		comp = (Q20(tdit) / Q20(100)) - Q20(1);
		tdit = 100;
	}
	
	n = 0;
	extra = 0;
	for (i := 0; i < len morse; i++){
		c := morse[i];
		if (c == '.'){
			extra += DITLEN + DITLEN;
			n += DITLEN + DITLEN;
		}else if (c == '-'){
			extra += DAHLEN + DITLEN;
			n += DAHLEN + DITLEN;
		}else if (c == ' '){
			extra += SPCLEN - DITLEN;
			n += SPCLEN - DITLEN + int (Q20(extra) * comp);
		}else if (c == '/'){
			extra += WSPCLEN - DITLEN;
			n += WSPCLEN - DITLEN + int (Q20(extra) * comp);
		}
	}
	
	s.n = 0;
	s.d = array[n*tdit*TONELEN] of byte;
	
	extra = 0;
	for (i = 0; i < len morse; i++){
		c := morse[i];
		if (c == '.'){
			extra += DITLEN + DITLEN;
			s.tone(DITLEN, tdit);
			s.space(DITLEN, tdit);
		}else if (c == '-'){
			extra += DAHLEN + DITLEN;
			s.tone(DAHLEN, tdit);
			s.space(DITLEN, tdit);
		}else if (c == ' '){
			extra += SPCLEN - DITLEN;
			s.space(SPCLEN - DITLEN + int (Q20(extra) * comp), tdit);
			extra = 0;
		}else if (c == '/'){
			extra += WSPCLEN - DITLEN;
			s.space(WSPCLEN - DITLEN + int (Q20(extra) * comp), tdit);
			extra = 0;
		}
	}
}

# encode text to morse
morenct(in,out: ref sys->FD)
{
	c := array[1] of byte;
	i := 0;

	while(sys->readn(in, c, 1) > 0){
		C := str->toupper(string c);
		for(i = 0; i < len amorse; i++){
			(t, m) := amorse[i];
			if(C == t){
				sys->fprint(out, "%s ", m);
				break;
			}
		}
	}
	sys->fprint(out, "\n");
}

# decode morse to text
mordect(in,out: ref sys->FD)
{
	cc := array[16] of byte;
	c := array[1] of byte;

	b := 0;
	i := 0;
	
	while(sys->readn(in, c, 1) > 0){
		if((c[0] == byte '.' || c[0] == byte '-') && i < len cc)
			cc[i++] = c[0];
		if(c[0] == byte ' '){
			if(i == 0){
				if(b >= 1) {
					sys->fprint(out, " ");
					b = 0;
				} else
					b++;
				continue;
			}
			CC := string cc[0:i];
			for(i = 0; i < len amorse; i++){
				(t, m) := amorse[i];
				if(CC == m){
					sys->fprint(out, "%s", t);
					break;
				}
			}
			i = 0;
			cc = array[16] of { * => byte 0};
		}
	}
}

rf(file: string): string
{
	fd := sys->open(file, Sys->OREAD);
	buf := array[128] of byte;
	nr := sys->read(fd, buf, len buf);
	if(nr > 0)
		return string buf[0:nr];
	return nil;
}

wf(file, s: string): int
{
	fd := sys->open(file, Sys->ORDWR);
	return sys->write(fd, array of byte s, len s);
}

morenca(in,out: ref sys->FD)
{
	w := ref Wave;
	c := array[1] of byte;

	while(sys->readn(in, c, len c) > 0){
		w.mk(string c);
		if (sys->write(out, w.d, len w.d) < 0)
			warn(sys->sprint("morenca: %r\n"));
	}
}

# uses Goertzel algorithm for signal detection,
# variations in W can help to tune/detect the right freq.
# see http://www.embedded.com/story/OEG20020819S0057

mordeca(in,out: ref sys->FD)
{
	sample:= array[N] of byte;
	st := ref State(0, 0, "");

	k, w: Q28;
	c, q0, q1, q2: Q28;

	# check that N is a good choice
	dprint(sys->sprint("demorse: W %% (ARATE / N) == %d\n", W % (ARATE / N)));
	
	k = Q28((1 + 2 * N * W / ARATE) / 2);	# W is target signal's freq
	w = Q28(2.0 * math->Pi) / Q28(N) * k;	# freq
	c = Q28(2.0 * math->cos(real w));	# coefficient

	for(;;){
		if(sys->read(in, sample, len sample) == 0)
			break;

		q1 = Q28(0);
		q2 = Q28(0);
		for(i:=0; i < len sample; i++){
			q0 = c * q1 - q2 + Q28(sample[i]);
			q2 = q1;
			q1 = q0;
		}

		m2 := q1*q1 + q2*q2 - q1*q2*c;
		slice(int m2, st);
	}

#	mc := ""; for(i:=0; i < len st.m; i++) if(st.m[i] != '_') mc[len mc] = st.m[i];
	sys->fprint(out, "%s\n", st.m);
}

State: adt
{
	t:	int;	# tones
	s: 	int;	# silences
	m:	string;
};

# break input into slices: tone/silence
slice(m: int, s: ref State)
{
	tdit: int = (60*1000)/(wpm*WORDLEN);
	ditlen: int = DITLEN*tdit*TONELEN/N - 1;
	
	istone := (m < -T || +T < m);
	s.t += istone;
	s.s += !istone;

	if(s.t > ditlen){ 
		s.t = 0;
		s.s = 0;
		s.m += ".";
		lenm := len s.m;
		if(lenm >= 3 && s.m[lenm-3:lenm] == "..."){
			s.m = s.m[0:lenm-3] + "-";
		}
	}

	if(s.s > ditlen){
		s.t = 0;
		s.s = 0;
		s.m += "_";
		lenm := len s.m;
		if(lenm >= 3 && s.m[lenm-3:lenm] == "___"){
			s.m = s.m[0:lenm-3] + " ";
		}
	}

	dprint(sys->sprint("mo %d %d %d ", m, s.t, s.s));
	dprint(sys->sprint("m %s\n", s.m));
}

# keyboard press/release timings (ms)
KDITLEN: con 300;	# short press
KDAHLEN: con 500;	# long press

morkbdi(in,out: ref sys->FD)
{
	t, d, k: int;
	c := array[1] of byte;

	wf("/dev/consctl", "rawon");
	while(sys->readn(in, c, 1) > 0) {
		d = sys->millisec() - t;
		if(d <= KDITLEN)
			k = '.';
		else if(d <= KDAHLEN)
			k = '-';
		else
			k = ' ';
		
		dprint(sys->sprint("k %c %d.%d\n", k, d / 1000, d % 1000));
		if(k != ' ')
			sys->fprint(out, "%c", k);
		
		t = sys->millisec();
	}
	sys->fprint(out, "\n");
}

# audio settings
ABITS:	con 8;
AENC:	con "pcm";
ACHANS:	con 1;
ARATE:	con 8000;

# test/set the audio settings
audioctl(set: int)
{
	audioctl :=  array [] of {
		"bits "	+ string ABITS,
		"enc "	+ string AENC,
		"chans "+ string ACHANS,
		"rate "	+ string ARATE,
	};

	if(set){
		for(i:=0; i < len audioctl; i++)
			if(wf("/dev/audioctl", audioctl[i]) != len audioctl[i])
				warn("audioctl: bad write\n");
	}else{
		# TODO: read on audioctl sigsegv/linux
		s := rf("/dev/audioctl");
		for(i:=0; i < len audioctl; i++)
			warn("audioctl: bad read\n");
	}
}

dflag: int;
dprint(s: string)
{
	if(dflag)
		sys->fprint(sys->fildes(2), "%s", s);
}

warn(s: string)
{
	sys->fprint(sys->fildes(2), "%s", s);
}

usage(diag: string)
{
	sys->print("usage: morse [-Dic] [-atde] [-w wpm] [-A amp] [-W freq]\n%s", diag);
}

init(nil: ref Draw->Context, args: list of string)
{
	sys = load Sys Sys->PATH;
	arg = load Arg Arg->PATH;
	str = load String String->PATH;
	math = load Math Math->PATH;

	stdin := sys->fildes(0);
	stdout := sys->fildes(1);

	wpm = DFLTWPM;
	(A, W, P) = (DFLTAMP, DFLTFREQ, DFLTPHASE);
	(N, T) = (DFLTBLK, DFLTTHR);

	mode := 0;
	arg->init(args);
	while((c := arg->opt()) != 0)
		case c {
		'a' => mode |= AUD;
		't' => mode &= ~AUD;
		'd' => mode &= ~ENC;
		'e' => mode |= ENC;

		'k' => morkbdi(stdin, stdout);
		
		'w' => (wpm, nil) = str->toint(arg->arg(), 10);
		'A' => (A, nil) = str->toint(arg->arg(), 10);
		'W' => (W, nil) = str->toint(arg->arg(), 10);
		'P' => (P, nil) = str->toint(arg->arg(), 10);

		'N' => (N, nil) = str->toint(arg->arg(), 10);
		'T' => (T, nil) = str->toint(arg->arg(), 10);

		'D' => dflag++;
		'c' => audioctl(1);

		* => usage(nil); exit;
	}
	args = arg->argv();

	Wave.init(A, W, P);

	dprint(sys->sprint("mode %x\n", mode));
	if(mode & AUD){
		# audioctl(0);
		if(mode & ENC)
			morenca(stdin, stdout);
		else
			mordeca(stdin, stdout);
	}else{
		if(mode & ENC)
			morenct(stdin, stdout);
		else
			mordect(stdin, stdout);
	}
}
