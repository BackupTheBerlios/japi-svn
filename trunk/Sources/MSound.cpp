//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <map>
#include <dlfcn.h>
#include <iostream>
#include <canberra.h>

#include "MFile.h"
#include "MSound.h"
#include "MPreferences.h"
#include "MGlobals.h"
#include "MWindow.h"
#include "MError.h"

using namespace std;

namespace {

class MAudioSocket
{
  public:
	static MAudioSocket&
					Instance();
	
	void			Play(
						const string&	inPath);

  private:

					MAudioSocket();
					~MAudioSocket();

	static void		CAFinishCallback(
						ca_context*		inContext,
						uint32_t		inID,
						int				inErrorCode,
						void*			inUserData);
	
	ca_context*		mCAContext;
};

MAudioSocket::MAudioSocket()
	: mCAContext(nil)
{
	int err = ca_context_create(&mCAContext);
	if (err == 0)
		err = ca_context_open(mCAContext);
	
}

MAudioSocket::~MAudioSocket()
{
	ca_context_destroy(mCAContext);
	mCAContext = nil;
}

MAudioSocket& MAudioSocket::Instance()
{
	static MAudioSocket sInstance;
	return sInstance;
}

void MAudioSocket::CAFinishCallback(
	ca_context*		inContext,
	uint32_t		inID,
	int				inErrorCode,
	void*			inUserData)
{
	if (inErrorCode != CA_SUCCESS)
		cerr << "Error playing sound using canberra: " << ca_strerror(inErrorCode) << endl;
}

void MAudioSocket::Play(
	const string&	inFile)
{
	if (mCAContext != nil)
	{
		ca_proplist* pl;
		
		ca_proplist_create(&pl);
		ca_proplist_sets(pl, CA_PROP_MEDIA_FILENAME, inFile.c_str());
		
		int pan = 2;
//        gc = gconf_client_get_default();
//        value = gconf_client_get(gc, ALARM_GCONF_PATH, NULL);
//
//        if (value && value->type == GCONF_VALUE_INT)
//                pan = gconf_value_get_int(value);
//        else
//                pan = 2;
		float volume = (1.0f - float(pan) / 2.0f) * -6.0f;
		ca_proplist_setf(pl, CA_PROP_CANBERRA_VOLUME, "%f", volume);

		int err = ca_context_play_full(mCAContext, 0, pl, &MAudioSocket::CAFinishCallback, this);
		if (err != CA_SUCCESS)
			cerr << "Error calling ca_context_play_full: " << ca_strerror(err) << endl;

		ca_proplist_destroy(pl);
	}
	else if (MWindow::GetFirstWindow() != nil)
		MWindow::GetFirstWindow()->Beep();
	else
		gdk_beep();
}

}

void PlaySound(
	const string&		inSoundName)
{
	if (not gPlaySounds)
		return;
	
	try
	{
		StOKToThrow ok;
		string filename;
		
		if (inSoundName == "success")
			filename = Preferences::GetString("success sound", "info.wav");
		else if (inSoundName == "failure" or inSoundName == "error")
			filename = Preferences::GetString("failure sound", "error.wav");
		else if (inSoundName == "warning")
			filename = Preferences::GetString("warning sound", "warning.wav");
		else if (inSoundName == "question")
			filename = Preferences::GetString("question sound", "question.wav");
		else
		{
			filename = "warning.wav";
			cerr << "Unknown sound name " << inSoundName << endl;
		}

		fs::path path = filename;

		const char* const* config_dirs = g_get_system_data_dirs();
		for (const char* const* dir = config_dirs; *dir != nil; ++dir)
		{
			path = fs::path(*dir) / "sounds" / filename;
			if (fs::exists(path))
				break;
		}
		
		if (fs::exists(path))
			MAudioSocket::Instance().Play(path.string());
		else
		{
			cerr << "Sound does not exist: " << path.string() << endl;
			if (MWindow::GetFirstWindow() != nil)
				MWindow::GetFirstWindow()->Beep();
			else
				gdk_beep();
		}
	}
	catch (...)
	{
		gdk_beep();
	}
}

