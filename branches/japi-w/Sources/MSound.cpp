//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <map>
#include <dlfcn.h>
#include <iostream>
#include <gst/gst.h>

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
	
	GstElement*		mPlayer;
};

MAudioSocket::MAudioSocket()
{
	gst_init(nil, nil);
	
	mPlayer = gst_element_factory_make("playbin", "play");
	if (mPlayer == nil)
		THROW(("Failed to create player"));
	
	// Instead of using the default audiosink, use the gconfaudiosink, which
	// will respect the defaults in gstreamer-properties

	GstElement* sink = gst_element_factory_make("gconfaudiosink", "GconfAudioSink");
	if (sink == nil)
		THROW(("Failed to create sink"));
	
	g_object_set(G_OBJECT(mPlayer), "audio-sink", sink, nil);
}

MAudioSocket::~MAudioSocket()
{
	g_object_unref(mPlayer);
}

MAudioSocket& MAudioSocket::Instance()
{
	static MAudioSocket sInstance;
	return sInstance;
}

void MAudioSocket::Play(
	const string&	inFile)
{
	if (mPlayer != nil)
	{
		string uri = "file://";
		uri += inFile;
		
		// stop old sound
		gst_element_set_state(mPlayer, GST_STATE_NULL);

		// Set the input to a local file
		g_object_set(G_OBJECT(mPlayer), "uri", uri.c_str(), nil);
	
		// Start the pipeline again
		gst_element_set_state(mPlayer, GST_STATE_PLAYING);
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
