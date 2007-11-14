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

/*	$Id: MTextBuffer.cpp 160 2007-05-30 15:37:02Z maarten $
	Copyright Maarten L. Hekkelman
	Created Monday June 21 2004 20:58:09
*/

#include "MJapieG.h"

#include <list>
#include <set>
#include <unistd.h>

// for some weird reason the BOOST_ASSERT's wreak havoc here...
#if DEBUG
#define BOOST_DISABLE_ASSERTS 1
#endif

#include <pcre.h>

#include "MTextBuffer.h"
//#include "MUnicode.h"
#include "MSelection.h"
#include "MError.h"
#include "MFile.h"
#include "MPreferences.h"
//#include "MMessageWindow.h"

using namespace std;

namespace
{

// PCRE error messages for version 7.4

const char* kPCRE_ERR_STR[] = {
/* 0 */		"no error",
/* 1 */		"\\ at end of pattern",
/* 2 */		"\\c at end of pattern",
/* 3 */		"unrecognized character follows \\",
/* 4 */		"numbers out of order in {} quantifier",
/* 5 */		"number too big in {} quantifier",
/* 6 */		"missing terminating ] for character class",
/* 7 */		"invalid escape sequence in character class",
/* 8 */		"range out of order in character class",
/* 9 */		"nothing to repeat",
/* 10 */	"[this code is not in use]",
/* 11 */	"internal error: unexpected repeat",
/* 12 */	"unrecognized character after (?",
/* 13 */	"POSIX named classes are supported only within a class",
/* 14 */	"missing )",
/* 15 */	"reference to non-existent subpattern",
/* 16 */	"erroffset passed as NULL",
/* 17 */	"unknown option bit(s) set",
/* 18 */	"missing ) after comment",
/* 19 */	"[this code is not in use]",
/* 20 */	"regular expression too large",
/* 21 */	"failed to get memory",
/* 22 */	"unmatched parentheses",
/* 23 */	"internal error: code overflow",
/* 24 */	"unrecognized character after (?<",
/* 25 */	"lookbehind assertion is not fixed length",
/* 26 */	"malformed number or name after (?(",
/* 27 */	"conditional group contains more than two branches",
/* 28 */	"assertion expected after (?(",
/* 29 */	"(?R or (?[+-]digits must be followed by )",
/* 30 */	"unknown POSIX class name",
/* 31 */	"POSIX collating elements are not supported",
/* 32 */	"this version of PCRE is not compiled with PCRE_UTF8 support",
/* 33 */	"[this code is not in use]",
/* 34 */	"character value in \\x{...} sequence is too large",
/* 35 */	"invalid condition (?(0)",
/* 36 */	"\\C not allowed in lookbehind assertion",
/* 37 */	"PCRE does not support \\L, \\l, \\N, \\U, or \\u",
/* 38 */	"number after (?C is > 255",
/* 39 */	"closing ) for (?C expected",
/* 40 */	"recursive call could loop indefinitely",
/* 41 */	"unrecognized character after (?P",
/* 42 */	"syntax error in subpattern name (missing terminator)",
/* 43 */	"two named subpatterns have the same name",
/* 44 */	"invalid UTF-8 string",
/* 45 */	"support for \\P, \\p, and \\X has not been compiled",
/* 46 */	"malformed \\P or \\p sequence",
/* 47 */	"unknown property name after \\P or \\p",
/* 48 */	"subpattern name is too long (maximum 32 characters)",
/* 49 */	"too many named subpatterns (maximum 10,000)",
/* 50 */	"[this code is not in use]",
/* 51 */	"octal value is greater than \\377 (not in UTF-8 mode)",
/* 52 */	"internal error: overran compiling workspace",
/* 53 */	"internal error: previously-checked referenced subpattern not found",
/* 54 */	"DEFINE group contains more than one branch",
/* 55 */	"repeating a DEFINE group is not allowed",
/* 56 */	"inconsistent NEWLINE options",
/* 57 */	"\\g is not followed by a braced name or an optionally braced non-zero number",
/* 58 */	"(?+ or (?- or (?(+ or (?(- must be followed by a non-zero number",
};

const uint32 kBlockSize = 10240;

class wc_iterator : public boost::iterator_facade<wc_iterator, const wchar_t,
	boost::bidirectional_traversal_tag, const wchar_t>
{
  public:
					wc_iterator(const MTextBuffer* inBuffer, uint32 inOffset) : mBuffer(inBuffer), mOffset(inOffset) {}
					wc_iterator() : mBuffer(nil), mOffset(0) {}
					wc_iterator(const wc_iterator& inOther) : mBuffer(inOther.mBuffer), mOffset(inOther.mOffset) {}
	
	uint32			GetOffset() const							{ return mOffset; }
	
  private:
	friend class boost::iterator_core_access;

	wchar_t			dereference() const							{ return mBuffer->GetWChar(mOffset); }

	void			increment()									{ mOffset += mBuffer->GetNextCharLength(mOffset); }
	void			decrement()									{ mOffset -= mBuffer->GetPrevCharLength(mOffset); }
	bool			equal(const wc_iterator& inOther) const		{ return mOffset == inOther.mOffset; }
	
	const MTextBuffer*	mBuffer;
	uint32			mOffset;
};

}

// -----------------------------------------------------------------------------
// Undo/Redo support

class MicroAction
{
  public:
					MicroAction();
					MicroAction(const MicroAction&);
					~MicroAction();
	
						// Delete
	void			DoDelete(
							MTextBuffer&	inBuffer,
							uint32			inOffset,
							uint32			inLength);

						// Insert
	void			DoInsert(
							MTextBuffer&	inBuffer,
							uint32			inOffset,
							const char*		inText,
							uint32			inLength);

	bool			DoInsertMore(
							MTextBuffer&	inBuffer,
							uint32			inOffset,
							const char*		inText,
							uint32			inLength);

	void			Undo(	MTextBuffer&	inBuffer,
							uint32&			ioChangeOffset,	// The minimal offset of text touched by this Undo
							uint32&			ioChangeLength,	// Total length of area touched by Undo
							int32&			ioDelta);		// The cumulative amount of characters deleted/inserted

	void			Redo(	MTextBuffer&	inBuffer,
							uint32&			ioChangeOffset,	// The minimal offset of text touched by this Undo
							uint32&			ioChangeLength,	// Total length of area touched by Undo
							int32&			ioDelta);		// The cumulative amount of characters deleted/inserted

  private:

	void			Delete(	MTextBuffer&	inBuffer,
							uint32&			ioChangeOffset,	// The minimal offset of text touched by this Undo
							uint32&			ioChangeLength,	// Total length of area touched by Undo
							int32&			ioDelta);		// The cumulative amount of characters deleted/inserted

	void			Insert(	MTextBuffer&	inBuffer,
							uint32&			ioChangeOffset,	// The minimal offset of text touched by this Undo
							uint32&			ioChangeLength,	// Total length of area touched by Undo
							int32&			ioDelta);		// The cumulative amount of characters deleted/inserted

	MicroAction&	operator=(const MicroAction&);

