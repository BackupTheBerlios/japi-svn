//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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
	uint32						index;
	std::vector<MNamedRange>	subrange;
	
								MNamedRange();
};

struct MIncludeFile
{
	std::string					name;
	bool						isQuoted;
};

class MIncludeFileList : public std::vector<MIncludeFile> {};

class MLanguage
{
  public:
	
	static MLanguage* GetLanguageForDocument(
						const std::string&	inFile,
						MTextBuffer&		inText);

	static MLanguage* GetLanguage(
						const std::string&	inName);
		
	virtual std::string
					GetName() const = 0;
			
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
						wchar_t				inChar,
						const MTextBuffer&	inText,
						uint32&				ioOpenOffset);

	virtual bool	IsAutoCompleteChar(
						wchar_t				inChar,
						const MTextBuffer&	inText,
						uint32				ioOffset,
						std::string&		outCompletionText,
						int32&				outCaretDelta);

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
						MNamedRange&		inRanges,
						const std::string&	inNSName,
						MMenu&				inMenu,
						uint32&				ioIndex);
	
	bool			GetSelectionForParseItem(
						const MNamedRange&	inRanges,
						uint32				inItem,
						uint32&				outSelectionStart,
						uint32&				outSelectionEnd);

	virtual void	Parse(
						const MTextBuffer&	inText,
						MNamedRange&		outRange,
						MIncludeFileList&	outIncludeFiles);

	void			CollectKeyWordsBeginningWith(
						std::string			inPattern,
						std::vector<std::string>&
											ioStrings);

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
