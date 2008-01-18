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

/*	$Id: MSelection.h 75 2006-09-05 09:56:43Z maarten $
	Copyright Maarten L. Hekkelman
	Created Thursday July 01 2004 20:59:15
*/

#ifndef SELECTION_H
#define SELECTION_H

class MTextDocument;

class MSelection
{
  public:
					MSelection();

					MSelection(
						uint32				inAnchor,
						uint32				inCaret);

					MSelection(
						uint32				inAnchorLine,
						uint32				inAnchorColumn,
						uint32				inCaretLine,
						uint32				inCaretColumn);

					MSelection(
						const MSelection&	inSelection);
	
	bool			operator==(
						const MSelection&	inOther) const;

	bool			operator!=(
						const MSelection&	inOther) const;
	
	bool			IsBlock() const;

	bool			IsEmpty() const;

	void			Set(
						uint32				inAnchor,
						uint32				inCaret);

	void			Set(
						uint32				inAnchorLine,
						uint32				inAnchorColumn,
						uint32				inCaretLine,
						uint32				inCaretColumn);

	uint32			GetAnchor() const;

	void			SetAnchor(
						uint32				inAnchor);

	uint32			GetCaret() const;

	void			SetCaret(
						uint32				inCaret);

	void			GetAnchorLineAndColumn(
						MTextDocument&		inDoc,
						uint32&				outLine,
						uint32&				outColumn) const;

	void			GetCaretLineAndColumn(
						MTextDocument&		inDoc,
						uint32&				outLine,
						uint32&				outColumn) const;

	uint32			GetMinLine(
						MTextDocument&		inDoc) const;

	uint32			GetMaxLine(
						MTextDocument&		inDoc) const;

	uint32			GetMinColumn(
						MTextDocument&		inDoc) const;

	uint32			GetMaxColumn(
						MTextDocument&		inDoc) const;

	uint32			GetMinOffset(
						MTextDocument&		inDoc) const;

	uint32			GetMaxOffset(
						MTextDocument&		inDoc) const;

	uint32			GetMinOffset() const;

	uint32			GetMaxOffset() const;
	
	uint32			CountLines(
						MTextDocument&		inDoc) const;
	
	MSelection		SelectLines(
						MTextDocument&		inDoc) const;

  private:
	
	struct Pos
	{
					Pos(uint64 inOffset = 0);
					Pos(uint32 inLine, uint32 inColumn);

		bool		operator==(const Pos& inOther) const;
		bool		operator!=(const Pos& inOther) const;
		
		union
		{
			uint64	mOffset;
			struct
			{
				uint32	line;
				uint32	column;
			}		mLocation;
		};
	};
	
	Pos				mAnchor;
	Pos				mCaret;
	bool			mIsBlock;
};

#endif // SELECTION_H
