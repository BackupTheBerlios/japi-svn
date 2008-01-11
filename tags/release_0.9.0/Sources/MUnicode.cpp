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

/*	$Id: MUnicode.cpp 151 2007-05-21 15:59:05Z maarten $
	Copyright Maarten L. Hekkelman
	Created Monday July 12 2004 21:45:58
*/

#include "MJapieG.h"

#include <sstream>

#include "MUnicode.h"
#include "MError.h"
#include "MTypes.h"

enum {
	kLETTER			= 0,
	kNUMBER			= 1,
	kCOMBININGMARK	= 2,
	kPUNCTUATION	= 3,
	kSYMBOL			= 4,
	kSEPARATOR		= 5,
	kCONTROL		= 6,
	kOTHER			= 7
};

const bool kCharBreakTable[10][10] = {
	//	CR	LF	Cnt	Ext	L	V	T	LV	LVT	Oth
	{	1,	0,	1,	1,	1,	1,	1,	1,	1,	1	 },		// CR
	{	1,	1,	1,	1,	1,	1,	1,	1,	1,	1	 },		// LF
	{	1,	1,	1,	1,	1,	1,	1,	1,	1,	1	 },		// Control
	{	1,	1,	1,	0,	1,	1,	1,	1,	1,	1	 },		// Extend
	{	1,	1,	1,	0,	0,	0,	1,	0,	0,	1	 },		// L
	{	1,	1,	1,	0,	1,	0,	0,	1,	1,	1	 },		// V
	{	1,	1,	1,	0,	1,	1,	0,	1,	1,	1	 },		// T
	{	1,	1,	1,	0,	1,	0,	0,	1,	1,	1	 },		// LV
	{	1,	1,	1,	0,	1,	1,	0,	1,	1,	1	 },		// LVT
	{	1,	1,	1,	0,	1,	1,	1,	1,	1,	1	 },		// Other
};

#include "MUnicodeTables.h"

using namespace std;

