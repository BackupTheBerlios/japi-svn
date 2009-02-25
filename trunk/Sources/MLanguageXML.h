//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MLanguageXML.h 44 2005-07-22 20:40:22Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 15:27:00
*/

#ifndef LANGUAGEXML_H
#define LANGUAGEXML_H

#include "MLanguage.h"

class MLanguageXML : public MLanguage
{
  public:
					MLanguageXML();
	
	virtual std::string
					GetName() const			{ return "XML"; }

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
						wchar_t				inChar,
						const MTextBuffer&	inText,
						uint32&				ioOpenOffset);

	virtual bool	IsAutoCompleteChar(
						wchar_t				inChar,
						const MTextBuffer&	inText,
						uint32				inOffset,
						std::string&		outCompletionText);

	virtual void	CommentLine(
						std::string&		ioLine);

	virtual void	UncommentLine(
						std::string&		ioLine);

	virtual bool	Softwrap() const;
	
	static uint32	MatchLanguage(
						const std::string&	inFile,
						MTextBuffer&		inText);

};

#endif // LANGUAGEHTML_H
