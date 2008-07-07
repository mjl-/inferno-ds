#include	"u.h"
#include	"io.h"
#include	"mem.h"
#include 	"arm7/jtypes.h"
#include	"arm7/card.h"

/* TODO
 * - math.c could be in archds.c	(optional)
 * - fifo.c could also be in archds.c 	(required)
 * - ttio.c + discio.c -> r4tf.c and card.c in arm7
 */

/*
misc/ttio.dldi:     file format binary

Disassembly of section .data:

00000000 <.data>:
   0:	bf8da5ed 	swilt	0x008da5ed
   4:	69684320 	stmvsdb	r8!, {r5, r8, r9, lr}^
   8:	006d6873 	rsbeq	r6, sp, r3, ror r8
   c:	000e0d01 	andeq	r0, lr, r1, lsl #26
  10:	41435454 	cmpmi	r3, r4, asr r4
  14:	49204452 	stmmidb	r0!, {r1, r4, r6, sl, lr}
  18:	694c204f 	stmvsdb	ip, {r0, r1, r2, r3, r6, sp}^
  1c:	72617262 	rsbvc	r7, r1, #536870918	; 0x20000006
  20:	20202079 	eorcs	r2, r0, r9, ror r0
  24:	20202020 	eorcs	r2, r0, r0, lsr #32
  28:	00002020 	andeq	r2, r0, r0, lsr #32
	...
  40:	bf800000 	swilt	0x00800000
  44:	bf80068c 	swilt	0x0080068c
  48:	bf800098 	swilt	0x00800098
  4c:	bf800098 	swilt	0x00800098
  50:	bf800688 	swilt	0x00800688
  54:	bf800688 	swilt	0x00800688
  58:	bf80068c 	swilt	0x0080068c
  5c:	bf8006a8 	swilt	0x008006a8
  60:	4f495454 	swimi	0x00495454					// ioifc.type
  64:	00000023 	andeq	r0, r0, r3, lsr #32				// ioifc.caps
  68:	bf800598 	swilt	0x00800598					// ioifc.init
  6c:	bf8005a0 	swilt	0x008005a0					// ioifc.isin
  70:	bf8005a8 	swilt	0x008005a8					// ioifc.read
  74:	bf8005fc 	swilt	0x008005fc					// ioifc.write
  78:	bf800650 	swilt	0x00800650					// ioifc.clrstat
  7c:	bf800658 	swilt	0x00800658					// ioifc.deinit
*/