	char*			mSavedText;
	uint32			mOffset;
	int32			mLength;
};

typedef list<MicroAction>	MicroActionList;

class Action
{
  public:
					Action(
						MTextBuffer&		inBuffer,
						const string&		inTitle);
	
	void			Insert(
						uint32				inOffset,
						const char*			inText,
						uint32				inLength);

	void			Delete(
						uint32				inOffset,
						uint32				inLength);
	
	void			SetSelectionBefore(
						const MSelection&	inSelection);

	MSelection		GetSelectionBefore() const					{ return mSelectionBefore; }
	
	void			SetSelectionAfter(
						const MSelection&	inSelection);
	MSelection		GetSelectionAfter() const					{ return mSelectionAfter; }
	
	const string&	GetTitle() const							{ return mTitle; }
	
	void			Undo(
						MSelection&			outSelection,
						uint32&				outChangeOffset,	// The minimal offset of text touched by this Undo
						uint32&				outChangeLength,	// Total length of area touched by Undo
						int32&				outDelta);			// The cumulative amount of characters deleted/inserted

	void			Redo(
						MSelection&			outSelection,
						uint32&				outChangeOffset,	// The minimal offset of text touched by this Undo
						uint32&				outChangeLength,	// Total length of area touched by Undo
						int32&				outDelta);			// The cumulative amount of characters deleted/inserted

  private:
	MTextBuffer&	mBuffer;
	string			mTitle;
	MSelection		mSelectionBefore;
	MSelection		mSelectionAfter;
	MicroActionList	mMicroActions;
};

// -----------------------------------------------------------------------------

MicroAction::MicroAction()
	: mSavedText(nil)
	, mOffset(0)
	, mLength(0)
{
}

MicroAction::MicroAction(
	const MicroAction& inOther)
	: mSavedText(inOther.mSavedText)
	, mOffset(inOther.mOffset)
	, mLength(inOther.mLength)
{
	assert(inOther.mSavedText == nil);
}

MicroAction::~MicroAction()
{
	delete[] mSavedText;
}

void
MicroAction::DoDelete(
	MTextBuffer&	inBuffer,
	uint32			inOffset,
	uint32			inLength)
{
	mSavedText = new char[inLength];
	mOffset = inOffset;
	mLength = -inLength;

	inBuffer.GetText(mOffset, mSavedText, inLength);
	inBuffer.DeleteSelf(mOffset, inLength);
}

void
MicroAction::DoInsert(
	MTextBuffer&	inBuffer,
	uint32			inOffset,
	const char*		inText,
	uint32			inLength)
{
	mOffset = inOffset;
	mLength = inLength;
	inBuffer.InsertSelf(mOffset, inText, mLength);
}

bool
MicroAction::DoInsertMore(
	MTextBuffer&	inBuffer,
	uint32			inOffset,
	const char*		inText,
	uint32			inLength)
{
	bool result = false;
	
	if (mLength > 0 and inOffset == mOffset + mLength)
	{
		inBuffer.InsertSelf(inOffset, inText, inLength);
		mLength += inLength;
		result = true;
	}
	
	return result;
}

void
MicroAction::Undo(
	MTextBuffer&	inBuffer,
	uint32&			ioChangeOffset,	// The minimal offset of text touched by this Undo
	uint32&			ioChangeLength,	// Total length of area touched by Undo
	int32&			ioDelta)		// The cumulative amount of characters deleted/inserted
{
	if (mLength > 0)
		Delete(inBuffer, ioChangeOffset, ioChangeLength, ioDelta);
	else
		Insert(inBuffer, ioChangeOffset, ioChangeLength, ioDelta);
}

void
MicroAction::Redo(
	MTextBuffer&	inBuffer,
	uint32&			ioChangeOffset,	// The minimal offset of text touched by this Undo
	uint32&			ioChangeLength,	// Total length of area touched by Undo
	int32&			ioDelta)		// The cumulative amount of characters deleted/inserted
{
	if (mLength > 0)
		Insert(inBuffer, ioChangeOffset, ioChangeLength, ioDelta);
	else
		Delete(inBuffer, ioChangeOffset, ioChangeLength, ioDelta);
}

void
MicroAction::Delete(
	MTextBuffer&	inBuffer,
	uint32&			ioChangeOffset,	// The minimal offset of text touched by this Undo
	uint32&			ioChangeLength,	// Total length of area touched by Undo
	int32&			ioDelta)		// The cumulative amount of characters deleted/inserted
{
	uint32 length = abs(mLength);
	mSavedText = new char[length];
	
	inBuffer.GetText(mOffset, mSavedText, length);
	inBuffer.DeleteSelf(mOffset, length);
	
	if (mOffset < ioChangeOffset)
	{
		ioChangeLength += ioChangeOffset - mOffset;
		ioChangeOffset = mOffset;
	}
	
	if (mOffset + length > ioChangeOffset + ioChangeLength)
		ioChangeLength = mOffset + length - ioChangeOffset;

	ioDelta -= length;
}

void
MicroAction::Insert(
	MTextBuffer&	inBuffer,
	uint32&			ioChangeOffset,	// The minimal offset of text touched by this Undo
	uint32&			ioChangeLength,	// Total length of area touched by Undo
	int32&			ioDelta)		// The cumulative amount of characters deleted/inserted
{
	uint32 length = abs(mLength);
	
	inBuffer.InsertSelf(mOffset, mSavedText, length);
	delete[] mSavedText;
	mSavedText = nil;

	if (mOffset < ioChangeOffset)
	{
		ioChangeLength += ioChangeOffset - mOffset;
		ioChangeOffset = mOffset;
	}
	
	if (mOffset + length > ioChangeOffset + ioChangeLength)
		ioChangeLength = mOffset + length - ioChangeOffset;

	ioDelta += length;
}

// -----------------------------------------------------------------------------

Action::Action(
	MTextBuffer&		inBuffer,
	const string&		inTitle)
	: mBuffer(inBuffer)
	, mTitle(inTitle)
{
}

void
Action::SetSelectionBefore(
	const MSelection&	inSelection)
{
	mSelectionBefore = inSelection;
}

void
Action::SetSelectionAfter(
	const MSelection&	inSelection)
{
	mSelectionAfter = inSelection;
}

void
Action::Insert(
	uint32				inOffset,
	const char*			inText,
	uint32				inLength)
{
	if (mMicroActions.size() == 0 or
		not mMicroActions.back().DoInsertMore(mBuffer, inOffset, inText, inLength))
	{
		mMicroActions.push_back(MicroAction());
		mMicroActions.back().DoInsert(mBuffer, inOffset, inText, inLength);
	}
	
	if (mSelectionAfter.IsEmpty())
		mSelectionAfter.Set(inOffset, inOffset + inLength);
	else
	{
		uint32 a = mSelectionAfter.GetAnchor();
		uint32 c = mSelectionAfter.GetCaret();
		if (a > c)
			swap(a, c);
		mSelectionAfter.Set(a, inOffset + inLength);
	}
}

