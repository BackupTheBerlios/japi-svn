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

/*	$Id: MSelection.cpp 151 2007-05-21 15:59:05Z maarten $
	Copyright Maarten L. Hekkelman
	Created Thursday July 01 2004 21:03:45
*/

#include "MJapieG.h"

#include <cassert>

#include "MSelection.h"
#include "MTextDocument.h"
#include "MError.h"

using namespace std;

inline
MSelection::Pos::Pos(
	uint64		inOffset)
	: mOffset(inOffset)
{
}

inline
MSelection::Pos::Pos(
	uint32		inLine,
	uint32		inColumn)
{
	mLocation.line = inLine;
	mLocation.column = inColumn;
}

bool
inline
MSelection::Pos::operator==(
	const Pos&	inOther) const
{
	return mOffset == inOther.mOffset;
}

bool
inline
MSelection::Pos::operator!=(
	const Pos&	inOther) const
{
	return mOffset != inOther.mOffset;
}

MSelection::MSelection()
	: mIsBlock(false)
{
}

MSelection::MSelection(
	uint32		inAnchor,
	uint32		inCaret)
	: mAnchor(inAnchor)
	, mCaret(inCaret)
	, mIsBlock(false)
{
}

MSelection::MSelection(
	uint32		inAnchorLine,
	uint32		inAnchorColumn,
	uint32		inCaretLine,
	uint32		inCaretColumn)
	: mAnchor(inAnchorLine, inAnchorColumn)
	, mCaret(inCaretLine, inCaretColumn)
	, mIsBlock(true)
{
}

MSelection::MSelection(
	const MSelection&	inSelection)
{
	mAnchor = inSelection.mAnchor;
	mCaret = inSelection.mCaret;
	mIsBlock = inSelection.mIsBlock;
}

bool MSelection::operator==(
	const MSelection&	inOther) const
{
	return
		mIsBlock == inOther.mIsBlock and
		mAnchor == inOther.mAnchor and
		mCaret == inOther.mCaret;
}

bool MSelection::operator!=(
	const MSelection&	inOther) const
{
	return
		mIsBlock != inOther.mIsBlock or
		mAnchor != inOther.mAnchor or
		mCaret != inOther.mCaret;
}

bool MSelection::IsBlock() const
{
	return mIsBlock;
}

bool MSelection::IsEmpty() const
{
	return mAnchor == mCaret;
}

void MSelection::Set(
	uint32		inAnchor,
	uint32		inCaret)
{
	mAnchor.mOffset = inAnchor;
	mCaret.mOffset = inCaret;
	mIsBlock = false;
}

void MSelection::Set(
	uint32		inAnchorLine,
	uint32		inAnchorColumn,
	uint32		inCaretLine,
	uint32		inCaretColumn)
{
	mAnchor.mLocation.line = inAnchorLine;
	mAnchor.mLocation.column = inAnchorColumn;
	mCaret.mLocation.line = inCaretLine;
	mCaret.mLocation.column = inCaretColumn;
	mIsBlock = true;
}

uint32 MSelection::GetAnchor() const
{
	assert(not mIsBlock);
	return mAnchor.mOffset;
}

void MSelection::SetAnchor(
	uint32		inAnchor)
{
	assert(not mIsBlock);
	mAnchor.mOffset = inAnchor;
}

uint32 MSelection::GetCaret() const
{
	assert(not mIsBlock);
	return mCaret.mOffset;
}

void MSelection::SetCaret(
	uint32		inCaret)
{
	assert(not mIsBlock);
	mCaret.mOffset = inCaret;
}

void MSelection::GetAnchorLineAndColumn(
	MTextDocument&	inDoc,
	uint32&			outLine,
	uint32&			outColumn) const
{
	assert(not mIsBlock);
	outLine = inDoc.OffsetToLine(mAnchor.mOffset);
	outColumn = inDoc.OffsetToColumn(mAnchor.mOffset);
}

void MSelection::GetCaretLineAndColumn(
	MTextDocument&	inDoc,
	uint32&			outLine,
	uint32&			outColumn) const
{
	assert(not mIsBlock);
	outLine = inDoc.OffsetToLine(mCaret.mOffset);
	outColumn = inDoc.OffsetToColumn(mCaret.mOffset);
}