/*
  80:	e1a0c00d 	mov	ip, sp
  84:	e92ddff8 	stmdb	sp!, {r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
  88:	e24cb004 	sub	fp, ip, #4	; 0x4
  8c:	e24bd028 	sub	sp, fp, #40	; 0x28
  90:	e89d6ff0 	ldmia	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
  94:	e12fff1e 	bx	lr
  98:	e92d4010 	stmdb	sp!, {r4, lr}
  9c:	e59f402c 	ldr	r4, [pc, #44]	; 0xd0
  a0:	e5d43000 	ldrb	r3, [r4]
  a4:	e3530000 	cmp	r3, #0	; 0x0
  a8:	e59f2024 	ldr	r2, [pc, #36]	; 0xd4
  ac:	1a000005 	bne	0xc8
  b0:	e3520000 	cmp	r2, #0	; 0x0
  b4:	e59f001c 	ldr	r0, [pc, #28]	; 0xd8
  b8:	11a0e00f 	movne	lr, pc
  bc:	112fff12 	bxne	r2
  c0:	e3a03001 	mov	r3, #1	; 0x1
  c4:	e5c43000 	strb	r3, [r4]
  c8:	e8bd4010 	ldmia	sp!, {r4, lr}
  cc:	e12fff1e 	bx	lr
  d0:	bf80068c 	swilt	0x0080068c
  d4:	00000000 	andeq	r0, r0, r0
  d8:	bf800680 	swilt	0x00800680
  dc:	e52de004 	str	lr, [sp, #-4]!
  e0:	e59f3040 	ldr	r3, [pc, #64]	; 0x128
  e4:	e3530000 	cmp	r3, #0	; 0x0
  e8:	e24dd004 	sub	sp, sp, #4	; 0x4
  ec:	e59f0038 	ldr	r0, [pc, #56]	; 0x12c
  f0:	e59f1038 	ldr	r1, [pc, #56]	; 0x130
  f4:	11a0e00f 	movne	lr, pc
  f8:	112fff13 	bxne	r3
  fc:	e59f0030 	ldr	r0, [pc, #48]	; 0x134
 100:	e5903000 	ldr	r3, [r0]
 104:	e3530000 	cmp	r3, #0	; 0x0
 108:	e59f3028 	ldr	r3, [pc, #40]	; 0x138
 10c:	0a000002 	beq	0x11c
 110:	e3530000 	cmp	r3, #0	; 0x0
 114:	11a0e00f 	movne	lr, pc
 118:	112fff13 	bxne	r3
 11c:	e28dd004 	add	sp, sp, #4	; 0x4
 120:	e49de004 	ldr	lr, [sp], #4
 124:	e12fff1e 	bx	lr
 128:	00000000 	andeq	r0, r0, r0
 12c:	bf800680 	swilt	0x00800680
 130:	bf800690 	swilt	0x00800690
 134:	bf800684 	swilt	0x00800684
 138:	00000000 	andeq	r0, r0, r0
 13c:	e92d40f0 	stmdb	sp!, {r4, r5, r6, r7, lr}
 140:	e3a0e301 	mov	lr, #67108864	; 0x4000000
 144:	e3a03301 	mov	r3, #67108864	; 0x4000000
 148:	e1a04822 	mov	r4, r2, lsr #16
 14c:	e1a05422 	mov	r5, r2, lsr #8
 150:	e28eec01 	add	lr, lr, #256	; 0x100
 154:	e20270ff 	and	r7, r2, #255	; 0xff
 158:	e3a064a7 	mov	r6, #-1493172224	; 0xa7000000
 15c:	e1a02c22 	mov	r2, r2, lsr #24
 160:	e3a0c051 	mov	ip, #81	; 0x51
 164:	e5cec0a8 	strb	ip, [lr, #168]
 168:	e20440ff 	and	r4, r4, #255	; 0xff
 16c:	e5c321a9 	strb	r2, [r3, #425]
 170:	e20550ff 	and	r5, r5, #255	; 0xff
 174:	e3a02000 	mov	r2, #0	; 0x0
 178:	e2866706 	add	r6, r6, #1572864	; 0x180000
 17c:	e5c341aa 	strb	r4, [r3, #426]
 180:	e5c351ab 	strb	r5, [r3, #427]
 184:	e5ce70ac 	strb	r7, [lr, #172]
 188:	e5c311ad 	strb	r1, [r3, #429]
 18c:	e5c301ae 	strb	r0, [r3, #430]
 190:	e5c321af 	strb	r2, [r3, #431]
 194:	e58361a4 	str	r6, [r3, #420]
 198:	e1a02003 	mov	r2, r3
 19c:	e59231a4 	ldr	r3, [r2, #420]
 1a0:	e3130502 	tst	r3, #8388608	; 0x800000
 1a4:	0afffffc 	beq	0x19c
 1a8:	e3a03641 	mov	r3, #68157440	; 0x4100000
 1ac:	e5932010 	ldr	r2, [r3, #16]
 1b0:	e8bd40f0 	ldmia	sp!, {r4, r5, r6, r7, lr}
 1b4:	e12fff1e 	bx	lr
 1b8:	e3a02301 	mov	r2, #67108864	; 0x4000000
 1bc:	e3a014a7 	mov	r1, #-1493172224	; 0xa7000000
 1c0:	e2822c01 	add	r2, r2, #256	; 0x100
 1c4:	e2811706 	add	r1, r1, #1572864	; 0x180000
 1c8:	e3a00301 	mov	r0, #67108864	; 0x4000000
 1cc:	e3a03050 	mov	r3, #80	; 0x50
 1d0:	e5c230a8 	strb	r3, [r2, #168]
 1d4:	e58011a4 	str	r1, [r0, #420]
 1d8:	e59031a4 	ldr	r3, [r0, #420]
 1dc:	e3130502 	tst	r3, #8388608	; 0x800000
 1e0:	0afffffc 	beq	0x1d8
 1e4:	e3a03641 	mov	r3, #68157440	; 0x4100000
 1e8:	e5930010 	ldr	r0, [r3, #16]
 1ec:	e12fff1e 	bx	lr
 1f0:	e3a02301 	mov	r2, #67108864	; 0x4000000
 1f4:	e3a014a7 	mov	r1, #-1493172224	; 0xa7000000
 1f8:	e2822c01 	add	r2, r2, #256	; 0x100
 1fc:	e2811706 	add	r1, r1, #1572864	; 0x180000
 200:	e3a00301 	mov	r0, #67108864	; 0x4000000
 204:	e3a03052 	mov	r3, #82	; 0x52
 208:	e5c230a8 	strb	r3, [r2, #168]
 20c:	e58011a4 	str	r1, [r0, #420]
 210:	e59031a4 	ldr	r3, [r0, #420]
 214:	e3130502 	tst	r3, #8388608	; 0x800000
 218:	0afffffc 	beq	0x210
 21c:	e3a03641 	mov	r3, #68157440	; 0x4100000
 220:	e5930010 	ldr	r0, [r3, #16]
 224:	e12fff1e 	bx	lr
 228:	e52de004 	str	lr, [sp, #-4]!
 22c:	e1a02001 	mov	r2, r1
 230:	e24dd004 	sub	sp, sp, #4	; 0x4
 234:	e1a01000 	mov	r1, r0
 238:	e3a00001 	mov	r0, #1	; 0x1
 23c:	ebffffbe 	bl	0x13c
 240:	ebffffdc 	bl	0x1b8
 244:	e3500000 	cmp	r0, #0	; 0x0
 248:	1afffffc 	bne	0x240
 24c:	e28dd004 	add	sp, sp, #4	; 0x4
 250:	e49de004 	ldr	lr, [sp], #4
 254:	eaffffe5 	b	0x1f0
*/