void
Action::Delete(
	uint32				inOffset,
	uint32				inLength)
{
	mMicroActions.push_back(MicroAction());
	mMicroActions.back().DoDelete(mBuffer, inOffset, inLength);
	
	if (not mSelectionBefore.IsBlock())
	{
		uint32 a = mSelectionBefore.GetAnchor();
		uint32 c = mSelectionBefore.GetCaret();
		if (a > c)
			swap(a, c);
		
		if (inOffset < a)
			a = inOffset;
		mSelectionBefore.Set(a, c);
	}
	
	if (not mSelectionAfter.IsEmpty())
	{
		uint32 a = mSelectionBefore.GetAnchor();
		uint32 c = mSelectionBefore.GetCaret();
		if (a > c)
			swap(a, c);
		if (c == inOffset + inLength)
			mSelectionAfter.Set(a, c);
	}
}
	
void
Action::Undo(
	MSelection&			outSelection,
	uint32&				outChangeOffset,	// The minimal offset of text touched by this Undo
	uint32&				outChangeLength,	// Total length of area touched by Undo
	int32&				outDelta)			// The cumulative amount of characters deleted/inserted
{
	outSelection = mSelectionBefore;
	outChangeOffset = outSelection.GetAnchor();
	outChangeLength = 0;
	outDelta = 0;
	
	for (MicroActionList::reverse_iterator m = mMicroActions.rbegin(); m != mMicroActions.rend(); ++m)
		(*m).Undo(mBuffer, outChangeOffset, outChangeLength, outDelta);
}

void
Action::Redo(
	MSelection&			outSelection,
	uint32&				outChangeOffset,	// The minimal offset of text touched by this Undo
	uint32&				outChangeLength,	// Total length of area touched by Undo
	int32&				outDelta)			// The cumulative amount of characters deleted/inserted
{
	outSelection = mSelectionAfter;
	outChangeOffset = outSelection.GetAnchor();
	outChangeLength = 0;
	outDelta = 0;
	
	for (MicroActionList::iterator m = mMicroActions.begin(); m != mMicroActions.end(); ++m)
		(*m).Redo(mBuffer, outChangeOffset, outChangeLength, outDelta);
}

MTextBuffer::MTextBuffer()
	: mData(nil)
	, mPhysicalLength(0)
	, mLogicalLength(0)
	, mGapOffset(0)
{
	string s = Preferences::GetString("default encoding", "utf-8");
	if (s == "utf-16 be")
		mEncoding = kEncodingUTF16BE;
	else if (s == "utf-16 le")
		mEncoding = kEncodingUTF16LE;
	else if (s == "mac os roman")
		mEncoding = kEncodingMacOSRoman;
	else
		mEncoding = kEncodingUTF8;
	
	mBOM = Preferences::GetInteger("add bom", 0);

	s = Preferences::GetString("newline char", "LF");
	if (s == "CR")
		mEOLNKind = eEOLN_MAC;
	else if (s == "CRLF")
		mEOLNKind = eEOLN_DOS;
	else
		mEOLNKind = eEOLN_UNIX;
}

MTextBuffer::MTextBuffer(
	const string&		inText)
	: mData(nil)
	, mPhysicalLength(0)
	, mLogicalLength(0)
	, mGapOffset(0)
{
	mEncoding = kEncodingUTF8;
	mBOM = Preferences::GetInteger("add bom", 0);

	string s = Preferences::GetString("newline char", "LF");
	
	if (s == "CR")
		mEOLNKind = eEOLN_MAC;
	else if (s == "CRLF")
		mEOLNKind = eEOLN_DOS;
	else
		mEOLNKind = eEOLN_UNIX;

	mData = new char[inText.length()];
	memcpy(mData, inText.c_str(), inText.length());
	mPhysicalLength = mLogicalLength = mGapOffset = inText.length();
}

MTextBuffer::~MTextBuffer()
{
	delete[] mData;

	while (mUndoneActions.size())
	{
		delete mUndoneActions.top();
		mUndoneActions.pop();
	}

	while (mDoneActions.size())
	{
		delete mDoneActions.top();
		mDoneActions.pop();
	}
}

void MTextBuffer::ReadFromFile(
	MFile&		inFile)
{
	// first reset the data
	while (mUndoneActions.size())
	{
		delete mUndoneActions.top();
		mUndoneActions.pop();
	}

	while (mDoneActions.size())
	{
		delete mDoneActions.top();
		mDoneActions.pop();
	}

	mLogicalLength = 0;
	mPhysicalLength = 0;
	mGapOffset = 0;
	delete[] mData;
	mData = nil;
	
	// First read the data into a buffer
	
	if (not inFile.IsOpen())
		inFile.Open(O_RDONLY);
	
	int64 len = inFile.GetSize();
	if (len > numeric_limits<uint32>::max())
		THROW(("File too large to open"));
	auto_array<char> data(new char[len]);
	
	inFile.Read(data.get(), len);
	inFile.Close();
	
	// Now find out what this data contains.
	
	mEncoding = kEncodingUnknown;
	mBOM = false;
	
	// 1. easy, see if there is a BOM
	
	char* txt = data.get();
	unsigned char c1 = 0, c2 = 0, c3 = 0;
	
	if (len >= 2)
	{
		c1 = static_cast<unsigned char>(txt[0]);
		c2 = static_cast<unsigned char>(txt[1]);
		
		if (c1 == 0x0fe and c2 == 0x0ff)
		{
			mEncoding = kEncodingUTF16BE;
			txt += 2;
			len -= 2;
			mBOM = true;
		}
		else if (c1 == 0x0ff and c2 == 0x0fe)
		{
			mEncoding = kEncodingUTF16LE;
			txt += 2;
			len -= 2;
			mBOM = true;
		}
	}
	
	if (mEncoding == kEncodingUnknown and len >= 3)
	{
		c3 = static_cast<unsigned char>(txt[2]);
		
		if (c1 == 0x0ef and c2 == 0x0bb and c3 == 0x0bf)
		{
			mEncoding = kEncodingUTF8;
			txt += 3;
			len -= 3;
			mBOM = true;
		}
	}
	
	// 2. No BOM, see if it is still valid UTF8 (prefered) or not
	
	if (mEncoding == kEncodingUnknown)
	{
		bool validUtf8 = true;

		for (uint32 i = 0; i < len and validUtf8; ++i)
		{
			unsigned char c = static_cast<unsigned char>(txt[i]);

			if (c & 0x0080)
			{
				int cLen = 0;
			 
				if ((c & 0x00E0) == 0x00C0)
					cLen = 2;
				else if ((c & 0x00F0) == 0x00E0)
					cLen = 3;
				else if ((c & 0x00F8) == 0x00F0)
					cLen = 4;
				else if ((c & 0x00FC) == 0x00F8)
					cLen = 5;

				if (cLen == 0)
					validUtf8 = false;
				else
				{
					while (cLen-- > 1)
					{
						++i;
						c = static_cast<unsigned char>(txt[i]);
						if ((c & 0x00C0) != 0x0080)
							validUtf8 = false;
					}
				}
			}
		}
		
		if (validUtf8)
			mEncoding = kEncodingUTF8;
		else
			mEncoding = kEncodingMacOSRoman;
	}
	
	if (mEncoding == kEncodingUTF8)
	{
		mData = data.release();

		mLogicalLength = len;
		mGapOffset = 0;
		mPhysicalLength = inFile.GetSize();
	}
	else
	{
		auto_ptr<MDecoder> decoder(MDecoder::GetDecoder(mEncoding, txt, len));
		
		// preserve some memory
		switch (mEncoding)
		{
			case kEncodingUTF16LE:
			case kEncodingUTF16BE:
				reserve(len / 2 + kBlockSize);
				break;
			default:
				reserve(len + kBlockSize);
				break;
		}	
	
		wchar_t uc;
		while (decoder->ReadUnicode(uc))
			push_back(uc);
	}
	
	// now convert the line end character
	
	MoveGapTo(mLogicalLength);
	
	char* src = mData;
	char* dst = mData;
	char* end = mData + mLogicalLength;

	// assume (hope) it is UNIX (LF)	
	mEOLNKind = eEOLN_UNIX;
	
	for (; src != end; ++src)
	{
		if (*src == '\r')
		{
			if (src + 1 < end and *(src + 1) == '\n')
				mEOLNKind = eEOLN_DOS;
			else
				mEOLNKind = eEOLN_MAC;

			break;
		}
		else if (*src == '\n')
			break;
	}
	
	if (mEOLNKind == eEOLN_MAC)
	{
		for (; src != end; ++src)
		{
			if (*src == '\r')
				*src = '\n';
		}
	}
	else if (mEOLNKind == eEOLN_DOS)
	{
		for (src = dst = mData; src < end - 1; ++src, ++dst)
		{
			if (*src == '\r' and *(src + 1) == '\n')
				++src;
			*dst = *src;
		}
		
		mGapOffset = mLogicalLength = dst - mData;
	}
}

