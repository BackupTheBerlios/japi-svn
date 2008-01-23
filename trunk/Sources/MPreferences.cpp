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

/*	$Id: MPreferences.cpp 85 2006-09-14 08:23:20Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 01 2004 13:33:22
*/

#include "MJapi.h"

#include <sstream>
#include <cerrno>
#include <boost/bind.hpp>

#include "MTypes.h"
#include "MPreferences.h"
#include "MGlobals.h"

using namespace std;

namespace Preferences
{
	
class IniFile
{
  public:

	static IniFile&	Instance();
	
					operator GKeyFile*()		{ return mKeyFile; }	

  private:
					IniFile();
					~IniFile();
	
	GKeyFile*		mKeyFile;
	MPath			mIniFile;
};

IniFile::IniFile()
{
	mKeyFile = g_key_file_new();
	
	try
	{
		if (not fs::exists(gPrefsDir))
			fs::create_directories(gPrefsDir);
		
		mIniFile = gPrefsDir / "settings";

		GError *err = nil;
		GKeyFileFlags flags =
			GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);
		
		if (not g_key_file_load_from_file(mKeyFile, mIniFile.string().c_str(),
			flags, &err))
		{
			g_error_free(err);
		}
	}
	catch (exception& e)
	{
		cerr << "Exception reading preferences: " << e.what() << endl;		
	}
}

IniFile::~IniFile()
{
	try
	{
		GError* err = nil;
		gsize length;
		
		char* data = g_key_file_to_data(mKeyFile, &length, &err);
		
		if (data == nil)
		{
			cerr << "Error writing preferences data" << endl;

			if (err != nil)
			{
				cerr << err->message << endl;
				g_error_free(err);
			}
		}
		else
		{
			int fd = open(mIniFile.string().c_str(), O_CREAT|O_RDWR, 0600);
			if (fd >= 0)
			{
				write(fd, data, length);
				close(fd);
			}
			else
			{
				cerr << "Error writing preferences data to file: "
					 << strerror(errno) << endl;
			}
			
			g_free(data);
		}
	}
	catch (exception& e)
	{
		
	}
	catch (...) {}
}

IniFile& IniFile::Instance()
{
	static IniFile sInstance;
	return sInstance;
}

int32
GetInteger(
	const char*	inName,
	int32		inDefaultValue)
{
	GError* err = nil;
	
	int32 result = g_key_file_get_integer(IniFile::Instance(), "Settings", inName, &err);
	
	if (err != nil)
	{
		result = inDefaultValue;
		g_error_free(err);
		
		SetInteger(inName, inDefaultValue);
	}
	
	return result;
}

void
SetInteger(
	const char*	inName,
	int32		inValue)
{
	g_key_file_set_integer(IniFile::Instance(), "Settings", inName, inValue);
}

string GetString(
	const char*	inName,
	string		inDefaultValue)
{
	GError* err = nil;
	char* data = g_key_file_get_value(IniFile::Instance(), "Settings", inName, &err);
	
	string result;
	
	if (data == nil)
	{
		result = inDefaultValue;
		SetString(inName, inDefaultValue);
	}
	else
	{
		result = data;
		g_free(data);
	}
	
	if (err != nil)
		g_error_free(err);

	return result;
}

void SetString(
	const char*	inName,
	string		inValue)
{
	g_key_file_set_value(IniFile::Instance(), "Settings", inName, inValue.c_str());
}

void
GetArray(
	const char*		inName,
	vector<string>&	outArray)
{
	outArray.clear();
	
	GError* err = nil;
	gsize length;
	char** data = g_key_file_get_string_list(IniFile::Instance(), "Settings", inName, &length, &err);
	
	if (data != nil)
	{
		for (uint32 i = 0; i < length; ++i)
			outArray.push_back(data[i]);

		g_strfreev(data);
	}
	
	if (err != nil)
		g_error_free(err);
}

void
SetArray(
	const char*				inName,
	const vector<string>&	inArray)
{
	const char** data = new const char*[inArray.size()];
	
	transform(inArray.begin(), inArray.end(), data, boost::bind(&string::c_str, _1));

	g_key_file_set_string_list(IniFile::Instance(), "Settings", inName, data, inArray.size());
	
	delete[] data;
}

MColor GetColor(const char* inName, MColor inDefaultValue)
{
	inDefaultValue.hex(GetString(inName, inDefaultValue.hex()));
	return inDefaultValue;
}

void SetColor(const char* inName, MColor inValue)
{
	SetString(inName, inValue.hex());
}

MRect GetRect(const char* inName, MRect inDefault)
{
	stringstream s;
	s << inDefault.x << ' ' << inDefault.y << ' ' << inDefault.width << ' ' << inDefault.height;
	s.str(GetString(inName, s.str()));
	
	s >> inDefault.x >> inDefault.y >> inDefault.width >> inDefault.height;
	return inDefault;
}

void SetRect(const char* inName, MRect inValue)
{
	ostringstream s;
	s << inValue.x << ' ' << inValue.y << ' ' << inValue.width << ' ' << inValue.height;
	SetString(inName, s.str());
}

}
