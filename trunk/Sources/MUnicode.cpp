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
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <unicode/utext.h>
#include <unicode/unistr.h>

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

#include "MUnicodeTables.h"

using namespace std;

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
		
//		case kEncodingMacOSRoman:
//			encoder = new MEncoderImpl<kEncodingMacOSRoman>();
//			break;
		
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
		
//		case kEncodingMacOSRoman:
//			decoder = new MDecoderImpl<kEncodingMacOSRoman>(inBuffer, inLength);
//			break;
		
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
	
}
