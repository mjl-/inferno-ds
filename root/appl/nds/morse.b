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
include "rand.m";
	rand: Rand;
include "daytime.m";
	daytime: Daytime;

Morse: module
{
	Q20: type fixed(2.0**-20);		# fixed arith, choosen to fit precision=1/ARATE
	Q28: type fixed(2.0**-2, 2.0**28);	# fixed arith, for Goertzel algorithm

	# LENs provided in number of morse elements, using DIT as unit
	DITLEN:	con 1;	# dit element '.' (short pulse)
	DAHLEN:	con 3;	# dah element '-' (long  pulse)
	
	ESPCLEN:con 1;	# inter element space
	LSPCLEN:con 3;	# inter letter space: ' '
	WSPCLEN:con 7;	# inter word space: '/'
	WORDLEN:con 50;	# dits of reference word "PARIS"

	ENC, AUD: con 1<<iota;	# modes

	wpm:	int;		# words per minute (words measured as WORDLEN dits)

	# signal's parameters & default values DFLT*
	A:	int;		# amp
	W:	int;		# freq
	P:	int;		# phase
	R:	int;		# noise
	Z:	int;		# zero

	N:	int;		# Goertzel samples/block
	T:	int;		# slice detection threshold
	
	Byte:		con 1<<8;

	DFLTWPM:	con 15;
	DFLTAMP:	con 2*Byte/5;
	DFLTFREQ:	con 700;
	DFLTPHASE:	con 0;
	DFLTRND:	con 0;
	DFLTZERO:	con (Byte/2);

	DFLTBLK:	con 100;
	DFLTTHR:	con 0;
	
	wave:		ref Wave;

	Wave: adt
	{
		d:	array of byte;	# data samples
		n: 	int;			# num samples

		# tone & space hold the n samples of a period T:
		# n=len tone;	T=1/(W/ARATE);	i=1/NPOINTS,  T=i*n
		tone:	array of byte;
		space:	array of byte;

		init:	fn(s: self ref Wave, A, W, P, R, Z: int);
		cp:		fn(s: self ref Wave, d: array of byte, n,tdit: int);
		mk:		fn(s: self ref Wave, morse: string);
	};

	init:		fn(ctxt: ref Draw->Context, argv: list of string);

	morenct:	fn(in,out: ref Sys->FD);	# text to morse
	mordect:	fn(in,out: ref Sys->FD);	# morse to text
	morenca:	fn(in,out: ref Sys->FD);	# morse to audio
	mordeca:	fn(in,out: ref Sys->FD);	# audio to morse
	morkbdi:	fn(in,out: ref Sys->FD);	# keyboard to morse
};

# NOTE amorse is lexicographically sorted (using as key ascii letters)
amorse := array[] of {
	("\n", "...-.-"),	(" ", " "),		("!", "-.-.--"),	("\"", ".-..-."),
	("#", "-...-.-"),	("$", "...-..-"),	("&", "-.-.-"),		("'", ".----."),
	("(", "-.--."),		(")", "-.--.-"),	("*", ".-..."),		("+", ".-.-."),
	(",", "--..--"),	("-", "-....-"),	(".", ".-.-.-"),	("/", "-..-."),
	
	("0", "-----"),		("1", ".----"),		("2", "..---"),		("3", "...--"),	
	("4", "....-"),		("5", "....."),		("6", "-...."),		("7", "--..."),	
	("8", "---.."),		("9", "----."),

	(":", "---..."),	(";", "-.-.-."), 	("=", "-...-"),		("?", "..--.."),
	("@", ".--.-."),
	
	("A", ".-"),		("B", "-..."),		("C", "-.-."),		("D", "-.."),
	("E", "."),		("F", "..-."),		("G", "--."),		("H", "...."),
	("I", ".."),		("J", ".---"),		("K", "-.-"),		("L", ".-.."),
	("M", "--"),		("N", "-."),		("O", "---"),		("P", ".--."),
	("Q", "--.-"),		("R", ".-."),		("S", "..."),		("T", "-"),
	("U", "..-"),		("V", "...-"),		("W", ".--"),		("X", "-..-"),
	("Y", "-.--"),		("Z", "--.."),

	("_", "..--.-"),
	("Á", ".--.-"),		("Ä", ".-.-"),		("É", "..-.."),		("Ñ", "--.--"),
	("Ö", "---."),		("Ü", "..--"),
};

