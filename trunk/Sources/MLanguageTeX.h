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

/*	$Id: MLanguageTeX.h 64 2005-08-22 20:43:37Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 15:27:00
*/

#ifndef LANGUAGETEX_H
#define LANGUAGETEX_H

#include "MLanguage.h"

class MLanguageTeX : public MLanguage
{
  public:
					MLanguageTeX();
	
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

	virtual bool	Softwrap() const;
	
	static uint32	MatchLanguage(
						const std::string&	inFile,
						MTextBuffer&		inText);

};

#endif // LANGUAGEPERL_H