const wchar_t kMacOSRomanChars[] = {
	0x0000,  0x0001,  0x0002,  0x0003,  0x0004,  0x0005,  0x0006,  0x0007,  
	0x0008,  0x0009,  0x000A,  0x000B,  0x000C,  0x000D,  0x000E,  0x000F,  
	0x0010,  0x0011,  0x0012,  0x0013,  0x0014,  0x0015,  0x0016,  0x0017,  
	0x0018,  0x0019,  0x001A,  0x001B,  0x001C,  0x001D,  0x001E,  0x001F,  
	0x0020,  0x0021,  0x0022,  0x0023,  0x0024,  0x0025,  0x0026,  0x0027,  
	0x0028,  0x0029,  0x002A,  0x002B,  0x002C,  0x002D,  0x002E,  0x002F,  
	0x0030,  0x0031,  0x0032,  0x0033,  0x0034,  0x0035,  0x0036,  0x0037,  
	0x0038,  0x0039,  0x003A,  0x003B,  0x003C,  0x003D,  0x003E,  0x003F,  
	0x0040,  0x0041,  0x0042,  0x0043,  0x0044,  0x0045,  0x0046,  0x0047,  
	0x0048,  0x0049,  0x004A,  0x004B,  0x004C,  0x004D,  0x004E,  0x004F,  
	0x0050,  0x0051,  0x0052,  0x0053,  0x0054,  0x0055,  0x0056,  0x0057,  
	0x0058,  0x0059,  0x005A,  0x005B,  0x005C,  0x005D,  0x005E,  0x005F,  
	0x0060,  0x0061,  0x0062,  0x0063,  0x0064,  0x0065,  0x0066,  0x0067,  
	0x0068,  0x0069,  0x006A,  0x006B,  0x006C,  0x006D,  0x006E,  0x006F,  
	0x0070,  0x0071,  0x0072,  0x0073,  0x0074,  0x0075,  0x0076,  0x0077,  
	0x0078,  0x0079,  0x007A,  0x007B,  0x007C,  0x007D,  0x007E,  0x007F,  
	0x00C4,  0x00C5,  0x00C7,  0x00C9,  0x00D1,  0x00D6,  0x00DC,  0x00E1,  
	0x00E0,  0x00E2,  0x00E4,  0x00E3,  0x00E5,  0x00E7,  0x00E9,  0x00E8,  
	0x00EA,  0x00EB,  0x00ED,  0x00EC,  0x00EE,  0x00EF,  0x00F1,  0x00F3,  
	0x00F2,  0x00F4,  0x00F6,  0x00F5,  0x00FA,  0x00F9,  0x00FB,  0x00FC,  
	0x2020,  0x00B0,  0x00A2,  0x00A3,  0x00A7,  0x2022,  0x00B6,  0x00DF,  
	0x00AE,  0x00A9,  0x2122,  0x00B4,  0x00A8,  0x2260,  0x00C6,  0x00D8,  
	0x221E,  0x00B1,  0x2264,  0x2265,  0x00A5,  0x00B5,  0x2202,  0x2211,  
	0x220F,  0x03C0,  0x222B,  0x00AA,  0x00BA,  0x03A9,  0x00E6,  0x00F8,  
	0x00BF,  0x00A1,  0x00AC,  0x221A,  0x0192,  0x2248,  0x2206,  0x00AB,  
	0x00BB,  0x2026,  0x00A0,  0x00C0,  0x00C3,  0x00D5,  0x0152,  0x0153,  
	0x2013,  0x2014,  0x201C,  0x201D,  0x2018,  0x2019,  0x00F7,  0x25CA,  
	0x00FF,  0x0178,  0x2044,  0x20AC,  0x2039,  0x203A,  0xFB01,  0xFB02,  
	0x2021,  0x00B7,  0x201A,  0x201E,  0x2030,  0x00C2,  0x00CA,  0x00C1,  
	0x00CB,  0x00C8,  0x00CD,  0x00CE,  0x00CF,  0x00CC,  0x00D3,  0x00D4,  
	0xF8FF,  0x00D2,  0x00DA,  0x00DB,  0x00D9,  0x0131,  0x02C6,  0x02DC,  
	0x00AF,  0x02D8,  0x02D9,  0x02DA,  0x00B8,  0x02DD,  0x02DB,  0x02C7,  
};

namespace MUnicodeMapping
{

wchar_t GetUnicode(MEncoding inEncoding, char inByte)
{
	return kMacOSRomanChars[static_cast<uint8>(inByte)];
}

char GetChar(MEncoding inEncoding, wchar_t inChar)
{
	char result = 0;
	
	for (int i = 0; i < 256; ++i)
	{
		if (inChar == kMacOSRomanChars[i])
		{
			result = static_cast<char>(i);
			break;
		}
	}
	
	if (inChar != 0 and result == 0)
	{
		result = '?';
	}
	
	return result;
}

}


uint8 GetProperty(
	wchar_t		inUnicode)
{
	uint8 result = 0;
	
	if (inUnicode < 0x110000)
	{
		uint32 ix = inUnicode >> 8;
		uint32 p_ix = inUnicode & 0x00FF;
		
		ix = kUnicodeInfo.page_index[ix];
		result = kUnicodeInfo.data[ix][p_ix].prop;
	}
	
	return result;
}

WordBreakClass GetWordBreakClass(wchar_t inUnicode)
{
	WordBreakClass result = eWB_Other;
	
	switch (inUnicode)
	{
		case '\r':
			result = eWB_CR;
			break;

		case '\n':
			result = eWB_LF;
			break;

		case '\t':
			result = eWB_Tab;
			break;
		
		default:
		{
			uint8 prop = GetProperty(inUnicode);
			if (prop == kLETTER or prop == kNUMBER)
			{
				if (inUnicode >= 0x003040 and inUnicode <= 0x00309f)
					result = eWB_Hira;
				else if (inUnicode >= 0x0030a0 and inUnicode <= 0x0030ff)
					result = eWB_Kata;
				else if (inUnicode >= 0x004e00 and inUnicode <= 0x009fff)
					result = eWB_Han;
				else if (inUnicode >= 0x003400 and inUnicode <= 0x004DFF)
					result = eWB_Han;
				else if (inUnicode >= 0x00F900 and inUnicode <= 0x00FAFF)
					result = eWB_Han;
				else
					result = eWB_Let;
			}
			else if (prop == kCOMBININGMARK)
				result = eWB_Com;
			else if (prop == kSEPARATOR)
				result = eWB_Sep;
		}
	}
	
	return result;
}

