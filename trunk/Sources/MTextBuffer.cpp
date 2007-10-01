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

#include <boost/regex.hpp>

#include "MTextBuffer.h"
#include "MUnicode.h"
#include "MSelection.h"
#include "MError.h"
#include "MFile.h"
#include "MPreferences.h"
#include "MMessageWindow.h"

using namespace std;

namespace
{

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

//	wc_iterator	wc_begin() const							{ return wc_iterator(this, 0); }
//	wc_iterator	wc_end() const								{ return wc_iterator(this, mLogicalLength); }
	


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
	
	mBOM = Preferences::GetInteger("add bom", 1);

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
	mBOM = Preferences::GetInteger("add bom", 1);

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
		mLogicalLength = mPhysicalLength = mGapOffset = len;
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
		inFile.Open(O_RDWR);

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
	ustring&		outText) const
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

uint32 MTextBuffer::NextCursorPosition(uint32 inOffset, CursorMovement inMovement) const
{
	const SInt8 kWordBreakStateTableForKeyboard[5][10] = {
		//	CR	LF	Sep	Tab	Let	Com	Hir	Kat	Han	Other		State
		{	-1,	-1,	 0,	 0,	 1,	 1,	 2,	 3,	 4,	 0	},	//	0
		{	-1,	-1,	-1,	-1,	 1,	 1,	-1,	-1,	-1,	-1	},	//	1
		{	-1,	-1,	-1,	-1,	-1,	 2,	 2,	-1,	-1,	-1	},	//	2
		{	-1,	-1,	-1,	-1,	-1,	 3,	 2,	 3,	-1,	-1	},	//	3
		{	-1,	-1,	-1,	-1,	-1,	 4,	 2,	-1,	 4,	-1	},	//	4
	};

	const SInt8 kWordBreakStateTableForMouse[8][10] = {
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
		const_iterator c(this, inOffset);

		if (inMovement == eMoveOneCharacter)
		{
			CharBreakClass c1 = GetCharBreakClass(GetChar(result));

		    do
		    {
				++result;
				CharBreakClass c2 = GetCharBreakClass(GetChar(result));
				
				if (kCharBreakTable[c1][c2])
					break;
				
				c1 = c2;
			}
			while (result < mLogicalLength);
		}
		else if (inMovement == eMoveOneWordForKeyboard)
		{
		    SInt8 state = 0;
			while (state >= 0)
			{
				result = c.GetOffset();
				
				if (result >= mLogicalLength)
					break;
				
				UniChar unicode = *c;
				++c;

				WordBreakClass cl = GetWordBreakClass(unicode);
				state = kWordBreakStateTableForKeyboard[state][cl];
			}
		}
		else
		{
		    SInt8 state = 0;
			while (state >= 0)
			{
				result = c.GetOffset();
				
				if (result >= mLogicalLength)
					break;
				
				UniChar unicode = *c;
				++c;

				WordBreakClass cl = GetWordBreakClass(unicode);
				state = kWordBreakStateTableForMouse[state][cl];
			}
		}
	}
	
	return result;
}

