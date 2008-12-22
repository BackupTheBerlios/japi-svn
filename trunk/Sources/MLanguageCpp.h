//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MLanguageCpp.h 145 2007-05-11 14:08:02Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 15:27:00
*/

#ifndef LANGUAGECPP_H
#define LANGUAGECPP_H

#include <stack>

#include "MLanguage.h"

class MLanguageCpp : public MLanguage
{
  public:
					MLanguageCpp();
	
	virtual std::string
					GetName() const			{ return "C++"; }

	virtual void	Init();

	virtual void	StyleLine(
						const MTextBuffer&	inText,
						uint32				inOffset,
						uint32				inLength,
						uint16&				ioState);

	virtual bool	Balance(
						const MTextBuffer&	inText,
						uint32&				ioOffset,
						uint32&				ioLength);

	virtual bool	IsBalanceChar(
						wchar_t				inChar);

	virtual bool	IsSmartIndentLocation(
						const MTextBuffer&	inText,
						uint32				inOffset);

	virtual bool	IsSmartIndentCloseChar(
						wchar_t				inChar);

	virtual void	CommentLine(
						std::string&		ioLine);

	virtual void	UncommentLine(
						std::string&		ioLine);

	virtual void	Parse(
						const MTextBuffer&	inText,
						MNamedRange&		outRange,
						MIncludeFileList&	outIncludeFiles);

	virtual bool	Softwrap() const;

	static uint32	MatchLanguage(
						const std::string&	inFile,
						MTextBuffer&		inText);

  private:

	typedef MTextBuffer::const_iterator		MTextPtr;
	
	MTextPtr		Identifier(
						MTextPtr			inText,
						MNamedRange&		outNamedRange,
						bool				inGetTemplateParameters = true,
						bool				inGetNameSpaces = true);
	
	MTextPtr		SkipToChar(
						MTextPtr			inText,
						char				inChar,
						bool				inIgnoreEscapes = false);
	
	MTextPtr		ParseComment(
						MTextPtr			inText,
						bool				inStripPreProcessor = true);

	MTextPtr		ParseParenthesis(
						MTextPtr			inText,
						char				inOpenChar);

	MTextPtr		ParsePreProcessor(
						MTextPtr			inText,
						MNamedRange&		ioNameSpace,
						MIncludeFileList&	outIncludeFiles);

	MTextPtr		ParseNameSpace(
						MTextPtr			inText,
						MNamedRange&		ioNameSpace,
						MIncludeFileList&	outIncludeFiles);

	MTextPtr		ParseType(
						MTextPtr			inText,
						MNamedRange&		ioNameSpace);

	MTextPtr		ParseIdentifier(
						MTextPtr			inText,
						MNamedRange&		ioNameSpace);

	bool									mExternBlock;
};

#endif // LANGUAGECPP_H
