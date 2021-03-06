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

#ifndef MCOLOR_H
#define MCOLOR_H

#include <iostream>
#include <iomanip>

struct MColor
{
  public:
	uint8		red;
	uint8		green;
	uint8		blue;

				MColor();

				MColor(
					const MColor&		inOther);

				MColor(
					const GdkColor&		inColor);

				MColor(
					const char*			inHex);

				MColor(
					uint8				inRed,
					uint8				inGreen,
					uint8				inBlue);

	MColor&		operator=(
					const MColor&		inOther);

				operator GdkColor() const;

	std::string	hex() const;
	void		hex(
					const std::string&	inHex);
};

extern const MColor
	kWhite,
	kBlack,
	kNoteColor,
	kWarningColor,
	kErrorColor,
	kSelectionColor;

//template<class charT, class traits>
//std::basic_ostream<charT,traits>&
//operator << (
//	std::basic_ostream<charT,traits>&	os,
//	const MColor&						inColor)
//{
//	std::ios_base::fmtflags flags = os.setf(std::ios_base::hex, std::ios_base::basefield);
//	
//	os << '#'
//		<< std::setw(2) << std::setfill('0') << static_cast<uint32>(inColor.red)
//		<< std::setw(2) << std::setfill('0') << static_cast<uint32>(inColor.green)
//		<< std::setw(2) << std::setfill('0') << static_cast<uint32>(inColor.blue);
//	
//	os.setf(flags);
//	return os;
//}

#endif