CharBreakClass GetCharBreakClass(
	wchar_t		inUnicode)
{
	CharBreakClass result = kCBC_Other;
	
	if (inUnicode < 0x110000)
	{
		uint32 ix = inUnicode >> 8;
		uint32 p_ix = inUnicode & 0x00FF;
		
		ix = kUnicodeInfo.page_index[ix];
		result = kUnicodeInfo.data[ix][p_ix].cbc;
	}
	
	return result;
}

wchar_t ToLower(
	wchar_t		inUnicode)
{
	wchar_t result = 0;
	
	if (inUnicode < 0x110000)
	{
		uint32 ix = inUnicode >> 8;
		uint32 p_ix = inUnicode & 0x00FF;
		
		ix = kUnicodeInfo.page_index[ix];
		result = kUnicodeInfo.data[ix][p_ix].lower;
	}
	
	return result;
}

wchar_t ToUpper(
	wchar_t		inUnicode)
{
	wchar_t result = 0;
	
	if (inUnicode < 0x110000)
	{
		uint32 ix = inUnicode >> 8;
		uint32 p_ix = inUnicode & 0x00FF;
		
		ix = kUnicodeInfo.page_index[ix];
		result = kUnicodeInfo.data[ix][p_ix].upper;
	}
	
	return result;
}

bool IsSpace(
	wchar_t		inUnicode)
{
	return
		(inUnicode >= 0x0009 and inUnicode <= 0x000D) or
		inUnicode == 0x0020 or
		inUnicode == 0x0085 or
		inUnicode == 0x00A0 or
		inUnicode == 0x1680 or
		inUnicode == 0x180E or
		(inUnicode >= 0x2000 and inUnicode <= 0x200A) or
		inUnicode == 0x2028 or
		inUnicode == 0x2029 or
		inUnicode == 0x202f or
		inUnicode == 0x205f or
		inUnicode == 0x3000;
}

bool IsAlpha(
	wchar_t		inUnicode)
{
	return GetProperty(inUnicode) == kLETTER;
}

bool IsNum(
	wchar_t		inUnicode)
{
	return GetProperty(inUnicode) == kNUMBER;
}

bool IsAlnum(wchar_t inUnicode)
{
	uint8 prop = GetProperty(inUnicode);
	
	return prop == kLETTER or prop == kNUMBER;
}

bool IsCombining(wchar_t inUnicode)
{
	return GetProperty(inUnicode) == kCOMBININGMARK;
}

template<MEncoding ENCODING>
class MEncoderImpl : public MEncoder
{
	typedef MEncodingTraits<ENCODING>	traits;
  public:
	
	virtual void	WriteUnicode(wchar_t inUnicode)
					{
						back_insert_iterator<vector<char> > iter(mBuffer);
						traits::WriteUnicode(iter, inUnicode);
					}
};

void MEncoder::SetText(const string& inText)
{
	MDecoder* decoder = MDecoder::GetDecoder(kEncodingUTF8,
		inText.c_str(), inText.length());
	
	wchar_t uc;
	while (decoder->ReadUnicode(uc))
		WriteUnicode(uc);
}

void MEncoder::SetText(const wstring& inText)
{
	for (wstring::const_iterator ch = inText.begin(); ch != inText.end(); ++ch)
		WriteUnicode(*ch);
}

