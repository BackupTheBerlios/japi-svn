//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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
					MSelection(
						MTextDocument*		inDocument);

					MSelection(
						const MSelection&	inSelection);

	MSelection		operator=(
						const MSelection&	rhs);

					MSelection(
						MTextDocument*		inDocument,
						uint32				inAnchor,
						uint32				inCaret);

					MSelection(
						MTextDocument*		inDocument,
						uint32				inAnchorLine,
						uint32				inAnchorColumn,
						uint32				inCaretLine,
						uint32				inCaretColumn);

	bool			operator==(
						const MSelection&	inOther) const;

	bool			operator!=(
						const MSelection&	inOther) const;
	
	bool			IsBlock() const;

	bool			IsEmpty() const;

	void			SetDocument(
						MTextDocument*		inDocument);

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
						uint32&				outLine,
						uint32&				outColumn) const;

	void			GetCaretLineAndColumn(
						uint32&				outLine,
						uint32&				outColumn) const;

	uint32			GetMinLine() const;

	uint32			GetMaxLine() const;

	uint32			GetMinColumn() const;

	uint32			GetMaxColumn() const;

	uint32			GetMinOffset() const;

	uint32			GetMaxOffset() const;

	uint32			CountLines() const;
	
	MSelection		SelectLines() const;

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
	
	MTextDocument*	mDocument;
	Pos				mAnchor;
	Pos				mCaret;
	bool			mIsBlock;
};

#endif // SELECTION_H
