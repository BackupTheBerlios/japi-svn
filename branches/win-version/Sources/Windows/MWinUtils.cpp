//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <windows.h>

#include <cstdarg>

#include "MLib.h"

#include "MUtils.h"
#include "MWinUtils.h"
#include "MError.h"

using namespace std;

char* __S_FILE = "";
int __S_LINE;

double GetLocalTime()
{
	static double sDiff = -1.0;

	FILETIME tm;
	ULARGE_INTEGER li;
	
	if (sDiff == -1.0)
	{
		SYSTEMTIME st = { 0 };

		st.wDay = 1;
		st.wMonth = 1;
		st.wYear = 1970;

		if (::SystemTimeToFileTime(&st, &tm))
		{
			li.LowPart = tm.dwLowDateTime;
			li.HighPart = tm.dwHighDateTime;
		
			// Prevent Ping Pong comment. VC cannot convert UNSIGNED int64 to double. SIGNED is ok. (No more long)
			sDiff = static_cast<int64>(li.QuadPart);
			sDiff /= 1e7;
		}
	}	
	
	::GetSystemTimeAsFileTime(&tm);
	
	li.LowPart = tm.dwLowDateTime;
	li.HighPart = tm.dwHighDateTime;
	
	double result = static_cast<__int64> (li.QuadPart);
	result /= 1e7;
	return result - sDiff;
}

wstring c2w(const string& s)
{
	wstring result;
	result.reserve(s.length());

	for (string::const_iterator i = s.begin(); i != s.end(); ++i)
	{
		uint32 ch = static_cast<unsigned char>(*i);

		if (ch & 0x0080)
		{
			if ((ch & 0x0E0) == 0x0C0)
			{
				uint32 ch1 = static_cast<unsigned char>(*(i + 1));
				if ((ch1 & 0x0c0) == 0x080)
				{
					ch = ((ch & 0x01f) << 6) | (ch1 & 0x03f);
					i += 1;
				}
			}
			else if ((ch & 0x0F0) == 0x0E0)
			{
				uint32 ch1 = static_cast<unsigned char>(*(i + 1));
				uint32 ch2 = static_cast<unsigned char>(*(i + 2));
				if ((ch1 & 0x0c0) == 0x080 and (ch2 & 0x0c0) == 0x080)
				{
					ch = ((ch & 0x00F) << 12) | ((ch1 & 0x03F) << 6) | (ch2 & 0x03F);
					i += 2;
				}
			}
			else if ((ch & 0x0F8) == 0x0F0)
			{
				uint32 ch1 = static_cast<unsigned char>(*(i + 1));
				uint32 ch2 = static_cast<unsigned char>(*(i + 2));
				uint32 ch3 = static_cast<unsigned char>(*(i + 3));
				if ((ch1 & 0x0c0) == 0x080 and (ch2 & 0x0c0) == 0x080 and (ch3 & 0x0c0) == 0x080)
				{
					ch = ((ch & 0x007) << 18) | ((ch1 & 0x03F) << 12) | ((ch2 & 0x03F) << 6) | (ch3 & 0x03F);
					i += 3;
				}
			}
		}

		if (ch <= 0x0FFFF)
			result += static_cast<wchar_t>(ch);
		else
		{
			wchar_t h = (ch - 0x010000) / 0x0400 + 0x0D800;
			wchar_t l = (ch - 0x010000) % 0x0400 + 0x0DC00;

			result += h;
			result += l;
		}
	}

	return result;
}

string w2c(const wstring& s)
{
	string result;
	result.reserve(s.length());

	for (wstring::const_iterator i = s.begin(); i != s.end(); ++i)
	{
		uint32 uc = static_cast<uint16>(*i);

		if (uc >= 0x0D800 and uc <= 0x0DBFF)
		{
			wchar_t ch = static_cast<uint16>(*(i + 1));
			if (ch >= 0x0DC00 and ch <= 0x0DFFF)
			{
				uc = (uc << 16) | ch;
				++i;
			}
		}

		if (uc < 0x080)
			result += (static_cast<char>(uc));
		else if (uc < 0x0800)
		{
			char ch[2] = {
				static_cast<char>(0x0c0 | (uc >> 6)),
				static_cast<char>(0x080 | (uc & 0x3f))
			};
			result.append(ch, 2);
		}
		else if (uc < 0x00010000)
		{
			char ch[3] = {
				static_cast<char>(0x0e0 | (uc >> 12)),
				static_cast<char>(0x080 | ((uc >> 6) & 0x3f)),
				static_cast<char>(0x080 | (uc & 0x3f))
			};
			result.append(ch, 3);
		}
		else
		{
			char ch[4] = {
				static_cast<char>(0x0f0 | (uc >> 18)),
				static_cast<char>(0x080 | ((uc >> 12) & 0x3f)),
				static_cast<char>(0x080 | ((uc >> 6) & 0x3f)),
				static_cast<char>(0x080 | (uc & 0x3f))
			};
			result.append(ch, 4);
		}
	}

	return result;
}

void __debug_printf(const char* inMessage, ...)
{
	char msg[1024] = {0};
	
	va_list vl;
	va_start(vl, inMessage);
	int n = vsnprintf(msg, sizeof(msg), inMessage, vl);
	va_end(vl);

	if (n < sizeof(msg) - 1 and msg[n] != '\n')
		msg[n] = '\n';

	_CrtDbgReport (_CRT_WARN, __S_FILE, __S_LINE, NULL, msg);
}

void __signal_throw(
	const char*		inCode,
	const char*		inFunction,
	const char*		inFile,
	int				inLine)
{
	cerr << "Throwing in file " << inFile << " line " << inLine
		<< " \"" << inFunction << "\": " << endl << inCode << endl;
	
	if (StOKToThrow::IsOK())
		return;

	//GtkWidget* dlg = gtk_message_dialog_new(nil, GTK_DIALOG_MODAL,
	//	GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
	//	"Exception thrown in file '%s', line %d, function: '%s'\n\n"
	//	"code: %s", inFile, inLine, inFunction, inCode);
	//
	//PlaySound("error");
	//(void)gtk_dialog_run(GTK_DIALOG(dlg));
	//
	//gtk_widget_destroy(dlg);
}

MWinException::MWinException(
	const char*			inMsg,
	...)
{
	DWORD error = ::GetLastError();

	va_list vl;
	va_start(vl, inMsg);
	int n = vsnprintf(mMessage, sizeof(mMessage), inMsg, vl);
	va_end(vl);

	if (n < sizeof(mMessage))
		mMessage[n++] = ' ';

	::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nil, error, 0, mMessage, sizeof(mMessage) - n, nil);
}

void DisplayError(const exception& e)
{
	assert(false);
}

string GetHome()
{
	const char* HOME = getenv("HOME");
	if (HOME == nil)
		HOME = "";
	return HOME;
}