void MTextBuffer::WriteToFile(
	MFile&		inFile)
{
	if (not inFile.IsOpen())
		inFile.Open(O_RDWR | O_TRUNC);

	MoveGapTo(mLogicalLength);

	if (mBOM)			// must be a unicode encoding
	{
		switch (mEncoding)
		{
			case kEncodingUTF8:
			{
				const char kBOM[] = { 0xEF, 0xBB, 0xBF };
				inFile.Write(kBOM, 3);
				break;
			}
			
			case kEncodingUTF16BE:
			{
				const char kBOM[] = { 0xFE, 0xFF };
				inFile.Write(kBOM, 2);
				break;
			}
			
			case kEncodingUTF16LE:
			{
				const char kBOM[] = { 0xFF, 0xFE };
				inFile.Write(kBOM, 2);
				break;
			}
			
			default:
				;
		}
	}

	if (mEncoding == kEncodingUTF8)
		inFile.Write(mData, mLogicalLength);
	else
	{
		wc_iterator txt(this, 0);
		auto_ptr<MEncoder> encoder(MEncoder::GetEncoder(mEncoding));
		
		while (txt.GetOffset() < mLogicalLength)
		{
			wchar_t uc = *txt++;
			
			if (uc == '\n' and mEOLNKind != eEOLN_UNIX)
			{
				encoder->WriteUnicode('\r');
				if (mEOLNKind == eEOLN_DOS)
					encoder->WriteUnicode('\n');
			}
			else
				encoder->WriteUnicode(uc);
		}
		
		if (encoder->GetBufferSize())
			inFile.Write(encoder->Peek(), encoder->GetBufferSize());
	}
}

void MTextBuffer::Insert(
	uint32			inPosition,
	const char*		inText,
	uint32			inLength)
{
	assert(inLength != 0);
	if (inLength > 0)
	{
		if (mDoneActions.size() == 0)
			THROW(("No Action defined"));

		Action* a = mDoneActions.top();
		a->Insert(inPosition, inText, inLength);
	}
}

void MTextBuffer::InsertSelf(
	uint32			inPosition,
	const char*		inText,
	uint32			inLength)
{
	if (mData == nil or mLogicalLength + inLength > mPhysicalLength)
	{
		uint32 newLength = ((mLogicalLength + inLength) / kBlockSize + 1) * kBlockSize;
		auto_array<char> tmp(new char[newLength]);
		
		if (mData != nil)
		{
			MoveGapTo(mLogicalLength);
			memcpy(tmp.get(), mData, mLogicalLength);
			delete[] mData;
		}
		
		mData = tmp.release();
		mPhysicalLength = newLength;
	}

	assert(mPhysicalLength - mLogicalLength >= inLength);
	MoveGapTo(inPosition);
	
	memcpy(mData + inPosition, inText, inLength);
	
	mLogicalLength += inLength;
	mGapOffset += inLength;
}

void MTextBuffer::Delete(
	uint32			inPosition,
	uint32			inLength)
{
	assert(inLength != 0);
	if (inLength > 0)
	{
		if (mDoneActions.size() == 0)
			THROW(("No Action defined"));
		
		Action* a = mDoneActions.top();
		a->Delete(inPosition, inLength);
	}
}

void MTextBuffer::DeleteSelf(
	uint32			inPosition,
	uint32			inLength)
{
	assert(inPosition + inLength <= mLogicalLength);
	if (inPosition + inLength > mLogicalLength)
		THROW(("Logic error"));

	MoveGapTo(inPosition + inLength);

	mGapOffset -= inLength;
	mLogicalLength -= inLength;
}

void MTextBuffer::Replace(
	uint32			inPosition,
	const char*		inText,
	uint32			inLength)
{
	Delete(inPosition, inLength);
	Insert(inPosition, inText, inLength);
}	

void MTextBuffer::MoveGapTo(
	uint32			inPosition)
{
	assert(inPosition <= mLogicalLength);
	if (inPosition > mLogicalLength)
		THROW(("Logic error"));
	
	uint32 gapSize = mPhysicalLength - mLogicalLength;
	uint32 gapEnd = mGapOffset + gapSize;
	uint32 src, dst, cnt = 0;
	
	if (mGapOffset < inPosition)
	{
		src = gapEnd;
		dst = mGapOffset;
		cnt = gapSize + inPosition - src;
		if (cnt > mPhysicalLength - gapEnd)
			cnt = mPhysicalLength - gapEnd;
	}
	else if (mGapOffset > inPosition)
	{
		src = inPosition;
		dst = inPosition + gapSize;
		cnt = gapEnd - dst;
	}
	
	if (cnt > 0)
	{
		assert(dst + cnt <= mPhysicalLength);
		memmove(mData + dst, mData + src, cnt);
	}

	mGapOffset = inPosition;
}

void MTextBuffer::GetText(
	uint32			inPosition,
	char*			outText,
	uint32			inLength) const
{
	assert(inPosition + inLength <= mLogicalLength);
	if (inPosition + inLength > mLogicalLength)
		THROW(("Logic error"));
	
	uint32 cnt1 = 0, cnt2 = 0, offset2 = 0;
	
	if (inPosition < mGapOffset)
	{
		cnt1 = mGapOffset - inPosition;
		if (cnt1 > inLength)
			cnt1 = inLength;
	}
	
	if (inPosition + inLength > mGapOffset)
	{
		if (inPosition > mGapOffset)
		{
			cnt2 = inLength;
			offset2 = inPosition + mPhysicalLength - mLogicalLength;
		}
		else
		{
			cnt2 = inLength - (mGapOffset - inPosition);
			offset2 = mGapOffset + mPhysicalLength - mLogicalLength;
		}
	}
	
	if (cnt1 > 0)
		memcpy(outText, mData + inPosition, cnt1);
	
	if (cnt2 > 0)
		memcpy(outText + cnt1, mData + offset2, cnt2);
}

