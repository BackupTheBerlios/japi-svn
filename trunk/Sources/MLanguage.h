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

/*	$Id: MLanguage.h 108 2007-04-21 20:28:14Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 13:50:02
*/

#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <vector>
#include "MTypes.h"
#include "MTextBuffer.h"

class MMenu;

const unsigned int kMaxStyles = 100;

enum {
	kLTextColor,
	kLKeyWordColor,
	kLPreProcessorColor,
	kLCharConstColor,
	kLCommentColor,
	kLStringColor,
	kLTagColor,
	kLAttribColor,
	
	// not really a style...
	kLInvisiblesColor,
	
	kLStyleCount
};

struct MNamedRange
{
	std::string					name;
	uint32						begin;
	uint32						end;
	uint32						selectFrom;
	uint32						selectTo;
	std::vector<MNamedRange>	subrange;
};

struct MIncludeFile
{
	std::string					name;
	bool						isQuoted;
};

struct MIncludeFileList : public std::vector<MIncludeFile> {};

class MLanguage
{
  public:
	
	static MLanguage* GetLanguageForDocument(
						const std::string&	inFile,
						MTextBuffer&		inText);
			
	virtual void	StyleLine(
						const MTextBuffer&	inText,
						uint32				inOffset,
						uint32				inLength,
						uint16&				ioState) = 0;

	uint32			StyleLine(
						const MTextBuffer&	inText,
						uint32				inOffset,
						uint32				inLength,
						uint16&				ioState,
						uint32				outStyles[],
						uint32				outOffsets[]);

	virtual bool	Balance(
						const MTextBuffer&	inText,
						uint32&				ioOffset,
						uint32&				ioLength) = 0;
	
	virtual bool	IsBalanceChar(
						wchar_t				inChar);

	virtual bool	IsSmartIndentLocation(
						const MTextBuffer&	inText,
						uint32				inOffset);

	virtual bool	IsSmartIndentCloseChar(
						wchar_t				inChar);

	virtual uint16	GetInitialState(
						const std::string&	inFile,
						MTextBuffer&		inText);

	virtual void	CommentLine(
						std::string&		ioLine) = 0;

	virtual void	UncommentLine(
						std::string&		ioLine) = 0;

	virtual bool	Softwrap() const;

	std::string		NameForPosition(
						const MNamedRange&	inRanges,
						uint32				inPosition);
	
	void			GetParsePopupItems(
						const MNamedRange&	inRanges,
						const std::string&	inNSName,
						MMenu&				inMenu);
	
	bool			GetSelectionForParseItem(
						const MNamedRange&	inRanges,
						uint32				inItem,
						uint32&				outSelectionStart,
						uint32&				outSelectionEnd);

	virtual void	Parse(
						const MTextBuffer&	inText,
						MNamedRange&		outRange,
						MIncludeFileList&	outIncludeFiles);

  protected:
					MLanguage();
	virtual			~MLanguage();

	void			SetStyle(
						uint32				inOffset,
						uint32				inStyle);
	
	bool			Equal(
						MTextBuffer::const_iterator	inBegin,
						MTextBuffer::const_iterator inEnd,
						const char*					inString);
	
	// keyword support
	
	void			AddKeywords(
						const char*			inKeywords[]);

	void			GenerateDFA();

	int				Move(
						char				inChar,
						int					inState);

	int				IsKeyWord(
						int					inState);
	
	struct MRecognizer*
					mRecognizer;
	uint32			mTag;
	
	uint32*			mStyles;
	uint32*			mOffsets;
	uint32			mLastStyleIndex;
};

#endif // LANGUAGE_H
