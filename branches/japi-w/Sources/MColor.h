//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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