uint32 MTextBuffer::PreviousCursorPosition(uint32 inOffset, CursorMovement inMovement) const
{
	const SInt8 kWordBreakStateTableForKeyboard[6][10] = {
		//	CR	LF	Sep	Tab	Let	Com	Hir	Kat	Han	Other		State
		{	-1,	-1,	 0,	 0,	 1,	 2,	 3,	 4,	 5,	 0	},	//	0
		{	-1,	-1,	-1,	-1,	 2,	 1,	 3,	 4,	 5,	-1	},	//	1
		{	-1,	-1,	-1,	-1,	 2,	 2,	-1,	-1,	-1,	-1	},	//	2
		{	-1,	-1,	-1,	-1,	-1,	 3,	 3,	 4,	 5,	-1	},	//	3
		{	-1,	-1,	-1,	-1,	-1,	 4,	-1,	 4,	-1,	-1	},	//	4
		{	-1,	-1,	-1,	-1,	-1,	 5,	-1,	-1,	 5,	-1	},	//	5
	};

//	const SInt8 kWordBreakStateTableForMouse[8][10] = {
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
		const_iterator c(this, inOffset);

		if (inMovement == eMoveOneCharacter)
		{
			CharBreakClass c1 = GetCharBreakClass(GetChar(result));

		    do
		    {
				--result;
				CharBreakClass c2 = GetCharBreakClass(GetChar(result));
				
				if (kCharBreakTable[c1][c2])
					break;
				
				c1 = c2;
			}
			while (result > 0);
		}
		else// if (inMovement == eMoveOneWordForKeyboard)
		{
		    SInt8 state = 0;
			while (state >= 0)
			{
				result = c.GetOffset();
				
				if (result == 0)
					break;
				
				--c;
				UniChar unicode = *c;

				WordBreakClass cl = GetWordBreakClass(unicode);
				state = kWordBreakStateTableForKeyboard[state][cl];
			}
		}
/*		else
		{
		    SInt8 state = 0;
			while (result < mLogicalLength and state >= 0)
			{
				result = c.GetOffset();
				UniChar unicode = *c;
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
MTextBuffer::StartAction(const string& inAction, const MSelection& inSelection)
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
MTextBuffer::CanUndo(string& outAction)
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
MTextBuffer::CanRedo(string& outAction)
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

// in order to reduce the skip table size we skip only on the even and uneven bytes
// of a UniChar...


void
MTextBuffer::InitSkip(
	const UniChar*	inPattern,
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
			UniChar c = inPattern[i];
			
			if (inIgnoreCase)
				c = ToUpper(c);
			
			unsigned char hiByte = static_cast<unsigned char>((c >> 8) & 0x00ff);
			unsigned char loByte = static_cast<unsigned char>(c & 0x00ff);
			
			ioSkip[hiByte] = M - i - 1;
			ioSkip[loByte] = M - i - 1;
		}
	}
	else
	{
		for (i = M - 1; i >= 0; --i)
		{
			UniChar c = inPattern[i];
			
			if (inIgnoreCase)
				c = ToUpper(c);
			
			unsigned char hiByte = static_cast<unsigned char>((c >> 8) & 0x00ff);
			unsigned char loByte = static_cast<unsigned char>(c & 0x00ff);
			
			ioSkip[hiByte] = i;
			ioSkip[loByte] = i;
		}
	}
}

bool
MTextBuffer::MismatchSearch(
	const UniChar*	inPattern,
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
				UniChar p = inPattern[j];
				UniChar a = GetChar(i);
				
				if (inIgnoreCase)
				{
					p = ToUpper(p);
					a = ToUpper(a);
				}
				
				if (a == p)
					break;
			
				unsigned char hiByte = static_cast<unsigned char>((a >> 8) & 0x00ff);
				unsigned char loByte = static_cast<unsigned char>(a & 0x00ff);
				
				t = inSkip[hiByte];
				if (inSkip[loByte] > static_cast<uint32>(t))
					t = inSkip[loByte];
				
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
				UniChar p = inPattern[j];
				UniChar a = GetChar(i);
				
				if (inIgnoreCase)
				{
					p = ToUpper(p);
					a = ToUpper(a);
				}
				
				if (a == p)
					break;
			
				unsigned char hiByte = static_cast<unsigned char>((a >> 8) & 0x00ff);
				unsigned char loByte = static_cast<unsigned char>(a & 0x00ff);
				
				t = inSkip[hiByte];
				if (inSkip[loByte] > static_cast<uint32>(t))
					t = inSkip[loByte];
				
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
	ustring			inWhat,
	MDirection		inDirection,
	bool			inIgnoreCase,
	bool			inRegex,
	MSelection&		outFound) const
{
	bool result = false;
	
	if (inRegex)
	{
		wstring expr;
		Convert(inWhat, expr);

		boost::regex::flag_type flags = boost::regex_constants::normal;
		
		if (inIgnoreCase)
			flags |= boost::regex_constants::icase;
		
		boost::wregex re(expr, flags);
		boost::match_results<wc_iterator> m;

		boost::regex_constants::match_flag_type match_flags =
			boost::regex_constants::match_not_dot_newline |
			boost::regex_constants::match_not_dot_null;
		
		wc_iterator start = wc_iterator(this, inOffset);
		wc_iterator wc_end = wc_iterator(this, mLogicalLength);

		if (inDirection == kDirectionForward)
		{
			if (inOffset > 0)
			{
				match_flags |= boost::regex_constants::match_not_bob;
			
				if (GetChar(inOffset - 1) != '\n')
					match_flags |= boost::regex_constants::match_not_bol;
			}
			
			if (boost::regex_search(start, wc_end, m, re, match_flags) and m[0].matched)
			{
				result = true;
				
				wc_iterator begin = m[0].first;
				wc_iterator end = m[0].second;
				
				// special case, never match an empty string
				// if the result is the same offset as the start
				
				if (begin == end and begin == start)
				{
					if (inOffset == mLogicalLength)
						result = false;
					else
					{
						++start;
						if (boost::regex_search(start, wc_end, m, re, match_flags) and
							m[0].matched)
						{
							begin = m[0].first;
							end = m[0].second;
						}
						else
							result = false;
					}
				}
				
				outFound.Set(begin.GetOffset(), end.GetOffset());
			}
		}
		else
		{
			match_flags |= boost::regex_constants::match_continuous;

			while (result == false and start.GetOffset() > 0)
			{
				--start;

				if (start.GetOffset() > 0)
				{
					match_flags |= boost::regex_constants::match_not_bob;
				
					if (GetChar(start.GetOffset() - 1) != '\n')
						match_flags |= boost::regex_constants::match_not_bol;
					else
						match_flags &= ~boost::regex_constants::match_not_bol;
				}
				else
				{
					match_flags &= ~boost::regex_constants::match_not_bob;
					match_flags &= ~boost::regex_constants::match_not_bol;
				}
				
				if (boost::regex_search(start, wc_end, m, re, match_flags) and m[0].matched)
				{
					result = true;

					wc_iterator begin = m[0].first;
					wc_iterator end = m[0].second;
					
					outFound.Set(begin.GetOffset(), end.GetOffset());
				}
			}
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

bool MTextBuffer::CanReplace(ustring inWhat, bool inRegex,
	bool inIgnoreCase, MSelection inSelection) const
{
	bool result;
	
	if (inRegex)
	{
		uint32 offset = inSelection.GetMinOffset();
		
		wc_iterator b = wc_iterator(this, offset);
		wc_iterator e = wc_iterator(this, inSelection.GetMaxOffset());
	
		boost::regex::flag_type flags = boost::regex_constants::normal;
		
		if (inIgnoreCase)
			flags |= boost::regex_constants::icase;
	
		wstring expr;
		Convert(inWhat, expr);
		
		boost::wregex re(expr, flags);
	
		boost::regex_constants::match_flag_type match_flags =
			boost::regex_constants::match_not_dot_newline |
			boost::regex_constants::match_not_dot_null;
		
		if (offset > 0)
		{
			match_flags |= boost::regex_constants::match_prev_avail;
		
			if (GetChar(offset - 1) != '\n')
				match_flags |= boost::regex_constants::match_not_bol;
		}
		
		result = boost::regex_match(b, e, re, match_flags);
	}
	else
	{
		ustring s;
		GetText(inSelection.GetMinOffset(),
			inSelection.GetMaxOffset() - inSelection.GetMinOffset(), s);
		result = inWhat == s;
	}
	
	return result;
}

void MTextBuffer::ReplaceExpression(MSelection inSelection,
	ustring inExpression, bool inIgnoreCase, ustring inFormat,
	ustring& outReplacement)
{
	uint32 offset = inSelection.GetMinOffset();
	
	wc_iterator b = wc_iterator(this, offset);
	wc_iterator e = wc_iterator(this, inSelection.GetMaxOffset());

	boost::regex::flag_type flags = boost::regex_constants::normal;
	
	if (inIgnoreCase)
		flags |= boost::regex_constants::icase;

	wstring expr, fmt;
	Convert(inExpression, expr);
	Convert(inFormat, fmt);
	
	boost::wregex re(expr, flags);

	boost::regex_constants::match_flag_type match_flags =
		boost::regex_constants::match_not_dot_newline |
		boost::regex_constants::match_not_dot_null;
	
	if (offset > 0)
	{
		match_flags |= boost::regex_constants::match_not_bob;
	
		if (GetChar(offset - 1) != '\n')
			match_flags |= boost::regex_constants::match_not_bol;
	}
	
	vector<wchar_t> result;
	(void)boost::regex_replace(back_inserter(result), b, e, re, fmt, match_flags);
	
	if (result.size())
	{
		auto_ptr<MDecoder> decoder(
			MDecoder::GetDecoder(kEncodingUCS2, &result[0], result.size() * sizeof(wchar_t)));

		decoder->GetText(outReplacement);
	}
	else
		outReplacement.clear();
}

void
MTextBuffer::CollectWordsBeginningWith(
	uint32					inFromOffset,
	MDirection				inDirection,
	ustring					inPattern,
	vector<ustring>&		ioStrings)
{
	set<ustring> keys;
	
	for (vector<ustring>::iterator k = ioStrings.begin(); k != ioStrings.end(); ++k)
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
			ustring k;
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

uint32 MTextBuffer::GetNextCharLength(uint32 inOffset) const
{
	uint32 result = 1;
	
	if (inOffset < mLogicalLength - 1)
	{
		UniChar ch = GetChar(inOffset);

		if (ch >= 0xD800 and ch <= 0xDBFF)
		{
			UniChar ch = GetChar(inOffset + 1);
			
			if (ch >= 0xDC00 and ch <= 0xDFFF)
				result = 2;
		}
	}
	
	return result;
}

uint32 MTextBuffer::GetPrevCharLength(uint32 inOffset) const
{
	uint32 result = 1;

	if (inOffset > 1)
	{
		UniChar ch = GetChar(inOffset - 1);

		if (ch >= 0xD800 and ch <= 0xDBFF)
		{
			UniChar ch = GetChar(inOffset - 2);
			
			if (ch >= 0xDC00 and ch <= 0xDFFF)
				result = 2;
		}
	}

	return result;
}

wchar_t MTextBuffer::GetWChar(uint32 inOffset) const
{
	wchar_t result = GetChar(inOffset);

	if (result >= 0xD800 and result <= 0xDBFF)
	{
		UniChar ch = GetChar(inOffset + 1);
		
		if (ch >= 0xDC00 and ch <= 0xDFFF)
			result = (result - 0xD800) * 0x400 + (ch - 0xDC00) + 0x10000;
	}
	
	return result;
}

void MTextBuffer::reserve(uint32 inSize)
{
	if (inSize > mPhysicalLength)
	{
		auto_array<UniChar> tmp(new UniChar[inSize]);
		
		if (mData != nil)
		{
			MoveGapTo(mLogicalLength);
			memcpy(tmp.get(), mData, mLogicalLength * sizeof(UniChar));
			delete[] mData;
		}
		
		mData = tmp.release();
		mPhysicalLength = inSize;
	}
}

bool MTextBuffer::const_iterator::operator==(const char* inText) const
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
