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

#include <string>
#include <sstream>
#include <string>
#include <stack>

#include <sys/time.h>

#include "MUtils.h"

using namespace std;

uint16 CalculateCRC(const void* inData, uint32 inLength, uint16 inCRC)
{
	const uint8* p = reinterpret_cast<const uint8*>(inData);

	while (inLength-- > 0)
	{
		inCRC = static_cast<uint16>(inCRC ^ (static_cast<uint16>(*p++) << 8));
		for (uint16 i = 0; i < 8; i++)
		{
			if (inCRC & 0x8000)
				inCRC = static_cast<uint16>((inCRC << 1) ^ 0x1021);
			else
				inCRC = static_cast<uint16>(inCRC << 1);
		}
	}

	return inCRC;
}

string Escape(string inString)
{
	string result;
	
	for (string::iterator c = inString.begin(); c != inString.end(); ++c)
	{
		if (*c == '\n')
		{
			result += '\\';
			result += 'n';
		}
		else if (*c == '\t')
		{
			result += '\\';
			result += 't';
		}
		else if (*c == '\\')
		{
			result += '\\';
			result += '\\';
		}
		else
			result += *c;
	}
	
	return result;
}

string Unescape(string inString)
{
	string result;
	
	for (string::iterator c = inString.begin(); c != inString.end(); ++c)
	{
		if (*c == '\\')
		{
			++c;
			if (c != inString.end())
			{
				switch (*c)
				{
					case 'n':
						result += '\n';
						break;
					
					case 't':
						result += '\t';
						break;
					
					default:
						result += *c;
						break;
				}
			}
			else
				result += '\\';
		}
		else
			result += *c;
	}
	
	return result;
}

string NumToString(uint32 inNumber)
{
	stringstream s;
	s << inNumber;
	return s.str();
}

uint32 StringToNum(string inString)
{
	stringstream s(inString);
	uint32 n;
	s >> n;
	return n;
}

void NormalizePath(string& ioPath)
{
	string path(ioPath);	
	stack<unsigned long> dirs;
	int r = 0;
	unsigned long i = 0;
	
	dirs.push(0);
	
	while (i < path.length())
	{
		while (i < path.length() && path[i] == '/')
		{
			++i;
			if (dirs.size() > 0)
				dirs.top() = i;
			else
				dirs.push(i);
		}
		
		if (path[i] == '.' && path[i + 1] == '.' && path[i + 2] == '/')
		{
			if (dirs.size() > 0)
				dirs.pop();
			if (dirs.size() == 0)
				--r;
			i += 2;
			continue;
		}
		else if (path[i] == '.' && path[i + 1] == '/')
		{
			i += 1;
			if (dirs.size() > 0)
				dirs.top() = i;
			else
				dirs.push(i);
			continue;
		}

		unsigned long d = path.find('/', i + 1);

		if (d == string::npos)
			break;
		
		i = d + 1;
		dirs.push(i);
	}
	
	if (dirs.size() > 0 && dirs.top() == path.length())
		ioPath.assign("/");
	else
		ioPath.erase(ioPath.begin(), ioPath.end());
	
	bool dir = false;
	while (dirs.size() > 0)
	{
		unsigned long l, n;
		n = path.find('/', dirs.top());
		if (n == string::npos)
			l = path.length() - dirs.top();
		else
			l = n - dirs.top();
		
		if (l > 0)
		{
			if (dir)
				ioPath.insert(0, "/");
			ioPath.insert(0, path.c_str() + dirs.top(), l);
			dir = true;
		}
		
		dirs.pop();
	}
	
	if (r < 0)
	{
		ioPath.insert(0, "../");
		while (++r < 0)
			ioPath.insert(0, "../");
	}
	else if (path.length() > 0 && path[0] == '/' && ioPath[0] != '/')
		ioPath.insert(0, "/");
}

double GetLocalTime()
{
	struct timeval tv;
	
	gettimeofday(&tv, nil);
	
	return tv.tv_sec + tv.tv_usec / 1e6;
}

double GetDoubleClickTime()
{
//	return ::GetDblTime() / 60.0;
	return 0.2;
}

string GetLocalisedString(const char* inString)
{
	return inString;
//	MCFString key(inString);
//	MCFString s(::CFCopyLocalizedString(key, "auto"), false);
//	
//	string r;
//	s.GetString(r);
//	return r;
}

string GetUserName(bool inShortName)
{
	return "ikke";
//	MCFString s(::CSCopyUserName(inShortName), false);
//	
//	string name;
//	s.GetString(name);
//	return name;
}

string GetDateTime()
{
//	MCFRef<CFDateRef> date(::CFDateCreate(NULL, ::CFAbsoluteTimeGetCurrent()), false);
//	MCFRef<CFLocaleRef> locale(::CFLocaleCopyCurrent(), false);
//	MCFRef<CFDateFormatterRef> dateFormatter(::CFDateFormatterCreate
//        (NULL, locale, kCFDateFormatterShortStyle, kCFDateFormatterLongStyle), false);
//	MCFString formattedString(::CFDateFormatterCreateStringWithDate
//        (NULL, dateFormatter, date), false);
//
//	string s;
//	formattedString.GetString(s);
//	return s;
	return "de tijd";
}

void Beep()
{
	
}