/*
 258:	e92d40f0 	stmdb	sp!, {r4, r5, r6, r7, lr}
 25c:	e1a04820 	mov	r4, r0, lsr #16
 260:	e1a05420 	mov	r5, r0, lsr #8
 264:	e3a0e301 	mov	lr, #67108864	; 0x4000000
 268:	e3a064a7 	mov	r6, #-1493172224	; 0xa7000000
 26c:	e3a0c301 	mov	ip, #67108864	; 0x4000000
 270:	e28eec01 	add	lr, lr, #256	; 0x100
 274:	e20440ff 	and	r4, r4, #255	; 0xff
 278:	e20070ff 	and	r7, r0, #255	; 0xff
 27c:	e2866706 	add	r6, r6, #1572864	; 0x180000
 280:	e20550ff 	and	r5, r5, #255	; 0xff
 284:	e1a00c20 	mov	r0, r0, lsr #24
 288:	e3a03054 	mov	r3, #84	; 0x54
 28c:	e5ce30a8 	strb	r3, [lr, #168]
 290:	e24dd004 	sub	sp, sp, #4	; 0x4
 294:	e5cc01a9 	strb	r0, [ip, #425]
 298:	e5cc41aa 	strb	r4, [ip, #426]
 29c:	e5cc51ab 	strb	r5, [ip, #427]
 2a0:	e1a04001 	mov	r4, r1
 2a4:	e5ce70ac 	strb	r7, [lr, #172]
 2a8:	e58c61a4 	str	r6, [ip, #420]
 2ac:	e1a06002 	mov	r6, r2
 2b0:	e59c31a4 	ldr	r3, [ip, #420]
 2b4:	e3130502 	tst	r3, #8388608	; 0x800000
 2b8:	0afffffc 	beq	0x2b0
 2bc:	e3a03641 	mov	r3, #68157440	; 0x4100000
 2c0:	e3a07301 	mov	r7, #67108864	; 0x4000000
 2c4:	e5932010 	ldr	r2, [r3, #16]
 2c8:	e2877c01 	add	r7, r7, #256	; 0x100
 2cc:	e3a05301 	mov	r5, #67108864	; 0x4000000
 2d0:	e3a034a7 	mov	r3, #-1493172224	; 0xa7000000
 2d4:	e2833706 	add	r3, r3, #1572864	; 0x180000
 2d8:	e3e0207f 	mvn	r2, #127	; 0x7f
 2dc:	e5c720a8 	strb	r2, [r7, #168]
 2e0:	e58531a4 	str	r3, [r5, #420]
 2e4:	e59531a4 	ldr	r3, [r5, #420]
 2e8:	e3130502 	tst	r3, #8388608	; 0x800000
 2ec:	0afffffc 	beq	0x2e4
 2f0:	e3a01641 	mov	r1, #68157440	; 0x4100000
 2f4:	e5913010 	ldr	r3, [r1, #16]
 2f8:	e3530000 	cmp	r3, #0	; 0x0
 2fc:	1afffff3 	bne	0x2d0
 300:	e28334a1 	add	r3, r3, #-1593835520	; 0xa1000000
 304:	e2833706 	add	r3, r3, #1572864	; 0x180000
 308:	e3e0207e 	mvn	r2, #126	; 0x7e
 30c:	e5c720a8 	strb	r2, [r7, #168]
 310:	e58531a4 	str	r3, [r5, #420]
 314:	e1a0e001 	mov	lr, r1
 318:	e3a0c301 	mov	ip, #67108864	; 0x4000000
 31c:	e59c31a4 	ldr	r3, [ip, #420]
 320:	e3130502 	tst	r3, #8388608	; 0x800000
 324:	0a00000f 	beq	0x368
 328:	e3140003 	tst	r4, #3	; 0x3
 32c:	059e3010 	ldreq	r3, [lr, #16]
 330:	05843000 	streq	r3, [r4]
 334:	0a00000a 	beq	0x364
 338:	e59e3010 	ldr	r3, [lr, #16]
 33c:	e1a02423 	mov	r2, r3, lsr #8
 340:	e1a01823 	mov	r1, r3, lsr #16
 344:	e1a00c23 	mov	r0, r3, lsr #24
 348:	e20220ff 	and	r2, r2, #255	; 0xff
 34c:	e20110ff 	and	r1, r1, #255	; 0xff
 350:	e20330ff 	and	r3, r3, #255	; 0xff
 354:	e5c43000 	strb	r3, [r4]
 358:	e5c42001 	strb	r2, [r4, #1]
 35c:	e5c41002 	strb	r1, [r4, #2]
 360:	e5c40003 	strb	r0, [r4, #3]
 364:	e2844004 	add	r4, r4, #4	; 0x4
 368:	e59c31a4 	ldr	r3, [ip, #420]
 36c:	e3530000 	cmp	r3, #0	; 0x0
 370:	baffffe9 	blt	0x31c
 374:	e2566001 	subs	r6, r6, #1	; 0x1
 378:	0a000004 	beq	0x390
 37c:	e3a01000 	mov	r1, #0	; 0x0
 380:	e3a00007 	mov	r0, #7	; 0x7
 384:	e1a02001 	mov	r2, r1
 388:	ebffff6b 	bl	0x13c
 38c:	eaffffcf 	b	0x2d0
 390:	e3a0000c 	mov	r0, #12	; 0xc
 394:	e3a01000 	mov	r1, #0	; 0x0
 398:	e28dd004 	add	sp, sp, #4	; 0x4
 39c:	e8bd40f0 	ldmia	sp!, {r4, r5, r6, r7, lr}
 3a0:	eaffffa0 	b	0x228
 3a4:	e92d4010 	stmdb	sp!, {r4, lr}
 3a8:	e1a04001 	mov	r4, r1
 3ac:	e1a01000 	mov	r1, r0
 3b0:	e3a00018 	mov	r0, #24	; 0x18
 3b4:	ebffff9b 	bl	0x228
 3b8:	e3a02301 	mov	r2, #67108864	; 0x4000000
 3bc:	e3a014e1 	mov	r1, #-520093696	; 0xe1000000
 3c0:	e3a00301 	mov	r0, #67108864	; 0x4000000
 3c4:	e2822c01 	add	r2, r2, #256	; 0x100
 3c8:	e2811706 	add	r1, r1, #1572864	; 0x180000
 3cc:	e3e0307d 	mvn	r3, #125	; 0x7d
 3d0:	e5c230a8 	strb	r3, [r2, #168]
 3d4:	e58011a4 	str	r1, [r0, #420]
 3d8:	e1a0c000 	mov	ip, r0
 3dc:	e3a0e641 	mov	lr, #68157440	; 0x4100000
 3e0:	e59c31a4 	ldr	r3, [ip, #420]
 3e4:	e3130502 	tst	r3, #8388608	; 0x800000
 3e8:	0a000011 	beq	0x434
 3ec:	e3540000 	cmp	r4, #0	; 0x0
 3f0:	03e03000 	mvneq	r3, #0	; 0x0
 3f4:	058e3010 	streq	r3, [lr, #16]
 3f8:	0a00000d 	beq	0x434
 3fc:	e3140003 	tst	r4, #3	; 0x3
 400:	05943000 	ldreq	r3, [r4]
 404:	058e3010 	streq	r3, [lr, #16]
 408:	0a000008 	beq	0x430
 40c:	e5d42000 	ldrb	r2, [r4]
 410:	e5d41001 	ldrb	r1, [r4, #1]
 414:	e5d40002 	ldrb	r0, [r4, #2]
 418:	e5d43003 	ldrb	r3, [r4, #3]
 41c:	e1a03c03 	mov	r3, r3, lsl #24
 420:	e1822401 	orr	r2, r2, r1, lsl #8
 424:	e1833800 	orr	r3, r3, r0, lsl #16
 428:	e1822003 	orr	r2, r2, r3
 42c:	e58e2010 	str	r2, [lr, #16]
 430:	e2844004 	add	r4, r4, #4	; 0x4
 434:	e59c31a4 	ldr	r3, [ip, #420]
 438:	e3530000 	cmp	r3, #0	; 0x0
 43c:	baffffe7 	blt	0x3e0
 440:	ebffff5c 	bl	0x1b8
 444:	e3500000 	cmp	r0, #0	; 0x0
 448:	1afffffc 	bne	0x440
 44c:	e3a02301 	mov	r2, #67108864	; 0x4000000
 450:	e3a014a7 	mov	r1, #-1493172224	; 0xa7000000
 454:	e2822c01 	add	r2, r2, #256	; 0x100
 458:	e2811706 	add	r1, r1, #1572864	; 0x180000
 45c:	e2800301 	add	r0, r0, #67108864	; 0x4000000
 460:	e3a03056 	mov	r3, #86	; 0x56
 464:	e5c230a8 	strb	r3, [r2, #168]
 468:	e58011a4 	str	r1, [r0, #420]
 46c:	e59031a4 	ldr	r3, [r0, #420]
 470:	e3130502 	tst	r3, #8388608	; 0x800000
 474:	0afffffc 	beq	0x46c
 478:	e3a03641 	mov	r3, #68157440	; 0x4100000
 47c:	e5932010 	ldr	r2, [r3, #16]
 480:	ebffff4c 	bl	0x1b8
 484:	e3500000 	cmp	r0, #0	; 0x0
 488:	1afffffc 	bne	0x480
 48c:	e8bd4010 	ldmia	sp!, {r4, lr}
 490:	e12fff1e 	bx	lr
*/
void
ttio_subread(void){
}

