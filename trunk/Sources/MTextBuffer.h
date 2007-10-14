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

/*	$Id: MTextBuffer.h 160 2007-05-30 15:37:02Z maarten $
	Copyright Maarten L. Hekkelman
	Created Monday June 21 2004 20:58:15
*/

#ifndef TEXTBUFFER_H
#define TEXTBUFFER_H

#include "MTypes.h"
#include "MUnicode.h"
#include "MError.h"

#include <stack>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/thread.hpp>

enum CursorMovement
{
	eMoveOneCharacter,
	eMoveOneWordForKeyboard,
	eMoveOneWordForMouse
};

enum EOLNKind
{
	eEOLN_UNIX,
	eEOLN_MAC,
	eEOLN_DOS
};

class MSelection;
class Action;
class MFile;
class MMessageList;

typedef std::stack<Action*>	ActionStack;

class MTextBuffer
{
  public:
				MTextBuffer();

				MTextBuffer(
					const std::string& inText);

	virtual		~MTextBuffer();

	void		ReadFromFile(
					MFile&			inFile);

	void		WriteToFile(
					MFile&			inFile);
	
	EOLNKind	GetEOLNKind() const									{ return mEOLNKind; }

	void		SetEOLNKind(
					EOLNKind		inKind);

	MEncoding	GetEncoding() const									{ return mEncoding; }

	void		SetEncoding(
					MEncoding		inEncoding);

	class iterator;

	class ref
	{
		friend class iterator;
		friend class MTextBuffer;
		
	  public:
						operator const char() const					{ return mBuffer.GetChar(mOffset); }
		ref&			operator=(
							char		inChar);
		ref&			operator=(
							const ref&	inOther);
		
	  private:
						ref(
							MTextBuffer&	inBuffer,
							uint32			inOffset)
							: mBuffer(inBuffer)
							, mOffset(inOffset) {}
		
		MTextBuffer&	mBuffer;
		uint32			mOffset;
	};
	
	class iterator : public boost::iterator_facade<iterator, char,
		boost::random_access_traversal_tag, ref, uint32>
	{
	  public:
						iterator() : mBuffer(nil), mOffset(0) {}
						iterator(const iterator& inOther) : mBuffer(inOther.mBuffer), mOffset(inOther.mOffset) {}
						iterator(MTextBuffer* inBuffer, uint32 inOffset) : mBuffer(inBuffer), mOffset(inOffset) {}
		
		uint32			GetOffset() const							{ return mOffset; }
		
	  private:
		friend class boost::iterator_core_access;
		friend class MTextBuffer;

		ref				dereference() const							{ ref result(*mBuffer, mOffset); return result; }

		void			increment()									{ ++mOffset; }
		void			decrement()									{ --mOffset; }
		void			advance(int32 inN)							{ mOffset += inN; }
		int32			distance_to(const iterator& inOther) const	{ return inOther.mOffset - mOffset; }
		bool			equal(const iterator& inOther) const		{ return mOffset == inOther.mOffset; }
		
		MTextBuffer*	mBuffer;
		uint32			mOffset;
	};

	class const_iterator : public boost::iterator_facade<const_iterator, const char,
		boost::random_access_traversal_tag, const char>
	{
	  public:
						const_iterator() : mBuffer(nil), mOffset(0) {}
						const_iterator(const const_iterator& inOther) : mBuffer(inOther.mBuffer), mOffset(inOther.mOffset) {}
						const_iterator(const MTextBuffer* inBuffer, uint32 inOffset) : mBuffer(inBuffer), mOffset(inOffset) {}
		
		uint32			GetOffset() const							{ return mOffset; }

		bool			operator==(const char* inText) const;
				
	  private:
		friend class boost::iterator_core_access;
		friend class MTextBuffer;

		char			dereference() const							{ return mBuffer->GetChar(mOffset); }

		void			increment()									{ ++mOffset; }
		void			decrement()									{ --mOffset; }
		void			advance(int32 inN)							{ mOffset += inN; }
		int32			distance_to(const const_iterator& inOther) const
																	{ return inOther.mOffset - mOffset; }
		bool			equal(const const_iterator& inOther) const	{ return mOffset == inOther.mOffset; }
		
		const MTextBuffer*	mBuffer;
		uint32			mOffset;
	};
	
