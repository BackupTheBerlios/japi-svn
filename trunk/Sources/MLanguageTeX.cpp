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

/*	$Id: MLanguageTeX.cpp 65 2005-08-24 20:37:58Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 15:26:34
*/

#include "Japie.h"

#include "MLanguageTeX.h"
#include "MTextBuffer.h"
#include "MUnicode.h"
#include "MFile.h"

#include <stack>
#include <cassert>

using namespace std;

enum
{
	START, COMMAND1, COMMAND2, CONSTCHAR, 
		LCOMMENT, MATH, WOORD, VERBATIM
};

const UniChar
	kCommentPrefix[] = { '%', 0 };

bool operator==(MTextBuffer::const_iterator& inIterator, const char* inText)
{
	bool result = true;
	
	while (*inText != 0 and *inIterator != 0)
	{
		if (*inIterator != *inText)
		{
			result = false;
			break;
		}
		
		++inIterator;
		++inText;
	}
	
	return result;
}

MLanguageTeX::MLanguageTeX()
{
}

void
MLanguageTeX::Init()
{
	const char* keywords[] = {
		"cite", "ref", "label", 
		
		"begin", "end", 
		
		"part", "chapter", "section", "subsection", "subsubsection",
		"paragraph", "subparagraph", nil
	};
	
	AddKeywords(keywords);
	GenerateDFA();
}

void
MLanguageTeX::StyleLine(
	const MTextBuffer&	inText,
	uint32				inOffset,
	uint32				inLength,
	UInt16&				ioState)
{
	uint32 i = 0, s = 0, kws = 0, esc = 0;
	bool leave = false;

	MTextBuffer::const_iterator text = inText.begin() + inOffset;
	MTextBuffer::const_iterator end = inText.end();

	UniChar verbChar = ioState >> 8;
	ioState &= 0x00ff;

	if (ioState == LCOMMENT)
		SetStyle(0, kLCommentColor);
	else
		SetStyle(0, kLTextColor);

	if (inLength <= 0)
		return;
	
	while (not leave)
	{
		UniChar c = 0;
		if (i < inLength)
			c = text[i];
		++i;
		
		if (c == '`')
		{
			if (i < inLength)
				c = text[i];
			++i;
			c = ' ';
		}
		
		switch (ioState)
		{
			case START:
				if (c == '\\')
					ioState = COMMAND1;
				else if (c == '%')
					ioState = LCOMMENT;
				else if (c == '$')
					ioState = MATH;
				else if (c == '\n' or c == 0)
					leave = true;
				else if (isalnum(c))
					ioState = WOORD;
                                      /* This is a bit strange to have some of the characters covered above, but who cares?*/
				else if (c == '#' or c == '&'
						or c == '~' or c == '_' or c == '^'
						or c == '{' or c == '}'
						or c == '[' or c == ']' 
						)
					ioState = CONSTCHAR;
					
				if ((leave or ioState != START) and s < i)
				{
					SetStyle(s, kLTextColor);
					s = i - 1;
				}
				break;

			case CONSTCHAR:
				SetStyle(s, kLCharConstColor);
				s = --i;
				ioState = START;
				break;

			case WOORD:
				if (not isalnum(c))
				{
					SetStyle(s, kLTextColor);
					s = --i;
					ioState = START;
				}
				break;
			
			case LCOMMENT:
				SetStyle(s, kLCommentColor);
				leave = true;
				if (*(text + inLength - 1) == '\n')
					ioState = START;
				break;
			
			case COMMAND1:
				if (c == '(')
				{
					  ioState = MATH;
				}
				else if ((isalnum(c) and c != '_' ) or (c == '@'))	/* a generic command has been found. */
					/* Note that commands with "@" in their name only appear in .cls or .sty files */
				{
					kws = Move(c, 1);
					ioState = COMMAND2;
				}
				else   /* we are escaping a special text character such as \# or \$*/
				{
					SetStyle(s, kLKeyWordColor);
					s = i;
					ioState = START;
				}
				break;
			
			case COMMAND2:    /* Inside the name of a generic command */
				if (not (isalnum(c)) and not (c == '@')) /* found end of command name */
				{      /* now check the command name against a keyword list */
					int kwc;

					if (i > s + 1 and (kwc = IsKeyWord(kws)) != 0)
					{
							 /* use a specific keyword category color */
						if (kwc & 1)
							SetStyle(s, kLKeyWordColor);
//						else if (kwc & 2)
//							SetStyle(s, kLUser1);
//						else if (kwc & 4)
//							SetStyle(s, kLUser2);
//						else if (kwc & 8)
//							SetStyle(s, kLUser3);
//						else if (kwc & 16)
//							SetStyle(s, kLUser4);
					}
					else      /* use a generic keyword color, but with a colored backslash */
					{
						SetStyle(s, kLKeyWordColor);
						SetStyle(s + 1, kLStringColor);
					}

					MTextBuffer::const_iterator txt = text + s;
					if (txt == "\\verb" and not isalnum(*(txt + 5)))
					{
						ioState = VERBATIM;
						verbChar = c;
						s = i - 1;
					}
					else if (txt == "\\begin{verbatim}")
					{
						SetStyle(i - 1, kLCharConstColor);
						SetStyle(i, kLTextColor);
						SetStyle(i + 8, kLCharConstColor);
						SetStyle(i + 9, kLTextColor);
						
						ioState = VERBATIM;
						verbChar = 1;
						s = i + 9;
					}
					else
					{
						ioState = START;
						s = --i;
					}
				}
				else if (kws) /* still in command name */
					kws = Move((int)(unsigned char)c, kws);
				break;
			
			
			case MATH:
                                     /* both "$" and "\)" will end math mode */
				if ( (c == '$' and not esc) or (c == ')' and esc) )
				{
					SetStyle(s, kLPreProcessorColor);
					s = i;
					ioState = START;
				} 
				else if (c == '\n' or c == 0)
				{
					SetStyle(s, kLPreProcessorColor);
					leave = true;
				}
				else
					esc = not esc and (c == '\\');
				break;

			case VERBATIM:
				if (c == '\n' or c == 0)
				{
					SetStyle(s, kLTextColor);
					leave = true;
				}
				else if (verbChar == 1 and c == '\\')
				{
					MTextBuffer::const_iterator text = inText.begin();
					text += s;
					if (text == "\\end{verbatim}")
					{
						SetStyle(s, kLTextColor);
						ioState = START;
						i = s;
					}
				}
				else if (c == verbChar)
				{
					SetStyle(s, kLTextColor);
					s = i;
					ioState = START;
				}
				break;
			
			default:	// error condition, gracefully leave the loop
				leave = true;
				break;
		}
	}
	
	ioState |= verbChar << 8;
}