/*
 494:	e3520001 	cmp	r2, #1	; 0x1
 498:	e92d41f0 	stmdb	sp!, {r4, r5, r6, r7, r8, lr}
 49c:	e1a06002 	mov	r6, r2
 4a0:	e1a04001 	mov	r4, r1
 4a4:	0a000039 	beq	0x590
 4a8:	e1a01000 	mov	r1, r0
 4ac:	e3a00019 	mov	r0, #25	; 0x19
 4b0:	ebffff5c 	bl	0x228
 4b4:	e3a08301 	mov	r8, #67108864	; 0x4000000
 4b8:	e2888c01 	add	r8, r8, #256	; 0x100
 4bc:	e3a05301 	mov	r5, #67108864	; 0x4000000
 4c0:	e3a07641 	mov	r7, #68157440	; 0x4100000
 4c4:	e3a034e1 	mov	r3, #-520093696	; 0xe1000000
 4c8:	e2833706 	add	r3, r3, #1572864	; 0x180000
 4cc:	e3e0207d 	mvn	r2, #125	; 0x7d
 4d0:	e5c820a8 	strb	r2, [r8, #168]
 4d4:	e58531a4 	str	r3, [r5, #420]
 4d8:	e59531a4 	ldr	r3, [r5, #420]
 4dc:	e3130502 	tst	r3, #8388608	; 0x800000
 4e0:	0a00000d 	beq	0x51c
 4e4:	e3140003 	tst	r4, #3	; 0x3
 4e8:	05943000 	ldreq	r3, [r4]
 4ec:	05873010 	streq	r3, [r7, #16]
 4f0:	0a000008 	beq	0x518
 4f4:	e5d42000 	ldrb	r2, [r4]
 4f8:	e5d41001 	ldrb	r1, [r4, #1]
 4fc:	e5d40002 	ldrb	r0, [r4, #2]
 500:	e5d43003 	ldrb	r3, [r4, #3]
 504:	e1a03c03 	mov	r3, r3, lsl #24
 508:	e1822401 	orr	r2, r2, r1, lsl #8
 50c:	e1833800 	orr	r3, r3, r0, lsl #16
 510:	e1822003 	orr	r2, r2, r3
 514:	e5872010 	str	r2, [r7, #16]
 518:	e2844004 	add	r4, r4, #4	; 0x4
 51c:	e59531a4 	ldr	r3, [r5, #420]
 520:	e3530000 	cmp	r3, #0	; 0x0
 524:	baffffeb 	blt	0x4d8
 528:	ebffff22 	bl	0x1b8
 52c:	e3500000 	cmp	r0, #0	; 0x0
 530:	1afffffc 	bne	0x528
 534:	e2566001 	subs	r6, r6, #1	; 0x1
 538:	0a000010 	beq	0x580
 53c:	e3a034a7 	mov	r3, #-1493172224	; 0xa7000000
 540:	e3a02056 	mov	r2, #86	; 0x56
 544:	e2833706 	add	r3, r3, #1572864	; 0x180000
 548:	e5c820a8 	strb	r2, [r8, #168]
 54c:	e58531a4 	str	r3, [r5, #420]
 550:	e3a02301 	mov	r2, #67108864	; 0x4000000
 554:	e59231a4 	ldr	r3, [r2, #420]
 558:	e3130502 	tst	r3, #8388608	; 0x800000
 55c:	0afffffc 	beq	0x554
 560:	e5973010 	ldr	r3, [r7, #16]
 564:	ebffff13 	bl	0x1b8
 568:	e3500000 	cmp	r0, #0	; 0x0
 56c:	1afffffc 	bne	0x564
 570:	e3560000 	cmp	r6, #0	; 0x0
 574:	1affffd2 	bne	0x4c4
 578:	e8bd41f0 	ldmia	sp!, {r4, r5, r6, r7, r8, lr}
 57c:	e12fff1e 	bx	lr
 580:	e1a01006 	mov	r1, r6
 584:	e280000c 	add	r0, r0, #12	; 0xc
 588:	ebffff26 	bl	0x228
 58c:	eaffffea 	b	0x53c
 590:	e8bd41f0 	ldmia	sp!, {r4, r5, r6, r7, r8, lr}
 594:	eaffff82 	b	0x3a4
*/
void
ttio_subwrite(void){
}

