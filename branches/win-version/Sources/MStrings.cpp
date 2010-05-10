//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <libintl.h>

#include "MStrings.h"

using namespace std;

const char* GetLocalisedString(
				const char* inString)
{
	const char* result = dgettext("japi", inString);

	if (result == nil)
		result = inString;

	return result;
}

string GetLocalisedString(
	const string&	inString)
{
	return GetLocalisedString(inString.c_str());
}

string GetFormattedLocalisedStringWithArguments(
	const string&			inString,
	const vector<string>&	inArgs)
{
	string result = GetLocalisedString(inString.c_str());
	
	char s[] = "^0";
	
	for (vector<string>::const_iterator a = inArgs.begin(); a != inArgs.end(); ++a)
	{
		string::size_type p = result.find(s);
		if (p != string::npos)
			result.replace(p, 2, *a);
		++s[1];
	}
	
	return result;
}

