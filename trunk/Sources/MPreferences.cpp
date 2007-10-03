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

#include "MJapieG.h"

#include "MTypes.h"
#include "MPreferences.h"
#include <sstream>

using namespace std;

namespace Preferences
{

int32
GetInteger(
	const char*	inName,
	int32		inDefaultValue)
{
//	MCFString key(inName);
//	Boolean valid;
//	
//	SInt32 result = ::CFPreferencesGetAppIntegerValue(key,
//		kCFPreferencesCurrentApplication, &valid);
//
//	if (not valid)
//		result = inDefaultValue;
//	
//	return result;
}

void
SetInteger(
	const char*	inName,
	int32		inValue)
{
//	MCFString key(inName);
//	CFNumberRef	value = ::CFNumberCreate(nil, kCFNumberIntType, &inValue);
//	
//	::CFPreferencesSetAppValue(key, value, kCFPreferencesCurrentApplication);
//
//	::CFRelease(value);
//
//	::CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}

string
GetString(
	const char*	inName,
	string		inDefaultValue)
{
//	string result = inDefaultValue;
//	
//	MCFString key(inName);
//	MCFString value(static_cast<CFStringRef>(::CFPreferencesCopyAppValue(key,
//		kCFPreferencesCurrentApplication)), false);
//
//	if (value.IsValid() != nil)
//		value.GetString(result);
//	
//	return result;
}

void SetString(
	const char*	inName,
	string		inValue)
{
//	MCFString key(inName);
//	MCFString value(inValue);
//	
//	::CFPreferencesSetAppValue(key, value, kCFPreferencesCurrentApplication);
//
//	::CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}

void
GetArray(
	const char*		inName,
	vector<string>&	outArray)
{
//	outArray.clear();
//	
//	MCFString key(inName);
//	CFArrayRef value = reinterpret_cast<CFArrayRef>(
//		::CFPreferencesCopyAppValue(key,
//			kCFPreferencesCurrentApplication));
//
//	if (value != nil)
//	{
//		for (int32 index = 0; index < ::CFArrayGetCount(value); ++index)
//		{
//			MCFString str(
//				static_cast<CFStringRef>(::CFArrayGetValueAtIndex(value, index)), true);
//			string s;
//			str.GetString(s);
//			outArray.push_back(s);
//		}
//
//		::CFRelease(value);
//	}
}

void
SetArray(
	const char*				inName,
	const vector<string>&	inArray)
{
//	auto_array<MCFString> ss(new MCFString[inArray.size()]);
//	MCFString* aa = ss.get();
//
//	auto_array<CFStringRef> s(new CFStringRef[inArray.size()]);
//	CFStringRef* a = s.get();
//	
//	for (UInt32 i = 0; i < inArray.size(); ++i)
//	{
//		aa[i] = MCFString(inArray[i]);
//		a[i] = aa[i];
//	}
//	
//	CFArrayRef value = ::CFArrayCreate(
//		kCFAllocatorDefault, (const void **)a, inArray.size(),
//			&kCFTypeArrayCallBacks);
//	
//	if (value != nil)
//	{
//		MCFString key(inName);
//	
//		::CFPreferencesSetAppValue(key, value,
//			kCFPreferencesCurrentApplication);
//		
//		::CFRelease(value);
//	}
//
//	::CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
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
