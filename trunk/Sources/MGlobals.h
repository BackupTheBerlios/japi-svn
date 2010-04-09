//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MGlobals.h 133 2007-05-01 08:34:48Z maarten $
	Copyright Maarten L. Hekkelman
	Created Thursday July 22 2004 22:10:13
*/

#ifndef MYGLOBALS_H
#define MYGLOBALS_H

#include "MFile.h"
#include "MColor.h"
#include <string>

const uint32			kJapiSignature = 'Japi';

extern bool				gAutoIndent;
extern bool				gSmartIndent;
extern bool				gKiss;
extern bool				gSmoothFonts;
extern bool				gShowInvisibles;
extern bool				gTabEntersSpaces;
extern bool				gPlaySounds;
extern uint32			gCharsPerTab;
extern uint32			gSpacesPerTab;

extern uint32			gConcurrentJobs;

extern uint32			gFontSize;
extern std::string		gFontName;

extern fs::path			gTemplatesDir, gScriptsDir, gPrefsDir;

extern MColor			gLanguageColors[];
extern MColor			gHiliteColor, gInactiveHiliteColor;
extern MColor			gCurrentLineColor, gMarkedLineColor;
extern MColor			gPCLineColor, gBreakpointColor;
extern MColor			gWhiteSpaceColor;

void InitGlobals();
void SaveGlobals();

#endif // MYGLOBALS_H
