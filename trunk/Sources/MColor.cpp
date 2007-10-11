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

#include "MJapieG.h"

#include <sstream>
#include <iomanip>

#include "MColor.h"

using namespace std;

const MColor
	kBlack(0, 0, 0),
	kWhite("#ffffff"),
	kCurrentLineColor("#ffffcc"),
	kMarkedLineColor("#efff7f"),
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

MColor& MColor::operator=(
	const MColor&	inOther)
{
	red = inOther.red;
	green = inOther.green;
	blue = inOther.blue;
	return *this;
}

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