	iterator	begin()												{ return iterator(this, 0); }
	iterator	end()												{ return iterator(this, mLogicalLength); }
	
	const_iterator	begin() const									{ return const_iterator(this, 0); }
	const_iterator	end() const										{ return const_iterator(this, mLogicalLength); }
	
	char		operator[](
					uint32			inPosition) const				{ return GetChar(inPosition); }
	
	ref			operator[](
					uint32			inPosition);
	
	uint32		GetSize() const										{ return mLogicalLength; }

	void		Insert(
					uint32			inPosition,
					const char*		inText,
					uint32			inLength);

	void		Delete(
					uint32			inPosition,
					uint32			inLength);

	void		Replace(
					uint32			inPosition,
					const char*		inText,
					uint32			inLength);

	void		GetText(
					uint32			inPosition,
					uint32			inLength,
					std::string&	outText) const;

	void		GetText(
					uint32			inPosition,
					char*			outText,
					uint32			inLength) const;

	// wchar_t support
	uint32		GetNextCharLength(
					uint32			inOffset) const;

	uint32		GetPrevCharLength(
					uint32			inOffset) const;

	wchar_t		GetWChar(
					uint32			inOffset) const;

	char		GetChar(
					uint32			inOffset) const;

	uint32		NextCursorPosition(
					uint32			inOffset,
					CursorMovement	inMovement) const;

	uint32		PreviousCursorPosition(
					uint32			inOffset,
					CursorMovement	inMovement) const;
	
	bool		Find(
					uint32			inOffset,
					std::string		inWhat,
					MDirection		inDirection,
					bool			inIgnoreCase,
					bool			inRegex,
					MSelection&		outFound);

	void		ReplaceExpression(
					MSelection		inSelection,
					std::string		inExpression,
					bool			inIgnoreCase,
					std::string		inFormat,
					std::string&	outReplacement);
	
	bool		CanReplace(
					std::string		inWhat,
					bool			inRegex,
					bool			inIgnoreCase,
					MSelection		inSelection);

//	void		FindAll(
//					std::string		inWhat,
//					bool			inIgnoreCase,
//					bool			inRegex,
//					MMessageList&	outHits);
	
	void		CollectWordsBeginningWith(
					uint32			inFromOffset,
					MDirection		inDirection,
					std::string		inPattern,
					std::vector<std::string>&
									ioStrings);
	
	// Undo support
	void		StartAction(
					const std::string&	inAction,
					const MSelection&	inSelection);

	void		SetSelectionBefore(
					const MSelection&	inSelection);

	MSelection	GetSelectionBefore() const;

	void		SetSelectionAfter(
					const MSelection&	inSelection);

	MSelection	GetSelectionAfter() const;

	bool		CanUndo(
					std::string&	outAction);

	void		Undo(
					MSelection&		outSelection,		// The selection to use after this Undo
					uint32&			outChangeOffset,	// The minimal offset of text touched by this Undo
					uint32&			outChangeLength,	// Total length of area touched by Undo
					int32&			outDelta);			// The cumulative amount of characters deleted/inserted

	bool		CanRedo(
					std::string&	outAction);

	void		Redo(
					MSelection&		outSelection,		// The selection to use after this Redo
					uint32&			outChangeOffset,	// The minimal offset of text touched by this Redo
					uint32&			outChangeLength,	// Total length of area touched by Redo
					int32&			outDelta);			// The cumulative amount of characters deleted/inserted

  private:

	friend class ref;
	friend class Action;
	friend class MicroAction;
	