NPOINTS: con 1;		
# http://www.comportco.com/~w5alt/cw/cwindex.php?pg=5
Wave.init(s: self ref Wave, A, W, P, R, Z: int)
{
	wlen := (ARATE/W) / NPOINTS;
	s.tone = array[wlen] of byte;
	s.space = array[wlen] of byte;

	dpiw := Q20(2.0 * real W * math->Pi / real ARATE);
	for (i := 0; i < wlen; i++)
	{
		s.space[i] = byte Z;
		s.tone[i] = byte Z + byte (real A * math->sin(real (dpiw * Q20(i) + Q20(P))));
		
		if(R){
			s.space[i] +=  byte (R/2 - rand->rand(R));
			s.tone[i] += byte (R/2 - rand->rand(R));
		}

		# dprints the tone to fix it at compile time
		dprint(sys->sprint("byte 16r%.2X, ", int s.tone[i]));
		if(i % 4 == 3)
			dprint("\n");
	}
}

# attenuated A
att(A, i, n: int): int
{
	b := n/2;
	c := n/2;
	return A * int math->exp(- real ((i-b)*(i-b)/(2*c*c)) );
}

Wave.cp(s: self ref Wave, d: array of byte, n,tdit: int)
{
	i: int;
	m: int;

	m=n*tdit;
	for (i=0; i<m; i++){
		s.d[s.n:] = d;
		s.n += len d;
	}
}

wpm2tdit(wpm: int): int
{
	MIN2MS: con 60*1000;
	
	msxdit := (MIN2MS)/(wpm*WORDLEN); # in ms
	return msxdit;
}

Wave.mk(s: self ref Wave, morse: string)
{
	n, x: int; 	# num of dits
	tdit: int;	# dit time (in ms)
	
	tdit = wpm2tdit(wpm);
	comp := Q20(0);		# compression factor
	if(wpm <= 12){
		# for low wpm compress the elements,
		# stretching the silences to maintain wpm
		tdit12 := wpm2tdit(wpm);
		comp = (Q20(tdit) / Q20(tdit12)) - Q20(1);
		tdit = tdit12;
	}

	# TODO this first pass calculates the samples array size
	n = x = 0;
	for (i := 0; i < len morse; i++){
		c := morse[i];
		if (c == '.'){
			x += DITLEN + ESPCLEN;
			n += DITLEN + ESPCLEN;
		}else if (c == '-'){
			x += DAHLEN + ESPCLEN;
			n += DAHLEN + ESPCLEN;
		}else if (c == ' '){
			x += LSPCLEN + ESPCLEN;
			n += LSPCLEN + int (Q20(x - ESPCLEN) * comp);
			x = 0;
		}else if (c == '/'){
			x += WSPCLEN + ESPCLEN;
			n += WSPCLEN + int (Q20(x - ESPCLEN) * comp);
			x = 0;
		}
	}
	
	s.n = 0;
	s.d = array[n*tdit*len s.tone] of byte;
	
	n = x = 0;
	for (i = 0; i < len morse; i++){
		c := morse[i];
		if (c == '.'){
			x += DITLEN + ESPCLEN;
			s.cp(s.tone, DITLEN, tdit);
			s.cp(s.space, ESPCLEN, tdit);
		}else if (c == '-'){
			x += DAHLEN + ESPCLEN;
 			s.cp(s.tone, DAHLEN, tdit);
			s.cp(s.space, ESPCLEN, tdit);
		}else if (c == ' '){
			x += LSPCLEN + ESPCLEN;
			s.cp(s.space, LSPCLEN + int (Q20(x - ESPCLEN) * comp), tdit);
			x = 0;
		}else if (c == '/'){
			x += WSPCLEN + ESPCLEN;
			s.cp(s.space, WSPCLEN + int (Q20(x - ESPCLEN) * comp), tdit);
			x = 0;
		}
	}
}

