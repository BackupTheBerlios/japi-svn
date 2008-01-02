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

/*	$Id: MGlobals.cpp 151 2007-05-21 15:59:05Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday July 25 2004 21:07:42
*/

#include "MJapieG.h"

#include "MGlobals.h"
#include "MPreferences.h"
#include "MLanguage.h"
//#include "MApplication.h"

using namespace std;

const char kHexChars[] = "0123456789abcdef";

struct RGBColor
{
	uint16	red, green, blue;
};

const MColor
	kTextColor,
	kKeyWordColor("#3d4c9e"),
	kPreProcessorColor("#005454"),
	kCharConstColor("#ad6739"),
	kCommentColor("#9b2e35"),
	kStringColor("#666666"),
	kTagColor("#008484"),
	kAttribColor("#1e843b"),
	kInvisiblesColor("#aaaaaa"),
	kHiliteColor("#ffd281"),
	kCurrentLineColor("#ffffcc"),
	kMarkedLineColor("#efff7f"),
	kPCLineColor = MColor("#cce5ff"),
	kBreakpointColor = MColor("#5ea50c");

const MColor
	kInactiveHighlightColor("#e5e5e5"),
	kOddRowBackColor("#eff7ff");

bool			gAutoIndent = true;
bool			gSmartIndent = true;
bool			gKiss = true;
bool			gSmoothFonts = false;
bool			gShowInvisibles = true;
bool			gTabEntersSpaces = false;
uint32			gCharsPerTab = 4;
uint32			gSpacesPerTab = 4;
uint32			gFontSize = 10;
std::string		gFontName = "Monaco";
MColor			gLanguageColors[kLStyleCount];
MColor			gHiliteColor, gInactiveHiliteColor;
MColor			gCurrentLineColor, gMarkedLineColor;
MColor			gPCLineColor, gBreakpointColor;

MPath			gTemplatesDir, gPrefsDir;

void InitGlobals()
{
	gPrefsDir = g_get_user_config_dir();
	gPrefsDir /= "japi";
	
	const char* templatesDir = g_get_user_special_dir(G_USER_DIRECTORY_TEMPLATES);
	if (templatesDir == nil)
		gTemplatesDir = gPrefsDir / "Templates";
	else
		gTemplatesDir = fs::system_complete(templatesDir);

	gAutoIndent = Preferences::GetInteger("auto indent", gAutoIndent);
	gSmartIndent = Preferences::GetInteger("smart indent", gSmartIndent);
	gKiss = Preferences::GetInteger("kiss", gKiss);
	gCharsPerTab = Preferences::GetInteger("chars per tab", gCharsPerTab);
	gSpacesPerTab = Preferences::GetInteger("spaces per tab", gSpacesPerTab);
//	gSmoothFonts = Preferences::GetInteger("smooth fonts", gSmoothFonts);
	gShowInvisibles = Preferences::GetInteger("show invisibles", gShowInvisibles);
	gTabEntersSpaces = Preferences::GetInteger("tab enters spaces", gTabEntersSpaces);
	gFontSize = Preferences::GetInteger("fontsize", gFontSize);
	gFontName = Preferences::GetString("fontname", gFontName);
	
	gLanguageColors[kLTextColor] =			Preferences::GetColor("text color", kTextColor);
	gLanguageColors[kLKeyWordColor] =		Preferences::GetColor("keyword color", kKeyWordColor);
	gLanguageColors[kLPreProcessorColor] =	Preferences::GetColor("preprocessor color", kPreProcessorColor);
	gLanguageColors[kLCharConstColor] =		Preferences::GetColor("char const color", kCharConstColor);
	gLanguageColors[kLCommentColor] =		Preferences::GetColor("comment color", kCommentColor);
	gLanguageColors[kLStringColor] =		Preferences::GetColor("string color", kStringColor);
	gLanguageColors[kLTagColor] =			Preferences::GetColor("tag color", kTagColor);
	gLanguageColors[kLAttribColor] =		Preferences::GetColor("attribute color", kAttribColor);
	gLanguageColors[kLInvisiblesColor] =	Preferences::GetColor("invisibles color", kInvisiblesColor);

	gHiliteColor = Preferences::GetColor("hilite color", kHiliteColor);
	gInactiveHiliteColor = Preferences::GetColor("inactive hilite color", kInactiveHighlightColor);
//	gOddRowColor = kOddRowBackColor;
	gCurrentLineColor = Preferences::GetColor("current line color", kCurrentLineColor);
	gMarkedLineColor = Preferences::GetColor("marked line color", kMarkedLineColor);
	gPCLineColor = Preferences::GetColor("pc line color", kPCLineColor);
	gBreakpointColor = Preferences::GetColor("breakpoint color", kBreakpointColor);
	
}

void SaveGlobals()
{
	Preferences::SetInteger("auto indent", gAutoIndent);
	Preferences::SetInteger("smart indent", gSmartIndent);
	Preferences::SetInteger("kiss", gKiss);
	Preferences::SetInteger("chars per tab", gCharsPerTab);
	Preferences::SetInteger("spaces per tab", gSpacesPerTab);
//	Preferences::SetInteger("smooth fonts", gSmoothFonts);
	Preferences::SetInteger("show invisibles", gShowInvisibles);
	Preferences::SetInteger("fontsize", gFontSize);
	Preferences::SetString("fontname", gFontName);
	Preferences::SetInteger("tab enters spaces", gTabEntersSpaces);

	for (uint32 ix = 0; ix < kLStyleCount; ++ix)
	{
//		gLanguageColors[ix] = Preferences::GetColor("color_" + char('0' + ix), kLanguageColors[ix]);
	}
}
