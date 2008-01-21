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

#include <string>
#include <sstream>
#include <string>
#include <stack>
#include <pwd.h>
#include <cmath>

#include <sys/time.h>

#include "MUtils.h"
#include "MError.h"

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

void NormalizePath(
	MPath&	ioPath)
{
	string p = ioPath.string();
	NormalizePath(p);
	ioPath = p;
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
	// had to remove this, because it depends on wchar_t in libstdc++...
//	using namespace boost::gregorian;
//
//	date today = day_clock::local_day();
//
//	date::ymd_type ymd = today.year_month_day();
//	greg_weekday wd = today.day_of_week();
//	
//	stringstream s;
//	
//	s << wd.as_long_string() << " "
//      << ymd.month.as_long_string() << " "
//	  << ymd.day << ", " << ymd.year;
//	
//	return s.str();

	string result;

	GDate* date = g_date_new();
	if (date != nil)
	{
		g_date_set_time_t(date, time(nil)); 
	
		char buffer[1024] = "";
		uint32 size = g_date_strftime(buffer, sizeof(buffer),
			"%A %d %B, %Y", date);
		
		result.assign(buffer, buffer + size);
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
	char* p = (char*)xmlGetProp(mNode, BAD_CAST inName);
	string result;
	if (p != nil)
	{
		result = p;
		xmlFree(p);
	}
	return result;
}

xmlNodePtr XMLNode::children() const
{
	return mNode->children;
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

void HexDump(
	const void*		inBuffer,
	uint32			inLength,
	ostream&		outStream)
{
	const char kHex[] = "0123456789abcdef";
	char s[] = "xxxxxxxx  cccc cccc cccc cccc  cccc cccc cccc cccc  |................|";
	const int kHexOffset[] = { 10, 12, 15, 17, 20, 22, 25, 27, 31, 33, 36, 38, 41, 43, 46, 48 };
	const int kAsciiOffset = 53;
	
	const unsigned char* data = reinterpret_cast<const unsigned char*>(inBuffer);
	uint32 offset = 0;
	
	while (offset < inLength)
	{
		int rr = inLength - offset;
		if (rr > 16)
			rr = 16;
		
		char* t = s + 7;
		long o = offset;
		
		while (t >= s)
		{
			*t-- = kHex[o % 16];
			o /= 16;
		}
		
		for (int i = 0; i < rr; ++i)
		{
			s[kHexOffset[i] + 0] = kHex[data[i] >> 4];
			s[kHexOffset[i] + 1] = kHex[data[i] & 0x0f];
			if (data[i] < 128 and isprint(data[i]))
				s[kAsciiOffset + i] = data[i];
			else
				s[kAsciiOffset + i] = '.';
		}
		
		for (int i = rr; i < 16; ++i)
		{
			s[kHexOffset[i] + 0] = ' ';
			s[kHexOffset[i] + 1] = ' ';
			s[kAsciiOffset + i] = ' ';
		}
		
		outStream << s << endl;
		
		offset += rr;
		data += rr;
	}
}

// --------------------------------------------------------------------
// code to create a GdkPixbuf containing a single dot.
//
// use cairo to create an alpha mask, then set the colour
// into the pixbuf.

GdkPixbuf* CreateDot(
	MColor			inColor,
	uint32			inSize)
{
	// first draw in a buffer with cairo
	cairo_surface_t* cs = cairo_image_surface_create(
		CAIRO_FORMAT_ARGB32, inSize, inSize);
	
	cairo_t* c = cairo_create(cs);

	cairo_translate(c, inSize / 2., inSize / 2.);
	cairo_scale(c, inSize / 2., inSize / 2.);
	cairo_arc(c, 0., 0., 1., 0., 2 * M_PI);
	cairo_fill(c);

	cairo_destroy(c);
	
	// then copy the data over to a pixbuf;

	GdkPixbuf* result = gdk_pixbuf_new(
		GDK_COLORSPACE_RGB, true, 8, inSize, inSize);
	THROW_IF_NIL(result);
	
	unsigned char* dst = gdk_pixbuf_get_pixels(result);
	unsigned char* src = cairo_image_surface_get_data(cs);
	
	uint32 dst_rowstride = gdk_pixbuf_get_rowstride(result);
	uint32 src_rowstride = cairo_image_surface_get_stride(cs);
	uint32 n_channels = gdk_pixbuf_get_n_channels(result);

	for (uint32 x = 0; x < inSize; ++x)
	{
		for (uint32 y = 0; y < inSize; ++y)
		{
			unsigned char* p = dst + y * dst_rowstride + x * n_channels;
			uint32 cp = *reinterpret_cast<uint32*>(src + y * src_rowstride + x * 4);

			p[0] = inColor.red;
			p[1] = inColor.green;
			p[2] = inColor.blue;

			p[3] = (cp >> 24) & 0xFF;
		}
	}
	
	cairo_surface_destroy(cs);

	return result;
}