/*
 598:	e3a00001 	mov	r0, #1	; 0x1
 59c:	e12fff1e 	bx	lr
*/
int
ttio_init(void){
	return 1;
}

/*
 5a0:	e3a00001 	mov	r0, #1	; 0x1
 5a4:	e12fff1e 	bx	lr
*/
int
ttio_isin(void){
	return 1;
}

/*
 5a8:	e52de004 	str	lr, [sp, #-4]!
 5ac:	e3a0c301 	mov	ip, #67108864	; 0x4000000
 5b0:	e3e0e03f 	mvn	lr, #63	; 0x3f
 5b4:	e5cce1a1 	strb	lr, [ip, #417]
 5b8:	e3a0350a 	mov	r3, #41943040	; 0x2800000
 5bc:	e2433001 	sub	r3, r3, #1	; 0x1
 5c0:	e513c3db 	ldr	ip, [r3, #-987]
 5c4:	e1a0e001 	mov	lr, r1
 5c8:	e35c0000 	cmp	ip, #0	; 0x0
 5cc:	e1a0c002 	mov	ip, r2
 5d0:	e1a01002 	mov	r1, r2
 5d4:	01a00480 	moveq	r0, r0, lsl #9
 5d8:	e1a0200e 	mov	r2, lr
 5dc:	e24dd004 	sub	sp, sp, #4	; 0x4
 5e0:	01a0100c 	moveq	r1, ip
 5e4:	01a0200e 	moveq	r2, lr
 5e8:	ebffff1a 	bl	0x258
 5ec:	e3a00001 	mov	r0, #1	; 0x1
 5f0:	e28dd004 	add	sp, sp, #4	; 0x4
 5f4:	e49de004 	ldr	lr, [sp], #4
 5f8:	e12fff1e 	bx	lr
*/
int
ttio_read(ulong start, ulong n, void *d){
	uchar *sb = (uchar*)(0x4000000 + 417);
	uchar *lb = (uchar*)(0x2800000 -1 - 987);

	*sb = 63;
	
	*sb = (uchar)(d);
	if (*lb == 0)
		start <<= 9;
	
	ttio_subread(start, d, n);
	return 0;
}

