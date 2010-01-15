//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <string>
#include <sstream>
#include <string>
#include <stack>
#include <pwd.h>
#include <cmath>

#include <sys/time.h>

#include "MError.h"
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

void decode_base64(
	const string&		inString,
	vector<uint8>&		outBinary)
{
    const char kLookupTable[] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
    };
    
    string::const_iterator b = inString.begin();
    string::const_iterator e = inString.end();
    
	while (b != e)
	{
		uint8 s[4] = {};
		int n = 0;
		
		for (int i = 0; i < 4 and b != e; ++i)
		{
			uint8 ix = uint8(*b++);

			if (ix == '=')
				break;
			
			char v = -1;
			if (ix <= 127) 
				v = kLookupTable[ix];
			if (v < 0)	THROW(("Invalid character in base64 encoded string"));
			s[i] = uint8(v);
			++n;
		}

		if (n > 1)	outBinary.push_back(s[0] << 2 | s[1] >> 4);
		if (n > 2)	outBinary.push_back(s[1] << 4 | s[2] >> 2);
		if (n > 3)	outBinary.push_back(s[2] << 6 | s[3]);
	}
}

