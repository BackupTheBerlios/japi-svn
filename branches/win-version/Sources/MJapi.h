//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MJAPI_H
#define MJAPI_H

#include <boost/filesystem/path.hpp>

#include "MLib.h"

#include "MTypes.h"
#include "MColor.h"

extern const char kAppName[], kVersionString[];

// ===========================================================================
//	Japi specific globals

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

extern boost::filesystem::path
						gTemplatesDir, gScriptsDir;

extern MColor			gLanguageColors[];
extern MColor			gHiliteColor, gInactiveHiliteColor;
extern MColor			gCurrentLineColor, gMarkedLineColor;
extern MColor			gPCLineColor, gBreakpointColor;
extern MColor			gWhiteSpaceColor;

#endif