static MTextBuffer::const_iterator skip(MTextBuffer::const_iterator inText)
{
	while (*inText)
	{
		switch (*inText)
		{
			case '\\':
				if (inText == "\\verb" and not isalnum(*(inText + 5)))
				{
					char c = *(inText + 5);
					inText += 6;
					
					while (*inText and *inText != c)
						++inText;

					if (*inText)
						++inText;
				}
				else if (inText == "\\begin{verbatim}")
				{
					inText += 16;
					while (*inText)
					{
						if (*inText and inText == "\\end{verbatim}")
						{
							inText += 14;
							break;
						}
						++inText;
					}
				}
				else
					++inText;
				break;
				
			case '%':
				++inText;
				while (*inText and *inText != '\n')
					++inText;
				break;
			
			case '{':
			case '[':
			case ']':
			case '}':
				return inText;
		}
		inText++;
	}
	
	return inText;
}

bool
MLanguageTeX::Balance(
	const MTextBuffer&	inText,
	uint32&				ioOffset,
	uint32&				ioLength)
{
	MTextBuffer::const_iterator txt = inText.begin();
	
	stack<int> bls, sbls;
	
	while (txt.GetOffset() < ioOffset + ioLength)
	{
		switch (*txt)
		{
			case '{':	bls.push(txt.GetOffset());			break;
			case '[':	sbls.push(txt.GetOffset());			break;
			case '}':	if (not bls.empty()) bls.pop();		break;
			case ']':	if (not sbls.empty()) sbls.pop();	break;
		}
		txt = skip(txt + 1);
	}
	
	char ec = 0, oc = 0;
	stack<int> *s = NULL;
	
	int db, dsb;
	
	db = bls.empty() ? -1 : static_cast<int32>(ioOffset) - bls.top();
	dsb = sbls.empty() ? -1 : static_cast<int32>(ioOffset) - sbls.top();
	
	if (db < 0 and dsb < 0)
		return false;
	
	if (db >= 0 and (dsb < 0 or db < dsb))
	{
		oc = '{';
		ec = '}';
		s = &bls;
	}
	
	if (dsb >= 0 and (db < 0 or dsb < db))
	{
		oc= '[';
		ec = ']';
		s = &sbls;
	}
	
	if (ec)
	{
		int l = 1;
		
		while (*txt)
		{
			if (*txt == ec)
			{
				if (--l == 0)
				{
					ioOffset = s->top() + 1;
					ioLength = txt.GetOffset() - ioOffset;
					return true;
				}

				if (not s->empty())
					s->pop();
			}
			else if (*txt == oc)
			{
				l++;
				s->push(0);
			}

			txt = skip(txt + 1);
		}
	}
	
	return false;
}

bool
MLanguageTeX::IsBalanceChar(
	UniChar	inChar)
{
	return inChar == ']' or inChar == '}';
}

bool
MLanguageTeX::IsSmartIndentLocation(
	const MTextBuffer&	inText,
	uint32				inOffset)
{
//	return inOffset > 0 and inText.GetChar(inOffset - 1) == '{';
	return false;
}

bool
MLanguageTeX::IsSmartIndentCloseChar(
	UniChar				inChar)
{
//	return inChar == '}';
	return false;
}

void
MLanguageTeX::CommentLine(
	ustring&			ioLine)
{
	ioLine.insert(0, kCommentPrefix);
}

void
MLanguageTeX::UncommentLine(
	ustring&			ioLine)
{
	if (ioLine.length() >= 1 and ioLine[0] == kCommentPrefix[0])
		ioLine.erase(0, 1);
}

uint32
MLanguageTeX::MatchLanguage(
	const std::string&	inFile,
	MTextBuffer&		inText)
{
	uint32 result = 0;
	if (FileNameMatches("*.tex;*.TeX;*.sty;*.cls", inFile))
	{
		result += 90;
	}
	return result;
}

bool MLanguageTeX::Softwrap() const
{
	return true;
}
