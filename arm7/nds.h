/*---------------------------------------------------------------------------------
	$Id: nds.h,v 1.13 2007/06/28 00:52:49 wntrmute Exp $

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


	1.	The origin of this software must not be misrepresented; you
		must not claim that you wrote the original software. If you use
		this software in a product, an acknowledgment in the product
		documentation would be appreciated but is not required.

	2.	Altered source versions must be plainly marked as such, and
		must not be misrepresented as being the original software.

	3.	This notice may not be removed or altered from any source
		distribution.

	$Log: nds.h,v $
	Revision 1.13  2007/06/28 00:52:49  wntrmute
	split out background defines
	
	Revision 1.12  2007/06/28 00:32:47  wntrmute
	add sprite.h
	
	Revision 1.11  2006/07/04 01:44:31  wntrmute
	remove CP15 header
	
	Revision 1.10  2006/06/18 21:16:26  wntrmute
	added arm9 exception handler API

	Revision 1.9  2006/04/26 05:52:37  wntrmute
	added all headers to main nds header

	Revision 1.8  2005/08/31 01:09:46  wntrmute
	removed spurious comment

	Revision 1.7  2005/08/23 17:06:09  wntrmute
	converted all endings to unix


---------------------------------------------------------------------------------*/
#include "jtypes.h"
#include "bios.h"
#include "ipc.h"
#include "system.h"
#include "audio.h"
#include "rtc.h"
#include "touch.h"
#include "spi.h"