MEncoder* MEncoder::GetEncoder(MEncoding inEncoding)
{
	MEncoder* encoder;
	switch (inEncoding)
	{
		case kEncodingUTF8:
			encoder = new MEncoderImpl<kEncodingUTF8>();
			break;
		
		case kEncodingUTF16BE:
			encoder = new MEncoderImpl<kEncodingUTF16BE>();
			break;
		
//		case kEncodingUTF16LE:
//			encoder = new MEncoderImpl<kEncodingUTF16LE>();
//			break;
		
		case kEncodingUCS2:
			encoder = new MEncoderImpl<kEncodingUCS2>();
			break;
		
		case kEncodingMacOSRoman:
			encoder = new MEncoderImpl<kEncodingMacOSRoman>();
			break;
		
		case kEncodingISO88591:
			encoder = new MEncoderImpl<kEncodingISO88591>();
			break;
		
		default:
			assert(false);
			THROW(("Unknown encoding"));
			break;
	}
	
	return encoder;
}


template<MEncoding ENCODING>
class MDecoderImpl : public MDecoder
{
	typedef MEncodingTraits<ENCODING>	traits;
  public:

					MDecoderImpl(const void* inBuffer, uint32 inLength)
						: MDecoder(inBuffer, inLength)
					{
					}
	
	virtual bool	ReadUnicode(wchar_t& outUnicode)
					{
						bool result = false;
						if (mLength > 0)
						{
							uint32 l;
							traits::ReadUnicode(mBuffer, l, outUnicode);
							
							if (l > 0)
							{
								mBuffer += l;
								mLength -= l;
								result = true;
							}
						}
						
						return result;
					}
};

MDecoder* MDecoder::GetDecoder(MEncoding inEncoding, const void* inBuffer, uint32 inLength)
{
	MDecoder* decoder;
	switch (inEncoding)
	{
		case kEncodingUTF8:
			decoder = new MDecoderImpl<kEncodingUTF8>(inBuffer, inLength);
			break;
		
		case kEncodingUTF16BE:
			decoder = new MDecoderImpl<kEncodingUTF16BE>(inBuffer, inLength);
			break;
		
		case kEncodingUTF16LE:
			decoder = new MDecoderImpl<kEncodingUTF16LE>(inBuffer, inLength);
			break;
		
		case kEncodingUCS2:
			decoder = new MDecoderImpl<kEncodingUCS2>(inBuffer, inLength);
			break;
		
		case kEncodingMacOSRoman:
			decoder = new MDecoderImpl<kEncodingMacOSRoman>(inBuffer, inLength);
			break;
		
		case kEncodingISO88591:
			decoder = new MDecoderImpl<kEncodingISO88591>(inBuffer, inLength);
			break;
		
		default:
			assert(false);
			THROW(("Unknown encoding"));
			break;
	}
	
	return decoder;
}

void MDecoder::GetText(string& outText)
{
	wchar_t uc;
	
	MEncodingTraits<kEncodingUTF8> traits;
	vector<char> b;
	back_insert_iterator<vector<char> > i(b);
	
	while (ReadUnicode(uc))
		traits.WriteUnicode(i, uc);
	
	outText.assign(b.begin(), b.end());
}

void MDecoder::GetText(wstring& outText)
{
	wchar_t uc;
	
	outText.clear();
	
	while (ReadUnicode(uc))
		outText += uc;
}

string tolower(
	string		inText)
{
	return inText;
}

string::iterator next_cursor_position(
	string::iterator	inStart,
	string::iterator	inEnd)
{
	string::iterator result = inEnd;

	uint32 length;
	wchar_t ch;
	
	MEncodingTraits<kEncodingUTF8>::ReadUnicode(inStart, length, ch);
	CharBreakClass c1 = GetCharBreakClass(ch);

	while (inEnd - inStart >= static_cast<int>(length))
    {
		inStart += length;
		result = inStart;

		MEncodingTraits<kEncodingUTF8>::ReadUnicode(inStart, length, ch);

		CharBreakClass c2 = GetCharBreakClass(ch);
		
		if (kCharBreakTable[c1][c2])
			break;
		
		c1 = c2;
	}
	
	return result;
}
