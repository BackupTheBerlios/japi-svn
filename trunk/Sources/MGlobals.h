/*
	Copyright (c) 2006, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*	$Id: MGlobals.h 133 2007-05-01 08:34:48Z maarten $
	Copyright Maarten L. Hekkelman
	Created Thursday July 22 2004 22:10:13
*/

#ifndef MYGLOBALS_H
#define MYGLOBALS_H

#include "MFile.h"
#include "MColor.h"
#include <string>

const uint32			kJapieSignature = 'Japi';

extern bool				gAutoIndent;
extern bool				gSmartIndent;
extern bool				gKiss;
extern bool				gSmoothFonts;
extern bool				gShowInvisibles;
extern bool				gTabEntersSpaces;
extern uint32			gCharsPerTab;
extern uint32			gSpacesPerTab;

extern uint32			gFontSize;
extern std::string		gFontName;

extern MURL				gTemplatesDir, gPrefsDir;

extern MColor			gLanguageColors[];
extern MColor			gHiliteColor, gInactiveHiliteColor, gOddRowColor;

void InitGlobals();
void SaveGlobals();

#endif // MYGLOBALS_H