# encode text to morse
morenct(in,out: ref sys->FD)
{
	c:= array[1] of byte;
	i: int;

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
			dprint(sys->sprint("\n  dec i %d b %d\n", i, b));
			if(i == 0){
				if(b >= 1){
					sys->fprint(out, " ");
					b = 0;
				}else
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
			cc = array[len cc] of { * => byte 0};
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
	c := array[1] of byte;

	while(sys->readn(in, c, len c) > 0){
		wave.mk(string c);
		if (sys->write(out, wave.d, len wave.d) < 0)
			warn(sys->sprint("morenca: %r\n"));
	}
}

# uses Goertzel algorithm for signal detection,
# use variations in W to match the targetet freq.
# see http://www.embedded.com/story/OEG20020819S0057

mordeca(in,out: ref sys->FD)
{
	sample:= array[N] of byte;
	st := ref State(0, 0, "");
	i, n: int;

	k, w: Q28;
	c, q0, q1, q2: Q28;

	# check that N is a good choice
	dprint(sys->sprint("demorse: W %% (ARATE / N) == %d\n", W % (ARATE / N)));
	
	k = Q28(1/2) + Q28(N * W / ARATE);
	w = Q28(2.0 * math->Pi) / Q28(N) * k;	# freq
	c = Q28(2.0 * math->cos(real w));	# coefficient

	mt := Q28(0);	# signal power
	for(;;){
		if((n = sys->read(in, sample, len sample)) == 0)
			break;

		q1 = Q28(0);
		q2 = Q28(0);
		for(i=0; i < n; i++){
			s := Q28(sample[i] - byte Z);

			q0 = c * q1 - q2 + s;
			q2 = q1;
			q1 = q0;

			mt += s*s;
		}

		m := q1*q1 + q2*q2 - q1*q2*c;
		slice(int m, int mt, st);
	}

	sys->fprint(out, "%s\n", st.m);
}

State: adt
{
	t:	int;	# tones
	s: 	int;	# silences
	m:	string;
};

# break input into slices: tone/silence
slice(m, mt: int, s: ref State)
{
	tdit: int;
	ditlen: int;
	
	tdit = wpm2tdit(wpm);
	ditlen = DITLEN*tdit*len wave.tone/N;
	
	istone := (m < -T || T < m); # TODO find a formula for T=f(m,mt,...)
	s.t += istone;
	s.s += !istone;
	
	# TODO ignore impossible transitions
	if(s.t >= ditlen){ 
		s.t = 0;
		s.s = 0;
		s.m += ".";
		lenm := len s.m;

		# DAH
		if(lenm >= DAHLEN && s.m[lenm-DAHLEN:lenm] == "..."){
			s.m = s.m[0:lenm-DAHLEN] + "-";
		}
	}

	if(s.s >= ditlen){
		s.t = 0;
		s.s = 0;
		s.m += "_";
		lenm := len s.m;

		# ESPC
		if(lenm >= LSPCLEN && s.m[lenm-LSPCLEN:lenm] == "___"){
			s.m = s.m[0:lenm-LSPCLEN] + " ";
		}
	}

	dprint(sys->sprint("m %d mt %d %d %d %d ", m, mt, s.t, s.s, ditlen));
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
				warn(sys->sprint("audioctl wf: %r\n"));
	}else{
		# TODO: read on audioctl sigsegv/linux
		s := rf("/dev/audioctl");
		for(i:=0; i < len audioctl; i++)
			warn(sys->sprint("audioctl rf: %r\n"));
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

usage(s: string)
{
	sys->print("usage: morse [-Dic] [-atde] [-w wpm] [-A amp] [-W freq]\n%s", s);
}

init(nil: ref Draw->Context, args: list of string)
{
	sys = load Sys Sys->PATH;
	arg = load Arg Arg->PATH;
	str = load String String->PATH;
	math = load Math Math->PATH;

	rand = load Rand Rand->PATH;
	daytime = load Daytime Daytime->PATH;
	rand->init(daytime->now());
	rand->init(rand->rand(1<<ABITS)^rand->rand(1<<ABITS));
	daytime = nil;

	stdin := sys->fildes(0);
	stdout := sys->fildes(1);

	wpm = DFLTWPM;
	(A, W, P) = (DFLTAMP, DFLTFREQ, DFLTPHASE);
	(R, Z) = (DFLTRND, DFLTZERO);
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
		'R' => (R, nil) = str->toint(arg->arg(), 10);
		'Z' => (Z, nil) = str->toint(arg->arg(), 10);

		'N' => (N, nil) = str->toint(arg->arg(), 10);
		'T' => (T, nil) = str->toint(arg->arg(), 10);

		'D' => dflag++;
		'c' => audioctl(1);

		* => usage(nil); exit;
	}
	args = arg->argv();
	
	wave = ref Wave;
	wave.init(A, W, P, R, Z);
	
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
