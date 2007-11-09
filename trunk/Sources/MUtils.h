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

#ifndef MUTILS_H
#define MUTILS_H

#include "MTypes.h"

struct no_swapper
{
	template<typename T>
	T			operator()(T inValue) const			{ return inValue; }
};

struct swapper
{
	template<typename T>
	T			operator()(T inValue) const
	{
		this_will_not_compile_I_hope(inValue);
	}
};

template<>
inline
uint16 swapper::operator()(uint16 inValue) const
{
	return static_cast<uint16>(
		((inValue & 0xFF00UL) >>  8) |
		((inValue & 0x00FFUL) <<  8)
	);
}

template<>
inline
uint32 swapper::operator()(uint32 inValue) const
{
	return static_cast<uint32>(
		((inValue & 0xFF000000UL) >> 24) |
		((inValue & 0x00FF0000UL) >>  8) |
		((inValue & 0x0000FF00UL) <<  8) |
		((inValue & 0x000000FFUL) << 24)
	);
}

template<>
inline
long unsigned int swapper::operator()(long unsigned int inValue) const
{
	return static_cast<long unsigned int>(
		((inValue & 0xFF000000UL) >> 24) |
		((inValue & 0x00FF0000UL) >>  8) |
		((inValue & 0x0000FF00UL) <<  8) |
		((inValue & 0x000000FFUL) << 24)
	);
}

template<>
inline
int64 swapper::operator()(int64 inValue) const
{
	return static_cast<int64>(
		(((static_cast<uint64>(inValue))<<56) & 0xFF00000000000000ULL)  |
		(((static_cast<uint64>(inValue))<<40) & 0x00FF000000000000ULL)  |
		(((static_cast<uint64>(inValue))<<24) & 0x0000FF0000000000ULL)  |
		(((static_cast<uint64>(inValue))<< 8) & 0x000000FF00000000ULL)  |
		(((static_cast<uint64>(inValue))>> 8) & 0x00000000FF000000ULL)  |
		(((static_cast<uint64>(inValue))>>24) & 0x0000000000FF0000ULL)  |
		(((static_cast<uint64>(inValue))>>40) & 0x000000000000FF00ULL)  |
		(((static_cast<uint64>(inValue))>>56) & 0x00000000000000FFULL));
}

template<>
inline
uint64 swapper::operator()(uint64 inValue) const
{
	return static_cast<uint64>(
		((((uint64)inValue)<<56) & 0xFF00000000000000ULL)  |
		((((uint64)inValue)<<40) & 0x00FF000000000000ULL)  |
		((((uint64)inValue)<<24) & 0x0000FF0000000000ULL)  |
		((((uint64)inValue)<< 8) & 0x000000FF00000000ULL)  |
		((((uint64)inValue)>> 8) & 0x00000000FF000000ULL)  |
		((((uint64)inValue)>>24) & 0x0000000000FF0000ULL)  |
		((((uint64)inValue)>>40) & 0x000000000000FF00ULL)  |
		((((uint64)inValue)>>56) & 0x00000000000000FFULL));
}

#if BYTE_ORDER == LITTLE_ENDIAN
typedef no_swapper	lsb_swapper;
typedef swapper		msb_swapper;
#else
typedef swapper		lsb_swapper;
typedef no_swapper	msb_swapper;
#endif

uint16 CalculateCRC(const void* inData, uint32 inLength, uint16 inCRC);

std::string Escape(std::string inString);
std::string Unescape(std::string inString);

std::string NumToString(uint32 inNumber);
uint32 StringToNum(std::string inString);

std::string GetLocalisedString(const char* inString);

std::string	GetUserName(bool inShortName = false);
std::string	GetDateTime();

void NormalizePath(std::string& ioPath);

double GetLocalTime();
double GetDoubleClickTime();

void Beep();

void ObscureCursor();

#endif
