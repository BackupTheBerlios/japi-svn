//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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
					unicode&			outUnicode);

	template<class ByteIterator>
	static
	uint32		WriteUnicode(
					ByteIterator&		inText,
					unicode				inUnicode);
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

WordBreakClass GetWordBreakClass(unicode inUnicode);

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
	kCBC_Other,

	kCBC_Prepend,
	kCBC_SpacingMark
};

extern const bool kCharBreakTable[10][10];

CharBreakClass GetCharBreakClass(unicode inUnicode);

enum LineBreakClass {
	kLBC_OpenPunctuation,
	kLBC_ClosePunctuation,
	kLBC_CloseParenthesis,
	kLBC_Quotation,
	kLBC_NonBreaking,
	kLBC_Nonstarter,
	kLBC_Exlamation,
	kLBC_SymbolAllowingBreakAfter,
	kLBC_InfixNumericSeparator,
	kLBC_PrefixNumeric,
	kLBC_PostfixNumeric,
	kLBC_Numeric,
	kLBC_Alphabetic,
	kLBC_Ideographic,
	kLBC_Inseperable,
	kLBC_Hyphen,
	kLBC_BreakAfter,
	kLBC_BreakBefor,
	kLBC_BreakOpportunityBeforeAndAfter,
	kLBC_ZeroWidthSpace,
	kLBC_CombiningMark,
	kLBC_WordJoiner,
	kLBC_HangulLVSyllable,
	kLBC_HangulLVTSyllable,
	kLBC_HangulLJamo,
	kLBC_HangulVJamo,
	kLBC_HangulTJamo,

	kLBC_MandatoryBreak,
	kLBC_CarriageReturn,
	kLBC_LineFeed,
	kLBC_NextLine,
	kLBC_Surrogate,
	kLBC_Space,
	kLBC_ContigentBreakOpportunity,
	kLBC_Ambiguous,
	kLBC_ComplexContext,
	kLBC_Unknown
};

LineBreakClass GetLineBreakClass(unicode inUnicode);

bool IsSpace(unicode inChar);
bool IsAlpha(unicode inChar);
bool IsNum(unicode inChar);
bool IsAlnum(unicode inChar);
bool IsCombining(unicode inChar);

std::string::iterator next_cursor_position(
	std::string::iterator	inStart,
	std::string::iterator	inEnd); 

std::string::iterator next_line_break(
	std::string::iterator	inStart,
	std::string::iterator	inEnd); 

// one byte character set utilities
namespace MUnicodeMapping
{
unicode GetUnicode(MEncoding inEncoding, char inByte);
char GetChar(MEncoding inEncoding, unicode inChar);
}

class MEncoder
{
  public:
	virtual				~MEncoder() {}
	
	virtual void		WriteUnicode(unicode inUnicode) = 0;

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
	
	virtual bool		ReadUnicode(unicode& outUnicode) = 0;

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

#include "MUnicode.inl"

#endif // MUNICODE_H
