/*---------------------------------------------------------------------------------
  $Id: bios.h,v 1.10 2007/06/25 20:23:26 wntrmute Exp $

  BIOS functions

  Copyright (C) 2005
    Michael Noland (joat)
    Jason Rogers (dovoto)
    Dave Murphy (WinterMute)

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any
  damages arising from the use of this software.

  Permission is granted to anyone to use this software for any
  purpose, including commercial applications, and to alter it and
  redistribute it freely, subject to the following restrictions:

  1.  The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.
  2.  Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.
  3.  This notice may not be removed or altered from any source
      distribution.

---------------------------------------------------------------------------------*/

/*!	\file bios.h

	\brief Nintendo DS Bios functions
*/





#pragma hjdicks 1
#pragma packed 1
typedef struct DecompressionStream {
  int (*getSize)(uint8 * source, uint16 * dest, uint32 r2);
  int (*getResult)(uint8 * source);  // can be NULL
  uint8 (*readByte)(uint8 * source);
}  TDecompressionStream;


typedef struct UnpackStruct {
  uint16 sourceSize;  // in bytes
  uint8 sourceWidth;
  uint8 destWidth;
  uint32 dataOffset;
}  TUnpackStruct, * PUnpackStruct;

#pragma hjdicks 0
#pragma packed 0

/*! \fn swiSoftReset()
	\brief reset the DS.

*/
void swiSoftReset(void);

/*! \fn swiDelay( uint32 duration)
	\brief delay

   Delays for for a period X + Y*duration where X is the swi overhead and Y is a cycle of
<CODE><PRE>
     loop:
       sub r0, #1
       bgt loop
</PRE></CODE>
  of thumb fetches in BIOS memory
	\param duration
		length of delay
	\note
		Duration should be 1 or more, a duration of 0 is a huge delay

*/
void swiDelay(uint32 duration);

/*! \fn swiIntrWait(int waitForSet, uint32 flags)

	\brief wait for interrupt(s) to occur

	\param waitForSet
		0: Return if the interrupt has already occured
		1: Wait until the interrupt has been set since the call
	\param flags
		interrupt mask to wait for

*/

void swiIntrWait(int waitForSet, uint32 flags);

/*! \fn  swiWaitForVBlank()
	\brief Wait for vblank interrupt

	Waits for a vertical blank interrupt

	\note Identical to calling swiIntrWait(1, 1)
*/

void swiWaitForVBlank(void);

/*!	\fn int swiDivide(int numerator, int divisor)
	\param numerator
		signed integer to divide
	\param divisor
		signed integer to divide by
	\return numerator / divisor

	\fn int swiRemainder(int numerator, int divisor)
	\param numerator
		signed integer to divide
	\param divisor
		signed integer to divide by
	\return numerator % divisor

	\fn void swiDivMod(int numerator, int divisor, int * res, int * remainder)
	\param numerator
		signed integer to divide
	\param divisor
		signed integer to divide by
	\param res
		pointer to integer set to numerator / divisor
	\param remainder
		pointer to integer set to numerator % divisor
*/

int swiDivide(int numerator, int divisor);
int swiRemainder(int numerator, int divisor);
void swiDivMod(int numerator, int divisor, int * res, int * remainder);

/*!	\fn swiCopy(const void * source, void * dest, int flags)
	\param source
		pointer to transfer source
	\param dest
		dest = pointer to transfer destination
	\param flags
		copy mode and size
  flags(26) = transfer width (0: halfwords, 1: words)
  flags(24) = transfer mode (0: copy, 1: fill)
  flags(20..0) = transfer count (always in words)

	\fn swiFastCopy (const void * source, void * dest, int flags)
	\param source
		pointer to transfer source
	\param dest
		dest = pointer to transfer destination
	\param flags
		copy mode and size
  flags(24) = transfer mode (0: copy, 1: fill)
  flags(20..0) = transfer count (in words)

	\note Transfers more quickly than swiCopy, but has higher interrupt latency
*/

#define COPY_MODE_HWORD  (0)
#define COPY_MODE_WORD  (1<<26)
#define COPY_MODE_COPY  (0)
#define COPY_MODE_FILL  (1<<24)

void swiCopy(const void * source, void * dest, int flags);
void swiFastCopy(const void * source, void * dest, int flags);
int swiSqrt(int value);
uint16 swiCRC16(uint16 crc, void * data, uint32 size);
int swiIsDebugger(void);
void swiUnpackBits(uint8 * source, uint32 * destination, PUnpackStruct params);
void swiDecompressLZSSWram(void * source, void * destination);
int swiDecompressLZSSVram(void * source, void * destination, uint32 toGetSize, TDecompressionStream * stream);
int swiDecompressHuffman(void * source, void * destination, uint32 toGetSize, TDecompressionStream * stream);
void swiDecompressRLEWram(void * source, void * destination);
int swiDecompressRLEVram(void * source, void * destination, uint32 toGetSize, TDecompressionStream * stream);
/*
 swiDecodeDelta8
   source      - pointer to a header word, followed by encoded data
                 word(31..8) = size of data (in bytes)
                 word(7..0) = ignored
   destination - destination address
 Writes data a byte at a time

 Note: ARM9 exclusive swi 0x16

 swiHalt (swi 0x06)
   Same as swiSetHaltCR(0x80)

 swiSleep (swi 0x07)
   Same as swiSetHaltCR(0xC0)

 swiSwitchToGBAMode (not a SWI)
   Same as swiSetHaltCR(0x40)

 swiSetHaltCR (swi 0x1F)
   Writes a byte of the data to 0x04000301:8

 Note: All of these are ARM7 exclusive

 swiSetHaltCR (swi 0x1F)
   Writes a word of the data to 0x04000300:32

 Note: This is on the ARM9, but works differently to the ARM7 function!
*/

void swiHalt(void);
void swiSleep(void);
void swiSwitchToGBAMode(void);
void swiSetHaltCR(uint8 data);
/*
 swiGetSineTable(int index)
  Returns an entry in the sine table (index = 0..63)

 Note: ARM7 exclusive swi 0x1A
*/
uint16 swiGetSineTable(int index);
uint16 swiGetPitchTable(int index);
uint8 swiGetVolumeTable(int index);
void swiChangeSoundBias(int enabled, int delay);


