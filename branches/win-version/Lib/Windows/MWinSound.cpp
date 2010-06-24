//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include <map>
#include <iostream>

#include "MFile.h"
#include "MSound.h"
#include "MPreferences.h"
#include "MWindow.h"
#include "MError.h"

using namespace std;

void PlaySound(
	const string&		inSoundName)
{
	if (Preferences::GetBoolean("play sounds", true) == false)
		return;

	if (inSoundName == "success")
		::MessageBeep(MB_ICONINFORMATION);
	else if (inSoundName == "failure" or inSoundName == "error")
		::MessageBeep(MB_ICONERROR);
	else if (inSoundName == "warning")
		::MessageBeep(MB_ICONWARNING);
	else if (inSoundName == "question")
		::MessageBeep(MB_ICONQUESTION);
	else
		::MessageBeep(MB_ICONINFORMATION);
}