uint32 MSelection::GetMinLine(
	MTextDocument&	inDoc) const
{
	uint32 result;

	if (mIsBlock)
	{
		result = mAnchor.mLocation.line;
		if (mCaret.mLocation.line < result)
			result = mCaret.mLocation.line;
	}
	else
	{
		uint32 offset = mAnchor.mOffset;
		if (mCaret.mOffset < offset)
			offset = mCaret.mOffset;
		
		result = inDoc.OffsetToLine(offset);
	}

	return result;
}

uint32 MSelection::GetMaxLine(
	MTextDocument&	inDoc) const
{
	uint32 result;

	if (mIsBlock)
	{
		result = mAnchor.mLocation.line;
		if (mCaret.mLocation.line > result)
			result = mCaret.mLocation.line;
	}
	else
	{
		uint32 offset = mAnchor.mOffset;
		if (mCaret.mOffset > offset)
			offset = mCaret.mOffset;
		
		result = inDoc.OffsetToLine(offset);
		if (mAnchor != mCaret and offset == inDoc.LineStart(result) and result > 0)
			--result;
	}

	return result;
}

uint32 MSelection::GetMinColumn(
	MTextDocument&	inDoc) const
{
	uint32 result;

	if (mIsBlock)
	{
		result = mAnchor.mLocation.column;
		if (mCaret.mLocation.column < result)
			result = mCaret.mLocation.column;
	}
	else
	{
		uint32 line, ac, cc;
		GetAnchorLineAndColumn(inDoc, line, ac);
		GetCaretLineAndColumn(inDoc, line, cc);
		
		result = ac;
		if (cc < result)
			result = cc;
	}

	return result;
}

uint32 MSelection::GetMaxColumn(
	MTextDocument&	inDoc) const
{
	uint32 result;

	if (mIsBlock)
	{
		result = mAnchor.mLocation.column;
		if (mCaret.mLocation.column > result)
			result = mCaret.mLocation.column;
	}
	else
	{
		uint32 line, ac, cc;
		GetAnchorLineAndColumn(inDoc, line, ac);
		GetCaretLineAndColumn(inDoc, line, cc);
		
		result = ac;
		if (cc > result)
			result = cc;
	}

	return result;
}

uint32 MSelection::GetMinOffset(
	MTextDocument&	inDoc) const
{
	uint32 result;

	if (mIsBlock)
	{
		uint32 line = GetMinLine(inDoc);
		uint32 column = GetMinColumn(inDoc);
		result = inDoc.LineStart(line) + column;
		if (result >= inDoc.LineStart(line + 1))
			result = inDoc.LineStart(line + 1) - 1;
	}
	else
	{
		result = mAnchor.mOffset;
		if (mCaret.mOffset < result)
			result = mCaret.mOffset;
	}

	return result;
}

uint32 MSelection::GetMaxOffset(
	MTextDocument&	inDoc) const
{
	uint32 result;

	if (mIsBlock)
	{
		uint32 line = GetMaxLine(inDoc);
		uint32 column = GetMaxColumn(inDoc);
		result = inDoc.LineStart(line) + column;
		if (result >= inDoc.LineStart(line + 1))
			result = inDoc.LineStart(line + 1) - 1;
	}
	else
	{
		result = mAnchor.mOffset;
		if (mCaret.mOffset > result)
			result = mCaret.mOffset;
	}

	return result;
}

MSelection MSelection::SelectLines(
	MTextDocument&	inDoc) const
{
	MSelection result;
	
	uint32 line = GetMinLine(inDoc);
	result.SetAnchor(inDoc.LineStart(line));
	
	line = GetMaxLine(inDoc);
	result.SetCaret(inDoc.LineStart(line + 1));
	
	return result;
}

uint32 MSelection::CountLines(
	MTextDocument&	inDoc) const
{
	return GetMaxLine(inDoc) - GetMinLine(inDoc) + 1;
}

uint32 MSelection::GetMinOffset() const
{
	if (IsBlock())
		THROW(("parameter error"));
	
	uint32 result = mAnchor.mOffset;
	if (mCaret.mOffset < result)
		result = mCaret.mOffset;
	return result;
}

uint32 MSelection::GetMaxOffset() const
{
	if (IsBlock())
		THROW(("parameter error"));

	uint32 result = mAnchor.mOffset;
	if (mCaret.mOffset > result)
		result = mCaret.mOffset;
	return result;
}