	void			InsertSelf(
						uint32		inPosition,
						const char*	inText,
						uint32		inLength);

	void			DeleteSelf(
						uint32		inPosition,
						uint32		inLength);

					MTextBuffer(const MTextBuffer&);
	MTextBuffer&		operator=(const MTextBuffer&);
	
	void			MoveGapTo(
						uint32		inPosition);

	typedef char	Skip[256];

	void			InitSkip(
						const char*		inPattern,
						uint32			inPatternLength,
						bool			inIgnoreCase,
						MDirection		inDirection,
						Skip&			ioSkip) const;

	bool			MismatchSearch(
						const char*		inPattern,
						uint32			inPatternLength,
						uint32&			ioOffset,
						bool			inIgnoreCase,
						MDirection		inDirection,
						const Skip&		inSkip) const;
	
	void			push_back(
						char			inChar);

	void			push_back(
						wchar_t			inUnicode);
	
	void			reserve(
						uint32			inSize);

	char*			mData;
	uint32			mPhysicalLength;
	uint32			mLogicalLength;
	uint32			mGapOffset;
	ActionStack		mDoneActions;
	ActionStack		mUndoneActions;
	MEncoding		mEncoding;
	EOLNKind		mEOLNKind;
	bool			mBOM;
};

// inlines

inline
char
MTextBuffer::GetChar(
	uint32			inOffset) const
{
	assert(inOffset <= mLogicalLength);
	
	char result = 0;
	
	if (inOffset < mLogicalLength)
	{
		if (inOffset >= mGapOffset)
			inOffset += mPhysicalLength - mLogicalLength;
	
		result = mData[inOffset];
	}
	
	return result;
}

inline
MTextBuffer::ref
MTextBuffer::operator[](
	uint32			inOffset)
{
	if (inOffset >= mLogicalLength)
	{
		assert(false);
		THROW(("Offset out of range"));
	}
	
	if (inOffset >= mGapOffset)
		inOffset += mPhysicalLength - mLogicalLength;

	return ref(*this, inOffset);
}

inline
MTextBuffer::ref&
MTextBuffer::ref::operator=(
	char		inChar)
{
	mBuffer.Replace(mOffset, &inChar, 1);
	return *this;
}

inline
MTextBuffer::ref&
MTextBuffer::ref::operator=(
	const ref&	inOther)
{
	char ch = inOther;
	mBuffer.Replace(mOffset, &ch, 1);
	return *this;
}

inline void MTextBuffer::push_back(
	char		inChar)
{
	if (mData != nil and mLogicalLength < mPhysicalLength and mGapOffset == mLogicalLength)
	{
		mData[mGapOffset] = inChar;
		++mGapOffset;
		++mLogicalLength;
	}
	else
		InsertSelf(mLogicalLength, &inChar, 1);
}

inline void MTextBuffer::push_back(
	wchar_t		inUnicode)
{
	if (inUnicode < 0x080)
	{
		push_back(static_cast<char>(inUnicode));
	}
	else if (inUnicode < 0x0800)
	{
		push_back(static_cast<char> (0x0c0 | (inUnicode >> 6)));
		push_back(static_cast<char> (0x080 | (inUnicode & 0x3f)));
	}
	else if (inUnicode < 0x00010000)
	{
		push_back(static_cast<char> (0x0e0 | (inUnicode >> 12)));
		push_back(static_cast<char> (0x080 | ((inUnicode >> 6) & 0x3f)));
		push_back(static_cast<char> (0x080 | (inUnicode & 0x3f)));
	}
	else
	{
		push_back(static_cast<char> (0x0f0 | (inUnicode >> 18)));
		push_back(static_cast<char> (0x080 | ((inUnicode >> 12) & 0x3f)));
		push_back(static_cast<char> (0x080 | ((inUnicode >> 6) & 0x3f)));
		push_back(static_cast<char> (0x080 | (inUnicode & 0x3f)));
	}
}

#endif // TEXTBUFFER_H
