/*---------------------------------------------------------------------------------
	$Id: system.h,v 1.22 2007/07/23 12:50:07 wntrmute Exp $

	Power control, keys, and HV clock registers

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

/* 
  packed_struct {
    unsigned language                   : 3;    //!<    User's language.
    unsigned gbaScreen                  : 1;    //!<    GBA screen selection (lower screen if set, otherwise upper screen).
    unsigned defaultBrightness  : 2;    //!<    Brightness level at power on, dslite.
    unsigned autoMode                   : 1;    //!<    The DS should boot from the DS cart or GBA cart automatically if one is inserted.
    unsigned RESERVED1                  : 2;    //!<    ???
        unsigned settingsLost           : 1;    //!<    User Settings Lost (0=Normal, 1=Prompt/Settings Lost)
        unsigned RESERVED2                      : 6;    //!<    ???
  } _user_data;
 */

typedef struct tPERSONAL_DATA {
  u8 version;
  u8 color;

  u8 theme;					//	The user's theme color (0-15).
  u8 birthMonth;			//	The user's birth month (1-12).
  u8 birthDay;				//	The user's birth day (1-31).

  u8 RESERVED1[1];			//	???

  Rune name[10];			//	The user's name in UTF-16 format.
  u16 nameLen;				//	The length of the user's name in characters.

  Rune message[26];			//	The user's message.
  u16 messageLen;			//	The length of the user's message in characters.

  u8 alarmHour;				//	What hour the alarm clock is set to (0-23).
  u8 alarmMinute;			//	What minute the alarm clock is set to (0-59).
       //     0x027FFCD3  alarm minute

  u8 RESERVED2[2];			//	???
  u16 alarmOn;
      //     0x027FFCD4  ??

  u16 calX1;				//	Touchscreen calibration: first X touch
  u16 calY1;				//	Touchscreen calibration: first Y touch
  u8 calX1px;				//	Touchscreen calibration: first X touch pixel
  u8 calY1px;				//	Touchscreen calibration: first X touch pixel

  u16 calX2;				//	Touchscreen calibration: second X touch
  u16 calY2;				//	Touchscreen calibration: second Y touch
  u8 calX2px;				//	Touchscreen calibration: second X touch pixel
  u8 calY2px;				//	Touchscreen calibration: second Y touch pixel

  u16 flags;				//	Described above: see struct _user_data

  u16	RESERVED3;
  u32	rtcOffset;
  u32	RESERVED4;
}  PERSONAL_DATA ;

/* location to store user's personal data */
#define PersonalData ((PERSONAL_DATA*)0x27FFC80)