void MTextBuffer::GetText(
	uint32			inPosition,
	uint32			inLength,
	string&			outText) const
{
	assert(inPosition + inLength <= mLogicalLength);
	if (inPosition + inLength > mLogicalLength)
		THROW(("Logic error"));
	
	uint32 cnt1 = 0, cnt2 = 0, offset2 = 0;
	
	if (inPosition < mGapOffset)
	{
		cnt1 = mGapOffset - inPosition;
		if (cnt1 > inLength)
			cnt1 = inLength;
	}
	
	if (inPosition + inLength > mGapOffset)
	{
		if (inPosition > mGapOffset)
		{
			cnt2 = inLength;
			offset2 = inPosition + mPhysicalLength - mLogicalLength;
		}
		else
		{
			cnt2 = inLength - (mGapOffset - inPosition);
			offset2 = mGapOffset + mPhysicalLength - mLogicalLength;
		}
	}
	
	if (cnt1 > 0)
		outText.assign(mData + inPosition, mData + inPosition + cnt1);
	else
		outText.clear();
	
	if (cnt2 > 0)
		outText.append(mData + offset2, mData + offset2 + cnt2);
}

uint32 MTextBuffer::NextCursorPosition(
	uint32			inOffset,
	CursorMovement	inMovement) const
{
	const int8 kWordBreakStateTableForKeyboard[5][10] = {
		//	CR	LF	Sep	Tab	Let	Com	Hir	Kat	Han	Other		State
		{	-1,	-1,	 0,	 0,	 1,	 1,	 2,	 3,	 4,	 0	},	//	0
		{	-1,	-1,	-1,	-1,	 1,	 1,	-1,	-1,	-1,	-1	},	//	1
		{	-1,	-1,	-1,	-1,	-1,	 2,	 2,	-1,	-1,	-1	},	//	2
		{	-1,	-1,	-1,	-1,	-1,	 3,	 2,	 3,	-1,	-1	},	//	3
		{	-1,	-1,	-1,	-1,	-1,	 4,	 2,	-1,	 4,	-1	},	//	4
	};

	const int8 kWordBreakStateTableForMouse[8][10] = {
		//	CR	LF	Sep	Tab	Let	Com	Hir	Kat	Han	Other		State
		{	 7,	 7,	 6,	 5,	 4,	 4,	 2,	 3,	 1,	 6	},	//	0
		{	-1,	-1,	-1,	-1,	-1,	 1,	 2,	-1,	 1,	-1	},	//	1
		{	-1,	-1,	-1,	-1,	-1,	 2,	 2,	-1,	-1,	-1	},	//	2
		{	-1,	-1,	-1,	-1,	-1,	 3,	 2,	 3,	-1,	-1	},	//	3
		{	-1,	-1,	-1,	-1,	 4,	 4,	-1,	-1,	-1,	-1	},	//	4
		{	-1,	-1,	-1,	 5,	-1,	-1,	-1,	-1,	-1,	-1	},	//	5
		{	-1,	-1,	-1,	 6,	-1,	-1,	-1,	-1,	-1,	-1	},	//	6
		{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1	},	//	7
	};

	uint32 result = inOffset;

	if (inOffset + 1 >= mLogicalLength)
		result = mLogicalLength;
	else
	{
		wc_iterator c(this, inOffset);

		if (inMovement == eMoveOneCharacter)
		{
			CharBreakClass c1 = GetCharBreakClass(*c);

		    do
		    {
				++c;
				result = c.GetOffset();
				CharBreakClass c2 = GetCharBreakClass(*c);
				
				if (kCharBreakTable[c1][c2])
					break;
				
				c1 = c2;
			}
			while (result < mLogicalLength);
		}
		else if (inMovement == eMoveOneWordForKeyboard)
		{
		    int8 state = 0;
			while (state >= 0)
			{
				result = c.GetOffset();
				
				if (result >= mLogicalLength)
					break;
				
				wchar_t unicode = *c;
				++c;

				WordBreakClass cl = GetWordBreakClass(unicode);
				state = kWordBreakStateTableForKeyboard[uint8(state)][cl];
			}
		}
		else
		{
		    int8 state = 0;
			while (state >= 0)
			{
				result = c.GetOffset();
				
				if (result >= mLogicalLength)
					break;
				
				wchar_t unicode = *c;
				++c;

				WordBreakClass cl = GetWordBreakClass(unicode);
				state = kWordBreakStateTableForMouse[uint8(state)][cl];
			}
		}
	}
	
	return result;
}

uint32 MTextBuffer::PreviousCursorPosition(
	uint32			inOffset,
	CursorMovement	inMovement) const
{
	const int8 kWordBreakStateTableForKeyboard[6][10] = {
		//	CR	LF	Sep	Tab	Let	Com	Hir	Kat	Han	Other		State
		{	-1,	-1,	 0,	 0,	 1,	 2,	 3,	 4,	 5,	 0	},	//	0
		{	-1,	-1,	-1,	-1,	 2,	 1,	 3,	 4,	 5,	-1	},	//	1
		{	-1,	-1,	-1,	-1,	 2,	 2,	-1,	-1,	-1,	-1	},	//	2
		{	-1,	-1,	-1,	-1,	-1,	 3,	 3,	 4,	 5,	-1	},	//	3
		{	-1,	-1,	-1,	-1,	-1,	 4,	-1,	 4,	-1,	-1	},	//	4
		{	-1,	-1,	-1,	-1,	-1,	 5,	-1,	-1,	 5,	-1	},	//	5
	};

//	const int8 kWordBreakStateTableForMouse[8][10] = {
//		//	CR	LF	Sep	Tab	Let	Com	Hir	Kat	Han	Other		State
//		{	-1,	-1,	 0,	 0,	 1,	 2,	 3,	 4,	 5,	 0	},	//	0
//		{	-1,	-1,	-1,	-1,	 2,	 1,	 3,	 4,	 5,	-1	},	//	1
//		{	-1,	-1,	-1,	-1,	 2,	 2,	-1,	-1,	-1,	-1	},	//	2
//		{	-1,	-1,	-1,	-1,	-1,	 3,	 3,	 4,	 5,	-1	},	//	3
//		{	-1,	-1,	-1,	-1,	-1,	 4,	-1,	 4,	-1,	-1	},	//	4
//		{	-1,	-1,	-1,	-1,	-1,	 5,	-1,	-1,	 5,	-1	},	//	5
//	};

	uint32 result = inOffset;

	if (inOffset <= 1)
		result = 0;
	else
	{
		wc_iterator c(this, inOffset);

		if (inMovement == eMoveOneCharacter)
		{
			CharBreakClass c1 = GetCharBreakClass(*c);

		    do
		    {
		    	--c;
		    	result = c.GetOffset();
		    	
				CharBreakClass c2 = GetCharBreakClass(*c);

				if (kCharBreakTable[c1][c2])
					break;
				
				c1 = c2;
			}
			while (result > 0);
		}
		else// if (inMovement == eMoveOneWordForKeyboard)
		{
		    int8 state = 0;
			while (state >= 0)
			{
				result = c.GetOffset();
				
				if (result == 0)
					break;
				
				--c;
				wchar_t unicode = *c;

				WordBreakClass cl = GetWordBreakClass(unicode);
				state = kWordBreakStateTableForKeyboard[uint8(state)][cl];
			}
		}
/*		else
		{
		    int8 state = 0;
			while (result < mLogicalLength and state >= 0)
			{
				result = c.GetOffset();
				wchar_t unicode = *c;
				++c;

				WordBreakClass cl = GetWordBreakClass(unicode);
				state = kWordBreakStateTableForMouse[state][cl];
			}
		}
*/	}
	
	return result;
}

