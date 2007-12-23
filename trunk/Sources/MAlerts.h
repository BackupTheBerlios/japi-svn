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

#ifndef MALERTS_H
#define MALERTS_H

#include <sstream>
#include <vector>

int32 DisplayAlertWithArgs(
		const char*		inResourceName,
		std::vector<std::string>&
						inArgs);

template<class T>
inline 
void AddArgument(
	std::vector<std::string>&	inArgs,
	const T&					inArg)
{
	std::stringstream s;
	s << inArg;
	inArgs.push_back(s.str());
}

inline
int32 DisplayAlert(
	const char*			inResourceName)
{
	std::vector<std::string> args;
	return DisplayAlertWithArgs(inResourceName, args);
}

template<class T1>
int32 DisplayAlert(
	const char*			inResourceName,
	const T1&			inArg1)
{
	std::vector<std::string> args;
	AddArgument(args, inArg1);
	return DisplayAlertWithArgs(inResourceName, args);
}

template<class T1, class T2>
int32 DisplayAlert(
	const char*			inResourceName,
	const T1&			inArg1,
	const T2&			inArg2)
{
	std::vector<std::string> args;
	AddArgument(args, inArg1);
	AddArgument(args, inArg2);
	return DisplayAlertWithArgs(inResourceName, args);
}

template<class T1, class T2, class T3>
int32 DisplayAlert(
	const char*			inResourceName,
	const T1&			inArg1,
	const T2&			inArg2,
	const T3&			inArg3)
{
	std::vector<std::string> args;
	AddArgument(args, inArg1);
	AddArgument(args, inArg2);
	AddArgument(args, inArg3);
	return DisplayAlertWithArgs(inResourceName, args);
}

#endif
