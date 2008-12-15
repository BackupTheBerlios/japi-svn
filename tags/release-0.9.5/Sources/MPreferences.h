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