// -----------------------------------------------------------------------------

void
MTextBuffer::StartAction(
	const string&		inAction,
	const MSelection&	inSelection)
{
	mDoneActions.push(new Action(*this, inAction));
	mDoneActions.top()->SetSelectionBefore(inSelection);
	
	while (mUndoneActions.size())
	{
		delete mUndoneActions.top();
		mUndoneActions.pop();
	}
}

void
MTextBuffer::SetSelectionBefore(
	const MSelection&	inSelection)
{
	if (mDoneActions.size() == 0)
		THROW(("No Action defined"));
	mDoneActions.top()->SetSelectionBefore(inSelection);
}

MSelection
MTextBuffer::GetSelectionBefore() const
{
	if (mDoneActions.size() == 0)
		THROW(("No Action defined"));
	return mDoneActions.top()->GetSelectionBefore();
}

void
MTextBuffer::SetSelectionAfter(
	const MSelection&	inSelection)
{
	if (mDoneActions.size() == 0)
		THROW(("No Action defined"));
	mDoneActions.top()->SetSelectionAfter(inSelection);
}

MSelection
MTextBuffer::GetSelectionAfter() const
{
	if (mDoneActions.size() == 0)
		THROW(("No Action defined"));
	return mDoneActions.top()->GetSelectionAfter();
}

bool
MTextBuffer::CanUndo(
	string&		outAction)
{
	bool result = false;
	if (mDoneActions.size())
	{
		result = true;
		outAction = mDoneActions.top()->GetTitle();
	}
	return result;
}

bool
MTextBuffer::CanRedo(
	string&		outAction)
{
	bool result = false;
	if (mUndoneActions.size())
	{
		result = true;
		outAction = mUndoneActions.top()->GetTitle();
	}
	return result;
}

void
MTextBuffer::Undo(
	MSelection&	outSelection,
	uint32&		outChangeOffset,	// The minimal offset of text touched by this Undo
	uint32&		outChangeLength,	// Total length of area touched by Undo
	int32&		outDelta)		// The cumulative amount of characters deleted/inserted
{
	if (mDoneActions.size() == 0)
		THROW(("No Action defined"));

	Action* a = mDoneActions.top();
	a->Undo(outSelection, outChangeOffset, outChangeLength, outDelta);
	mDoneActions.pop();
	mUndoneActions.push(a);
}

void
MTextBuffer::Redo(
	MSelection&	outSelection,
	uint32&		outChangeOffset,	// The minimal offset of text touched by this Undo
	uint32&		outChangeLength,	// Total length of area touched by Undo
	int32&		outDelta)		// The cumulative amount of characters deleted/inserted
{
	if (mUndoneActions.size() == 0)
		THROW(("No Action defined"));

	Action* a = mUndoneActions.top();
	a->Redo(outSelection, outChangeOffset, outChangeLength, outDelta);
	mUndoneActions.pop();
	mDoneActions.push(a);
}

// -----------------------------------------------------------------------------
// Find

void
MTextBuffer::InitSkip(
	const char*		inPattern,
	uint32			inPatternLength,
	bool			inIgnoreCase,
	MDirection		inDirection,
	Skip&			ioSkip) const
{
	int32 M = inPatternLength, i;
	
	for (i = 0; i < 256; ++i)
		ioSkip[i] = M;

	if (inDirection == kDirectionForward)
	{
		for (i = 0; i < M; ++i)
		{
			uint8 c = static_cast<uint8>(inPattern[i]);
			
			if (inIgnoreCase)
				c = toupper(c);
			
			ioSkip[c] = M - i - 1;
		}
	}
	else
	{
		for (i = M - 1; i >= 0; --i)
		{
			uint8 c = static_cast<uint8>(inPattern[i]);
			
			if (inIgnoreCase)
				c = toupper(c);
			
			ioSkip[c] = i;
		}
	}
}

bool
MTextBuffer::MismatchSearch(
	const char*		inPattern,
	uint32			inPatternLength,
	uint32&			ioOffset,
	bool			inIgnoreCase,
	MDirection		inDirection,
	const Skip&		inSkip) const
{
	int32 i, j, t;
	int32 M = inPatternLength;
	bool result = true;

	if (inDirection == kDirectionForward)
	{
		for (i = ioOffset + M - 1, j = M - 1; result and j >= 0; i--, j--)
		{
			if (i >= static_cast<int32>(mLogicalLength))
			{
				result = false;
				break;
			}
			
			for (;;)
			{
				uint8 p = static_cast<uint8>(inPattern[j]);
				uint8 a = static_cast<uint8>(GetChar(i));
				
				if (inIgnoreCase)
				{
					p = toupper(p);
					a = toupper(a);
				}
				
				if (a == p)
					break;
			
				t = inSkip[a];
				
				i += (M - j > t) ? M - j : t;
				if (i >= static_cast<int32>(mLogicalLength))
				{
					result = false;
					break;
				}
	
				j = M - 1;
			}
		}
	
		if (result)
			ioOffset = i + 1;
	}
	else
	{
		for (i = ioOffset - M + 1, j = 0; result and j < M; ++i, ++j)
		{
			if (i < 0)
			{
				result = false;
				break;
			}
			
			for (;;)
			{
				uint8 p = static_cast<uint8>(inPattern[j]);
				uint8 a = static_cast<uint8>(GetChar(i));
				
				if (inIgnoreCase)
				{
					p = toupper(p);
					a = toupper(a);
				}
				
				if (a == p)
					break;
			
				t = inSkip[a];
				
				i -= (j + 1 > t) ? j + 1 : t;
				if (i < 0)
				{
					result = false;
					break;
				}
	
				j = 0;
			}
		}
	
		if (result)
			ioOffset = i - M;
	}

	return result;
}

