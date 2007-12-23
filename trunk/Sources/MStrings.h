/*
	Copyright (c) 2007, Maarten L. Hekkelman
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

#ifndef MSTRINGS_H
#define MSTRINGS_H

#include <string>
#include <vector>
#include <sstream>

#define _(s)	(GetLocalisedString(s))

const char* GetLocalisedString(
				const char* 		inString);

std::string GetLocalisedString(
				const std::string&	inString);

std::string GetFormattedLocalisedStringWithArguments(
				const std::string&	inString,
				const std::vector<std::string>&
									inArgs);

template<class T1>
std::string FormatString(
				const char*			inString,
				const T1&			inArg1);

template<class T1, class T2>
std::string FormatString(
				const char*			inString,
				const T1&			inArg1,
				const T2&			inArg2);

template<class T1, class T2, class T3>
std::string FormatString(
				const char*			inString,
				const T1&			inArg1,
				const T2&			inArg2,
				const T3&			inArg3);

// --------------------------------------------------------------------

template<class T>
inline 
void PushArgument(
	std::vector<std::string>&	inArgs,
	const T&					inArg)
{
	std::stringstream s;
	s << inArg;
	inArgs.push_back(s.str());
}

inline
void PushArgument(
	std::vector<std::string>&	inArgs,
	const char*					inArg)
{
	inArgs.push_back(GetLocalisedString(inArg));
}

template<>
inline
void PushArgument(
	std::vector<std::string>&	inArgs,
	const std::string&			inArg)
{
	inArgs.push_back(GetLocalisedString(inArg.c_str()));
}

template<class T1>
std::string FormatString(
	const char*		inString,
	const T1&		inArg1)
{
	std::vector<std::string> args;
	PushArgument(args, inArg1);
	return GetFormattedLocalisedStringWithArguments(inString, args);
}

template<class T1, class T2>
std::string FormatString(
	const char*		inString,
	const T1&		inArg1,
	const T2&		inArg2)
{
	std::vector<std::string> args;
	PushArgument(args, inArg1);
	PushArgument(args, inArg2);
	return GetFormattedLocalisedStringWithArguments(inString, args);
}

template<class T1, class T2, class T3>
std::string FormatString(
	const char*		inString,
	const T1&		inArg1,
	const T2&		inArg2,
	const T3&		inArg3)
{
	std::vector<std::string> args;
	PushArgument(args, inArg1);
	PushArgument(args, inArg2);
	PushArgument(args, inArg3);
	return GetFormattedLocalisedStringWithArguments(inString, args);
}

#endif
