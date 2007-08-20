/*---------------------------------------------------------------------------------
	$Id: dma.h,v 1.8 2007/06/28 00:33:22 wntrmute Exp $

	Copyright (C) 2005
		Jason Rogers (dovoto)
		Dave Murphy (WinterMute)

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any
	damages arising from the use of this software.

	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:

	1.	The origin of this software must not be misrepresented; you
		must not claim that you wrote the original software. If you use
		this software in a product, an acknowledgment in the product
		documentation would be appreciated but is not required.

	2.	Altered source versions must be plainly marked as such, and
		must not be misrepresented as being the original software.

	3.	This notice may not be removed or altered from any source
		distribution.

---------------------------------------------------------------------------------*/

#define DMA0_SRC       (*(vuint32*)0x040000B0)
#define DMA0_DEST      (*(vuint32*)0x040000B4)
#define DMA0_CR        (*(vuint32*)0x040000B8)

#define DMA1_SRC       (*(vuint32*)0x040000BC)
#define DMA1_DEST      (*(vuint32*)0x040000C0)
#define DMA1_CR        (*(vuint32*)0x040000C4)

#define DMA2_SRC       (*(vuint32*)0x040000C8)
#define DMA2_DEST      (*(vuint32*)0x040000CC)
#define DMA2_CR        (*(vuint32*)0x040000D0)

#define DMA3_SRC       (*(vuint32*)0x040000D4)
#define DMA3_DEST      (*(vuint32*)0x040000D8)
#define DMA3_CR        (*(vuint32*)0x040000DC)

#define DMA_SRC(n)     (*(vuint32*)(0x040000B0+(n*12)))
#define DMA_DEST(n)    (*(vuint32*)(0x040000B4+(n*12)))
#define DMA_CR(n)      (*(vuint32*)(0x040000B8+(n*12)))
#define DMA_ENABLE      BIT(31)
#define DMA_BUSY	    BIT(31)
#define DMA_IRQ_REQ     BIT(30)

#define DMA_START_NOW   0
#define DMA_START_CARD  (5<<27)



#define DMA_16_BIT      0
#define DMA_32_BIT      BIT(26)


#define DMA_REPEAT      BIT(25)

#define DMA_SRC_INC     (0)
#define DMA_SRC_DEC     BIT(23)
#define DMA_SRC_FIX     BIT(24)

#define DMA_DST_INC     (0)
#define DMA_DST_DEC     BIT(21)
#define DMA_DST_FIX     BIT(22)
#define DMA_DST_RESET   (3<<21)

#define DMA_COPY_WORDS     (DMA_ENABLE | DMA_32_BIT | DMA_START_NOW)
#define DMA_COPY_HALFWORDS (DMA_ENABLE | DMA_16_BIT | DMA_START_NOW)
#define DMA_FIFO	(DMA_ENABLE | DMA_32_BIT  | DMA_DST_FIX | DMA_START_FIFO)

