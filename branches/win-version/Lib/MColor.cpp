//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include <sstream>
#include <iomanip>

#include "MColor.h"

using namespace std;

const MColor
	kBlack("#000000"),
	kWhite("#ffffff"),
	kNoteColor("#206cff"),
	kWarningColor("#ffeb17"),
	kErrorColor("#ff4811"),
	kSelectionColor("#f3bb6b");

MColor
	gSelectionColor = kSelectionColor;

MColor::MColor()
	: red(0), green(0), blue(0)
{
}

MColor::MColor(
	const MColor&	inOther)
{
	red = inOther.red;
	green = inOther.green;
	blue = inOther.blue;
}

//MColor::MColor(
//	const GdkColor&	inOther)
//{
//	red = inOther.red >> 8;
//	green = inOther.green >> 8;
//	blue = inOther.blue >> 8;
//}

MColor::MColor(
	const char*		inHex)
{
	hex(inHex);
}

MColor::MColor(
	uint8			inRed,
	uint8			inGreen,
	uint8			inBlue)
{
	red = inRed;
	green = inGreen;
	blue = inBlue;
}

MColor::MColor(
	float			inRed,
	float			inGreen,
	float			inBlue)
{
	red = static_cast<uint8>(inRed * 255);
	green = static_cast<uint8>(inGreen * 255);
	blue = static_cast<uint8>(inBlue * 255);
}

MColor& MColor::operator=(
	const MColor&	inOther)
{
	red = inOther.red;
	green = inOther.green;
	blue = inOther.blue;
	return *this;
}

//MColor::operator GdkColor() const
//{
//	GdkColor result = {};
//	result.red = red << 8 | red;
//	result.green = green << 8 | green;
//	result.blue = blue << 8 | blue;
//	return result;
//}

string MColor::hex() const
{
	stringstream s;
	
	s.setf(ios_base::hex, ios_base::basefield);
	
	s << '#'
		<< setw(2) << setfill('0') << static_cast<uint32>(red)
		<< setw(2) << setfill('0') << static_cast<uint32>(green)
		<< setw(2) << setfill('0') << static_cast<uint32>(blue);
	
	return s.str();
}

void MColor::hex(
	const string&	inHex)
{
	if (inHex.length() == 7 and inHex[0] == '#')
	{
		stringstream s;
		uint32 n;
		s.setf(ios_base::hex, ios_base::basefield);

		s.str(inHex.substr(1, 2));
		s.clear();
		s >> n;
		red = n;
		
		s.str(inHex.substr(3, 2));
		s.clear();
		s >> n;
		green = n;
		
		s.str(inHex.substr(5, 2));
		s.clear();
		s >> n;
		blue = n;
	}	
}

