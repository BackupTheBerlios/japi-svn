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

/*	$Id: MUnicode.h 91 2006-10-10 07:29:58Z maarten $
	Copyright Maarten L. Hekkelman
	Created Monday July 12 2004 21:44:56
	
	Conventions used in this program:
	
	std::string is in UTF-8 encoding, unless specified otherwise.
	ustring is in UTF-16 encoding with native byte ordering.
	wstring is in UCS2 encoding with native byte ordering.
	

	
*/

#ifndef MUNICODE_H
#define MUNICODE_H

#include <vector>
#include <string>

#include "MTypes.h"

enum MEncoding
{
	kEncodingUTF8,
	kEncodingUTF16BE,
	kEncodingUTF16LE,
	kEncodingUCS2,
	kEncodingISO88591,
	kEncodingMacOSRoman,
	
	kEncodingCount,			// number of supported encodings
	kEncodingUnknown = kEncodingCount + 1
};

template<MEncoding encoding>
struct MEncodingTraits
{
	template<class ByteIterator>
	static
	uint32		GetNextCharLength(
					const ByteIterator	inText);

	template<class ByteIterator>
	static
	uint32		GetPrevCharLength(
					const ByteIterator	inText);

	template<class ByteIterator>
	static
	void		ReadUnicode(
					const ByteIterator	inText,
					uint32&				outLength,
					wchar_t&			outUnicode);

	template<class ByteIterator>
	static
	uint32		WriteUnicode(
					ByteIterator&		inText,
					wchar_t				inUnicode);
};

enum WordBreakClass
{
	eWB_CR,
	eWB_LF,
	eWB_Sep,
	eWB_Tab,
	eWB_Let,
	eWB_Com,
	eWB_Hira,
	eWB_Kata,
	eWB_Han,
	eWB_Other,
	eWB_None
};

WordBreakClass GetWordBreakClass(wchar_t inUnicode);

enum CharBreakClass
{
	kCBC_CR,
	kCBC_LF,
	kCBC_Control,
	kCBC_Extend,
	kCBC_L,
	kCBC_V,
	kCBC_T,
	kCBC_LV,
	kCBC_LVT,
	kCBC_Other
};

extern const bool kCharBreakTable[10][10];

CharBreakClass GetCharBreakClass(wchar_t inUnicode);

wchar_t ToLower(wchar_t inChar);
wchar_t ToUpper(wchar_t inChar);
bool IsSpace(wchar_t inChar);
bool IsAlpha(wchar_t inChar);
bool IsNum(wchar_t inChar);
bool IsAlnum(wchar_t inChar);
bool IsCombining(wchar_t inChar);

std::string tolower(std::string inString);

std::string::iterator next_cursor_position(
	std::string::iterator	inStart,
	std::string::iterator	inEnd); 

std::string::iterator next_line_break(
	std::string::iterator	inStart,
	std::string::iterator	inEnd); 

// one byte character set utilities
namespace MUnicodeMapping
{
wchar_t GetUnicode(MEncoding inEncoding, char inByte);
char GetChar(MEncoding inEncoding, wchar_t inChar);
}

class MEncoder
{
  public:
	virtual				~MEncoder() {}
	
	virtual void		WriteUnicode(wchar_t inUnicode) = 0;

	void				SetText(const std::string& inText);
	void				SetText(const std::wstring& inText);
	
	uint32				GetBufferSize() const				{ return mBuffer.size(); }
	const void*			Peek() const						{ return &mBuffer[0]; }
	
	template<class Iterator>
	void				CopyBuffer(Iterator inDest) const	{ std::copy(mBuffer.begin(), mBuffer.end(), inDest); }

	static MEncoder*	GetEncoder(MEncoding inEncoding);

  protected:
	std::vector<char>	mBuffer;
};

class MDecoder
{
  public:
	virtual				~MDecoder() {}
	
	virtual bool		ReadUnicode(wchar_t& outUnicode) = 0;

	void				GetText(std::string& outText);
	void				GetText(std::wstring& outText);
	
	static MDecoder*	GetDecoder(MEncoding inEncoding, const void* inBuffer, uint32 inLength);

  protected:
						MDecoder(const void* inBuffer, uint32 inLength)
							: mBuffer(static_cast<const char*>(inBuffer))
							, mLength(inLength)
						{
						}

	const char*			mBuffer;
	uint32				mLength;
};

//template<typename String1, typename String2>
//void Convert(const String1& inSrc, String2& outDst);

#include "MUnicode.inl"

#endif // MUNICODE_H