/*
 5fc:	e52de004 	str	lr, [sp, #-4]!
 600:	e3a0c301 	mov	ip, #67108864	; 0x4000000
 604:	e3e0e03f 	mvn	lr, #63	; 0x3f
 608:	e5cce1a1 	strb	lr, [ip, #417]
 60c:	e3a0350a 	mov	r3, #41943040	; 0x2800000
 610:	e2433001 	sub	r3, r3, #1	; 0x1
 614:	e513c3db 	ldr	ip, [r3, #-987]
 618:	e1a0e001 	mov	lr, r1
 61c:	e35c0000 	cmp	ip, #0	; 0x0
 620:	e1a0c002 	mov	ip, r2
 624:	e1a01002 	mov	r1, r2
 628:	01a00480 	moveq	r0, r0, lsl #9
 62c:	e1a0200e 	mov	r2, lr
 630:	e24dd004 	sub	sp, sp, #4	; 0x4
 634:	01a0100c 	moveq	r1, ip
 638:	01a0200e 	moveq	r2, lr
 63c:	ebffff94 	bl	0x494
 640:	e3a00001 	mov	r0, #1	; 0x1
 644:	e28dd004 	add	sp, sp, #4	; 0x4
 648:	e49de004 	ldr	lr, [sp], #4
 64c:	e12fff1e 	bx	lr
*/
int
ttio_write(ulong start, ulong n, const void *d){
	/* almost the same code that ttio_read */
	ttio_subwrite(start, d, n);
	return 0;
}

/*
 650:	e3a00001 	mov	r0, #1	; 0x1
 654:	e12fff1e 	bx	lr
*/
int
ttio_clrstat(void){
	return 1;
}

/*
 658:	e3a00001 	mov	r0, #1	; 0x1
 65c:	e12fff1e 	bx	lr
*/
int
ttio_deinit(void){
	return 1;
}

/*
 660:	e1a0c00d 	mov	ip, sp
 664:	e92ddff8 	stmdb	sp!, {r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
 668:	e24cb004 	sub	fp, ip, #4	; 0x4
 66c:	e24bd028 	sub	sp, fp, #40	; 0x28
 670:	e89d6ff0 	ldmia	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
 674:	e12fff1e 	bx	lr
 678:	bf8000dc 	swilt	0x008000dc
 67c:	bf800098 	swilt	0x00800098
*/

Ioifc io_ttio = {
	"TTIO",
	Cread|Cwrite|Cslotnds,

	ttio_init,
	ttio_isin,
	(void*)ttio_read,
	(void*)ttio_write,
	ttio_clrstat,
	ttio_deinit
};
