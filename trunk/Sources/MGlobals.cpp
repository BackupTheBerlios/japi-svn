//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MGlobals.cpp 151 2007-05-21 15:59:05Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday July 25 2004 21:07:42
*/

#include "MJapi.h"

#include <boost/filesystem/fstream.hpp>
#include <glib.h>

#include <sys/stat.h>

#include "MGlobals.h"
#include "MPreferences.h"
#include "MLanguage.h"
#include "MResources.h"

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
	kBreakpointColor = MColor("#5ea50c"),
	kWhiteSpaceColor = MColor("#cf4c42");

const MColor
	kInactiveHighlightColor("#e5e5e5"),
	kOddRowBackColor("#eff7ff");

bool			gAutoIndent = true;
bool			gSmartIndent = true;
bool			gKiss = true;
bool			gSmoothFonts = false;
bool			gShowInvisibles = true;
bool			gTabEntersSpaces = false;
bool			gPlaySounds = true;
uint32			gCharsPerTab = 4;
uint32			gSpacesPerTab = 4;
uint32			gFontSize = 10;
std::string		gFontName = "Monaco";
MColor			gLanguageColors[kLStyleCount];
MColor			gHiliteColor, gInactiveHiliteColor;
MColor			gCurrentLineColor, gMarkedLineColor;
MColor			gPCLineColor, gBreakpointColor;
MColor			gWhiteSpaceColor;

uint32			gConcurrentJobs = 2;

fs::path		gTemplatesDir, gScriptsDir, gPrefsDir;

void InitGlobals()
{
	gPrefsDir = g_get_user_config_dir();
	gPrefsDir /= "japi";
	
	const char* templatesDir = g_get_user_special_dir(G_USER_DIRECTORY_TEMPLATES);
	if (templatesDir != nil)
		gTemplatesDir = fs::system_complete(templatesDir) / "japi";
	else
		gTemplatesDir = gPrefsDir / "Templates";

	gScriptsDir = gPrefsDir / "Scripts";
		
	gPlaySounds = Preferences::GetInteger("play sounds", gPlaySounds);
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
	gWhiteSpaceColor = Preferences::GetColor("whitespace color", kWhiteSpaceColor);
	
	if (not fs::exists(gTemplatesDir))
		fs::create_directory(gTemplatesDir);

	MResource rsrc = MResource::root().find("Templates");
	if (rsrc)
	{
		for (MResource::iterator t = rsrc.begin(); t != rsrc.end(); ++t)
		{
			if (t->size() == 0 or fs::exists(gTemplatesDir / t->name()))
				continue;
			
			fs::ofstream f(gTemplatesDir / t->name());
			f.write(t->data(), t->size());
		}
	}

	if (not fs::exists(gScriptsDir))
		fs::create_directory(gScriptsDir);

	rsrc = MResource::root().find("Scripts");
	if (rsrc)
	{
		for (MResource::iterator t = rsrc.begin(); t != rsrc.end(); ++t)
		{
			if (t->size() == 0 or fs::exists(gScriptsDir / t->name()))
				continue;
			
			fs::path file(gScriptsDir / t->name());
			
			fs::ofstream f(file);
			f.write(t->data(), t->size());
			
			chmod(file.string().c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
		}
	}
	
	gConcurrentJobs = Preferences::GetInteger("concurrent-jobs", gConcurrentJobs);
}

void SaveGlobals()
{
	Preferences::SetInteger("play sounds", gPlaySounds);
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
	
	Preferences::SetInteger("concurrent-jobs", gConcurrentJobs);
}