bool MTextBuffer::Find(
	uint32			inOffset,
	string			inWhat,
	MDirection		inDirection,
	bool			inIgnoreCase,
	bool			inRegex,
	MSelection&		outFound)
{
	bool result = false;
	
	if (inRegex)
	{
		MoveGapTo(mLogicalLength);
		
		const char* errmsg;
		int errcode, erroffset;
		
		if (inWhat.length() == 0)
			THROW(("Regular expression is too short"));
		
		int options = PCRE_MULTILINE | PCRE_UTF8;
		
		if (inIgnoreCase)
			options |= PCRE_CASELESS;
		
		pcre* pattern = nil;
		pcre_extra* info = nil;
		
		try
		{
			pattern = pcre_compile2(inWhat.c_str(),
				options, &errcode, &errmsg, &erroffset, nil);
			
			if (pattern == nil or errcode != 0)
				THROW(("Error compiling regular expression: %s", kPCRE_ERR_STR[errcode]));
			
			info = pcre_study(pattern, 0, &errmsg);
			if (errmsg != nil)
				THROW(("Error studying compiled regular expression: %s", errmsg));
			
			int matches[33] = {};
			
			if (inDirection == kDirectionForward)
			{
				options = 0;
				
				int r = pcre_exec(pattern, info, mData,
					mLogicalLength, inOffset, options, matches, 33);
				
				if (r >= 0)
				{
					outFound.Set(matches[0], matches[1]);
					result = true;
				}
			}
			else
			{
				int firstchar;
				const unsigned char* firstTable;

				int r = pcre_fullinfo(pattern, info, PCRE_INFO_FIRSTCHAR, &firstchar);
				if (r != 0 or firstchar < -1)
					r = pcre_fullinfo(pattern, info, PCRE_INFO_FIRSTTABLE, &firstTable);
				
				if (inIgnoreCase and firstchar != 0)
					firstchar = toupper(firstchar);

				while (result == false and inOffset > 0)
				{
					inOffset -= GetPrevCharLength(inOffset);
					
					if (inOffset > mLogicalLength)
						break;

					bool trymatch = true;
					unsigned char ch = static_cast<unsigned char>(mData[inOffset]);
					
					if (firstchar >= 0)
					{
						if (inIgnoreCase)
							ch = toupper(ch);
						trymatch = ch == firstchar;
					}
					else if (firstchar == -1)
						trymatch = inOffset == 0 or mData[inOffset - 1] == '\n';
					else if (firstTable)
						trymatch = (firstTable[ch / 8] & (1 << (ch % 8))) != 0;
					
					if (trymatch)
					{
						r = pcre_exec(pattern, info, mData, mLogicalLength, inOffset,
							PCRE_ANCHORED, matches, 33);
						
						if (r >= 0)
						{
							outFound.Set(matches[0], matches[1]);
							result = true;
						}
					}
				}
			}
		}
		catch (...)
		{
			if (pattern != nil)
				free(pattern);
			
			if (info != nil)
				free(info);
			
			throw;
		}
	}
	else
	{
		Skip skip;
		
		InitSkip(inWhat.c_str(), inWhat.length(), inIgnoreCase, inDirection, skip);
		
		uint32 offset = inOffset;
		result = MismatchSearch(inWhat.c_str(), inWhat.length(), offset, inIgnoreCase, inDirection, skip);

		if (result)
			outFound.Set(offset, offset + inWhat.length());
	}
	
	return result;
}

bool MTextBuffer::CanReplace(
	string			inWhat,
	bool			inRegex,
	bool			inIgnoreCase,
	MSelection		inSelection)
{
	bool result = false;

	if (inRegex)
	{
		MoveGapTo(mLogicalLength);
		
		int32 a = inSelection.GetMinOffset();
		int32 c = inSelection.GetMaxOffset();

		const char* errmsg;
		int errcode, erroffset;
		
		if (inWhat.length() == 0)
			THROW(("Regular expression is too short"));
		
		int options = PCRE_MULTILINE | PCRE_UTF8;
		
		if (inIgnoreCase)
			options |= PCRE_CASELESS;
		
		pcre* pattern = nil;
		pcre_extra* info = nil;
		
		try
		{
			pattern = pcre_compile2(inWhat.c_str(),
				options, &errcode, &errmsg, &erroffset, nil);
			
			if (pattern == nil or errcode != 0)
				THROW(("Error compiling regular expression: %s", kPCRE_ERR_STR[errcode]));
			
			info = pcre_study(pattern, 0, &errmsg);
			if (errmsg != nil)
				THROW(("Error studying compiled regular expression: %s", errmsg));
			
			int matches[33] = {};
			
			int r = pcre_exec(pattern, info, mData, mLogicalLength, a, PCRE_ANCHORED, matches, 33);
			
			if (r >= 0 and matches[0] == a and matches[1] == c)
				result = true;
		}
		catch (...)
		{
			if (pattern != nil)
				free(pattern);
			
			if (info != nil)
				free(info);
			
			throw;
		}
	}
	else
	{
		string s;
		GetText(inSelection.GetMinOffset(),
			inSelection.GetMaxOffset() - inSelection.GetMinOffset(), s);

		if (inIgnoreCase)
			result = tolower(inWhat) == tolower(s);
		else
			result = inWhat == s;
	}
	
	return result;
}

void MTextBuffer::ReplaceExpression(
	MSelection		inSelection,
	string			inExpression,
	bool			inIgnoreCase,
	string			inFormat,
	string&			outReplacement)
{
	MoveGapTo(mLogicalLength);
	
	int32 a = inSelection.GetMinOffset();
	int32 c = inSelection.GetMaxOffset();

	const char* errmsg;
	int errcode, erroffset;
	
	if (inExpression.length() == 0)
		THROW(("Regular expression is too short"));
	
	int options = PCRE_MULTILINE | PCRE_UTF8;
	
	if (inIgnoreCase)
		options |= PCRE_CASELESS;
	
	pcre* pattern = nil;
	pcre_extra* info = nil;
	
	try
	{
		pattern = pcre_compile2(inExpression.c_str(),
			options, &errcode, &errmsg, &erroffset, nil);
		
		if (pattern == nil or errcode != 0)
			THROW(("Error compiling regular expression: %s", kPCRE_ERR_STR[errcode]));
		
		info = pcre_study(pattern, 0, &errmsg);
		if (errmsg != nil)
			THROW(("Error studying compiled regular expression: %s", errmsg));
		
		int matches[33] = {};
		
		int r = pcre_exec(pattern, info, mData, mLogicalLength, a, PCRE_ANCHORED, matches, 33);
		
		if (r >= 0 and matches[0] == a and matches[1] == c)
		{
			string result, s;
			
			for (string::iterator ch = outReplacement.begin(); ch != outReplacement.end(); ++ch)
			{
				switch (*ch)
				{
					case '$':
						++ch;
						switch (*ch)
						{
							case '$':
								result += '$';
								break;
							
							case '&':
							{
								GetText(a, c - a, s);
								result += s;
								break;
							}
							
							default:
								if (isdigit(*ch))
								{
									uint32 m = *ch - '0';
									GetText(matches[2 * m], matches[2 * m + 1] - matches[2 * m], s);
									result += s;
								}
								else
								{
									result += '$';
									result += *ch;
								}
								break;
						}
						break;
					
					case '\\':
						++ch;
						switch (*ch)
						{
							case 'n':
								result += '\n';
								break;

							case 'r':
								result += '\r';
								break;

							case 't':
								result += '\t';
								break;
							
							default:
								result += *ch;
								break;
						}
						break;
					
					default:
						result += *ch;
				}
			}
			
			outReplacement = result;
		}
	}
	catch (...)
	{
		if (pattern != nil)
			free(pattern);
		
		if (info != nil)
			free(info);
		
		throw;
	}
}

