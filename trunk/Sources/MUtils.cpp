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

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <boost/date_time/gregorian/gregorian.hpp>

#include <string>
#include <sstream>
#include <string>
#include <stack>
#include <pwd.h>

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

string GetUserName(bool inShortName)
{
	string result;
	
	int uid = getuid();
	struct passwd* pw = getpwuid(uid);

	if (pw != nil)
	{
		if (inShortName or *pw->pw_gecos == 0)
			result = pw->pw_name;
		else
		{
			result = pw->pw_gecos;
			
			if (result.length() > 0)
			{
				string::size_type p = result.find(',');

				if (p != string::npos)
					result.erase(p, result.length() - p);

				p = result.find('&');

				if (p != string::npos)
					result.replace(p, 1, pw->pw_name);
			}
		}
	}

	return result;
}

string GetDateTime()
{
	using namespace boost::gregorian;

	date today = day_clock::local_day();

	date::ymd_type ymd = today.year_month_day();
	greg_weekday wd = today.day_of_week();
	
	stringstream s;
	
	s << wd.as_long_string() << " "
      << ymd.month.as_long_string() << " "
	  << ymd.day << ", " << ymd.year;
	
	return s.str();
}

uint32 AddNameToNameTable(
	string&			ioNameTable,
	const string&	inName)
{
	uint32 result = 0;
	
	const char* p = ioNameTable.c_str();
	const char* e = p + ioNameTable.length();
	
	while (p < e)
	{
		if (inName == p)
		{
			result = p - ioNameTable.c_str();
			break;
		}

		p += strlen(p) + 1;
	}
	
	if (p >= e)
	{
		result = ioNameTable.length();
		ioNameTable.append(inName);
		ioNameTable.append("\0", 1);
	}
	
	return result;
}

string XMLNode::name() const
{
	return string((const char*)mNode->name);
}

string XMLNode::text() const
{
	string result;
	
	if (mNode->children != nil)
	{
		const char* t = (const char*)XML_GET_CONTENT(mNode->children);
		if (t != nil)
			result = t;
	}
	
	return result;
}

string XMLNode::property(
	const char*	inName) const
{
	const char* p = (const char*)xmlGetProp(mNode, BAD_CAST inName);
	string result;
	if (p != nil)
		result = p;
	return result;
}


void XMLNode::iterator::increment()
{
	mNode = mNode->next;
}

bool XMLNode::iterator::equal(const iterator& inOther) const
{
	return mNode == inOther.mNode;
}

XMLNode::iterator::reference  XMLNode::iterator::dereference() const
{
	mXMLNode->mNode = mNode;
	return *mXMLNode;
}

XMLNode::iterator XMLNode::begin() const
{
	return iterator(mNode->children);
}

XMLNode::iterator XMLNode::end() const
{
	return iterator(nil);
}

bool IsModifierDown(
	int		inModifierMask)
{
	bool result = false;
	
	GdkModifierType state;

	if (gtk_get_current_event_state(&state))
		result = (state & inModifierMask) != 0;
	
	return result;
}

//bool ModifierKeyDown(int inModifiers)
//{
//	bool result = false;
//
//	 XModifierKeymap* km = ::XGetModifierMapping(gDisplay);
//	 if (km == nil)
//	 	return false;
//	
//	 char keyMap[32];
//	 ::XQueryKeymap(gDisplay, keyMap);
//	 
//	 int key = 0;
//
//	 if (inModifiers & GDK_SHIFT_MASK)
//	 	key = ShiftMask;
//	 else if (inModifiers & GDK_C)
//	 	key = gPrefs->GetPrefInt("cmd-mod", Mod1Mask);
//	 else if (inModifiers & kOptionKey)
//	 	key = gPrefs->GetPrefInt("opt-mod", Mod4Mask);
//	 else if (inModifiers & kControlKey)
//	 	key = gPrefs->GetPrefInt("cntrl-mod", ControlMask);
//	
//	if (key != 0)
//	{
//		int ix = 0;
//		
//		switch (key)
//		{
//			case ShiftMask:		ix = ShiftMapIndex;		break;
//			case LockMask:		ix = LockMapIndex;		break;
//			case ControlMask:	ix = ControlMapIndex;	break;
//			case Mod1Mask:		ix = Mod1MapIndex;		break;
//			case Mod2Mask:		ix = Mod2MapIndex;		break;
//			case Mod3Mask:		ix = Mod3MapIndex;		break;
//			case Mod4Mask:		ix = Mod4MapIndex;		break;
//			case Mod5Mask:		ix = Mod5MapIndex;		break;
//		}
//		
//		if (ix != 0)
//		{
//			for (int i = 0; result == false && i < km->max_keypermod; ++i)
//			{
//				int code = km->modifiermap[ix * km->max_keypermod + i];
//				if (code <= 0 || code > 255)
//					continue;
//				result = (keyMap[code >> 3] & (1 << (code & 0x07))) != 0;
//			}
//		}
//	}
//	
//	::XFreeModifiermap(km);
//	
//	return result;
//}
