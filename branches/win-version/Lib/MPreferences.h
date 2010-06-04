//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MPreferences.h 80 2006-09-11 08:50:35Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 01 2004 13:32:55
*/

#ifndef MYPREFS_H
#define MYPREFS_H

#include <string>
#include <vector>

#include "MColor.h"
#include "MTypes.h"

namespace Preferences
{

int32	GetInteger(const char* inName, int32 inDefaultValue);
void	SetInteger(const char* inName, int32 inValue);

std::string	GetString(const char* inName, std::string inDefaultValue);
void	SetString(const char* inName, std::string inValue);

void	GetArray(const char* inName, std::vector<std::string>& outArray);
void	SetArray(const char* inName, const std::vector<std::string>& inArray);

MColor	GetColor(const char* inName, MColor inDefaultValue);
void	SetColor(const char* inName, MColor inValue);

MRect	GetRect(const char* inName, MRect inDefault);
void	SetRect(const char* inName, MRect inValue);

}

#endif // MYPREFS_H