void
MTextBuffer::CollectWordsBeginningWith(
	uint32					inFromOffset,
	MDirection				inDirection,
	string					inPattern,
	vector<string>&			ioStrings)
{
	set<string> keys;
	
	for (vector<string>::iterator k = ioStrings.begin(); k != ioStrings.end(); ++k)
		keys.insert(*k);
	
	uint32 offset = inFromOffset;
	bool wrapped = false;

	for (;;)
	{
		if (inDirection == kDirectionBackward)
		{
			if (offset >= inPattern.length())
				offset -= inPattern.length();
			else if (not wrapped)
			{
				wrapped = true;
				offset = mLogicalLength - inPattern.length();
			}
			else
				break;
		}
		
		MSelection found;
		if (not Find(offset, inPattern, inDirection, false, false, found))
		{
			if (not wrapped)
			{
				wrapped = true;
				if (inDirection == kDirectionForward and inFromOffset != 0)
				{
					offset = 0;
					continue;
				}
				else if (inDirection == kDirectionBackward)
				{
					offset = mLogicalLength - inPattern.length();
					continue;
				}
			}
			break;
		}
		
		offset = found.GetMinOffset();

		uint32 end = NextCursorPosition(offset, eMoveOneWordForKeyboard);
		uint32 start = PreviousCursorPosition(end, eMoveOneWordForKeyboard);
		
		if (wrapped and
			((inDirection == kDirectionForward and end > inFromOffset) or
			 (inDirection == kDirectionBackward and start <= inFromOffset)))
		{
			break;
		}

		if (start == offset and end - start > inPattern.length() and
			(end < inFromOffset or start > inFromOffset))
		{
			string k;
			GetText(start + inPattern.length(), end - start - inPattern.length(), k);

			if (keys.find(k) == keys.end())	// we found a word!
			{
				ioStrings.push_back(k);
				keys.insert(k);
			}
		}

		if (inDirection == kDirectionForward)
			offset = end;
	}
}

uint32 MTextBuffer::GetNextCharLength(
	uint32		inOffset) const
{
	uint32 result = 1;
	
	if (inOffset < mLogicalLength - 1 and
		(GetChar(inOffset) & 0x0080) != 0)
	{
		uint32 r = 1;
		
		if ((GetChar(inOffset) & 0x0E0) == 0x0C0)
			r = 2;
		else if ((GetChar(inOffset) & 0x0F0) == 0x0E0)
			r = 3;
		else if ((GetChar(inOffset) & 0x0F8) == 0x0F0)
			r = 4;
		
		if (r + inOffset >= mLogicalLength)
			r = mLogicalLength - inOffset - 1;
		
		++inOffset;
		
		for (uint32 i = 1; (GetChar(inOffset) & 0x0c0) == 0x080 and i < r; ++i, ++inOffset)
			++result;
	}
	
	return result;
}

uint32 MTextBuffer::GetPrevCharLength(
	uint32		inOffset) const
{
	uint32 result = 0;
	
	if (inOffset > 0)
	{
		result = 1;
		--inOffset;
		
		if ((GetChar(inOffset) & 0x0080) != 0)
		{
			result = 2;
			--inOffset;
	
			while ((GetChar(inOffset) & 0x00C0) == 0x0080 and result < 6)
			{
				result++;
				--inOffset;
			}
		
			switch (result)
			{
				case 2:	if ((GetChar(inOffset) & 0x00E0) != 0x00C0) result = 1; break;
				case 3:	if ((GetChar(inOffset) & 0x00F0) != 0x00E0) result = 1; break;
				case 4:	if ((GetChar(inOffset) & 0x00F8) != 0x00F0) result = 1; break;
				case 5:	if ((GetChar(inOffset) & 0x00FC) != 0x00F8) result = 1; break;
				case 6:	if ((GetChar(inOffset) & 0x00FE) != 0x00FC) result = 1; break;
				default:											result = 1; break;
			}
		}
	}
	
	return result;
}

wchar_t MTextBuffer::GetWChar(
	uint32		inOffset) const
{
	wchar_t result = 0x0FFFD;
	uint32 length = GetNextCharLength(inOffset);
	
	switch (length)
	{
		case 1:	if ((GetChar(inOffset) & 0x0080) == 0)		result = GetChar(inOffset++);					break;
		case 2:	if ((GetChar(inOffset) & 0x00E0) == 0x00C0)	result = (GetChar(inOffset++) & 0x001F) <<  6;	break;
		case 3: if ((GetChar(inOffset) & 0x00E0) == 0x00C0) result = (GetChar(inOffset++) & 0x000F) << 12;	break;
		case 4: if ((GetChar(inOffset) & 0x00E0) == 0x00C0) result = (GetChar(inOffset++) & 0x0007) << 18;	break;
		case 5: if ((GetChar(inOffset) & 0x00E0) == 0x00C0) result = (GetChar(inOffset++) & 0x0003) << 24;	break;
		case 6: if ((GetChar(inOffset) & 0x00E0) == 0x00C0) result = (GetChar(inOffset++) & 0x0001) << 30;	break;
	}
	
	if (result != 0xFFFD)
	{
		++inOffset;
		switch (length)
		{
			case 6:	result |= (GetChar(inOffset++) & 0x003F) << 24;
			case 5:	result |= (GetChar(inOffset++) & 0x003F) << 18;
			case 4:	result |= (GetChar(inOffset++) & 0x003F) << 12;
			case 3:	result |= (GetChar(inOffset++) & 0x003F) <<  6;
			case 2:	result |= (GetChar(inOffset++) & 0x003F);
		}
	}
	
	return result;
}

void MTextBuffer::reserve(
	uint32		inSize)
{
	if (inSize > mPhysicalLength)
	{
		auto_array<char> tmp(new char[inSize]);
		
		if (mData != nil)
		{
			MoveGapTo(mLogicalLength);
			memcpy(tmp.get(), mData, mLogicalLength);
			delete[] mData;
		}
		
		mData = tmp.release();
		mPhysicalLength = inSize;
	}
}

bool MTextBuffer::const_iterator::operator==(
	const char*		inText) const
{
	bool result = true;
	uint32 offset = mOffset;
	
	while (result == true and *inText != 0 and offset < mBuffer->GetSize())
		result = *inText++ == mBuffer->GetChar(offset++);
	
	result = result and *inText == 0;

	return result;
}

void MTextBuffer::SetEOLNKind(
	EOLNKind		inKind)
{
	mEOLNKind = inKind;
}

void MTextBuffer::SetEncoding(
	MEncoding		inEncoding)
{
	mEncoding = inEncoding;
}
