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

#include "MJapieG.h"

#include <map>
#include <dlfcn.h>
#include <iostream>

#include "MFile.h"
#include "MSound.h"
#include "MPreferences.h"

#if defined(__APPLE__) and defined(__MACH__)
#define SO_EXT ".dylib"
#else
#define SO_EXT ".so"
#endif

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

	typedef int		(*esd_play_file)(
						const char*		name_prefix,
						const char*		filename,
						int				fallback);

					MAudioSocket();
					~MAudioSocket();
	
	void*			mHandle;
	esd_play_file	mFunc;
};

MAudioSocket::MAudioSocket()
{
	mHandle = dlopen("libesd" SO_EXT, RTLD_LAZY);
	
	if (mHandle == nil)
		cerr << "Failed to locate esd library, sounds are disabled" << endl;
	else
	{
		dlerror();
	
		mFunc = reinterpret_cast<esd_play_file>(dlsym(mHandle, "esd_play_file"));
		
		char* err;
		if ((err = dlerror()) != NULL)
		{
			cerr << "Could not find esd_play_file: " << err << endl;
			dlclose(mHandle);
			mHandle = nil;
		}
	}
}

MAudioSocket::~MAudioSocket()
{
	dlclose(mHandle);
}

MAudioSocket& MAudioSocket::Instance()
{
	static MAudioSocket sInstance;
	return sInstance;
}

void MAudioSocket::Play(
	const string&	inFile)
{
	if (mFunc != nil)
		(*mFunc)("Japie", inFile.c_str(), 1);
	else
		gdk_beep();
}

}

void PlaySound(
	const string&		inSoundName)
{
	static const MPath
		kSystemSoundDirectory("/System/Library/Sounds");

	try
	{
		MPath path = kSystemSoundDirectory / (inSoundName + ".aiff");
		if (fs::exists(path))
			MAudioSocket::Instance().Play(path.string());
		else
		{
			cerr << "Sound does not exist: " << path.string() << endl;
			gdk_beep();
		}
	}
	catch (...) {}
}
