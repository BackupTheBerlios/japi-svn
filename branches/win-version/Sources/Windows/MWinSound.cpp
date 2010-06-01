//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

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
	if (not gPlaySounds)
		return;
	
	//try
	//{
	//	StOKToThrow ok;
	//	string filename;
	//	
	//	if (inSoundName == "success")
	//		filename = Preferences::GetString("success sound", "info.wav");
	//	else if (inSoundName == "failure" or inSoundName == "error")
	//		filename = Preferences::GetString("failure sound", "error.wav");
	//	else if (inSoundName == "warning")
	//		filename = Preferences::GetString("warning sound", "warning.wav");
	//	else if (inSoundName == "question")
	//		filename = Preferences::GetString("question sound", "question.wav");
	//	else
	//	{
	//		filename = "warning.wav";
	//		cerr << "Unknown sound name " << inSoundName << endl;
	//	}

	//	fs::path path = filename;

	//	const char* const* config_dirs = g_get_system_data_dirs();
	//	for (const char* const* dir = config_dirs; *dir != nil; ++dir)
	//	{
	//		path = fs::path(*dir) / "sounds" / filename;
	//		if (fs::exists(path))
	//			break;
	//	}
	//	
	//	if (fs::exists(path))
	//		MAudioSocket::Instance().Play(path.string());
	//	else
	//	{
	//		cerr << "Sound does not exist: " << path.string() << endl;
	//		if (MWindow::GetFirstWindow() != nil)
	//			MWindow::GetFirstWindow()->Beep();
	//		else
	//			gdk_beep();
	//	}
	//}
	//catch (...)
	//{
	//	gdk_beep();
	//}
}

