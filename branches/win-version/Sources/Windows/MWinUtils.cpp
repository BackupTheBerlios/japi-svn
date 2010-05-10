//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <windows.h>

#include "MLib.h"

#include "MUtils.h"

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
			sDiff = static_cast<__int64> (li.QuadPart);
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

