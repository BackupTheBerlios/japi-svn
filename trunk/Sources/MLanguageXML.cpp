//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MLanguageXML.cpp 48 2005-08-03 08:06:02Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 15:26:34
*/

#include "MJapi.h"

#include "MLanguageXML.h"
#include "MTextBuffer.h"
#include "MUnicode.h"
#include "MFile.h"

#include <stack>
#include <cassert>

using namespace std;

enum {
	START = 0,
		TAGSTART,
		TAGNAME,
		TAG,
		TAGSTRING1,
		TAGSTRING2,
		TAGATTRIBUTE,
		SPECIAL,
		COMMENT_DTD,
		COMMENT,
		COMMENT_END
};

MLanguageXML::MLanguageXML()
{
}

void
MLanguageXML::Init()
{
}

void
MLanguageXML::StyleLine(
	const MTextBuffer&	inText,
	uint32				inOffset,
	uint32				inLength,
	uint16&				ioState)
{
	if (inLength <= 0)
		return;
	
	bool leave = false;
	bool close = ioState & 0x1000;
	ioState &= 0x0FFF;

	MTextBuffer::const_iterator text(inText.begin() + inOffset);
	uint32 i = 0, s = 0;
	
	SetStyle(s, kLTextColor);

	while (not leave)
	{
		char c = 0;
		if (i < inLength)
			c = text[i];
		++i;
		
		switch (ioState)
		{
			case START:
				close = false;
				if (c == '<')
					ioState = TAGSTART;
				else if (c == '&')
					ioState = SPECIAL;
				else if (c == 0 or c == '\n')
					leave = true;
					
				if ((leave or ioState != START) and s < i)
				{
					SetStyle(s, kLTextColor);
					s = i - 1;
				}
				break;
			
			case TAGSTART:
				if (isalpha(c) or c == '/')
				{
					close = (c == '/');
					SetStyle(s, kLTagColor);
					s = i - 1;
					ioState = TAGNAME;
				}
				else if (c == '?')
					;//ioState = COMMENT_DTD;
				else if (c == '!')
					ioState = COMMENT_DTD;
				else if (c == 0 or c == '\n')
				{
					SetStyle(s, kLTagColor);
					leave = true;
				}
				else if (not isspace(c))
				{
					--i;
					ioState = TAG;
				}
				break;
			
			case TAGNAME:
				if (not isalnum(c) and c != '-' and c != ':' and c != '_')
				{
					SetStyle(s, kLKeyWordColor);
					close = false;
					ioState = TAG;
					s = --i;
				}
				break;

			case TAG:
				switch (c)
				{
					case 0:
					case '\n':
						SetStyle(s, kLTagColor);
						leave = true;
						break;
					case '>':
						SetStyle(s, kLTagColor);
						s = i;
						ioState = START;
						break;
					case '"':
						SetStyle(s, kLTagColor);
						s = i - 1;
						ioState = TAGSTRING1;
						break;
					case '\'':
						SetStyle(s, kLTagColor);
						s = i - 1;
						ioState = TAGSTRING2;
						break;
					default:
						if (isalpha(c))
						{
							SetStyle(s, kLTagColor);
							s = i - 1;
							ioState = TAGATTRIBUTE;
						}
						break;
				}
				break;
			
			case TAGSTRING1:
				if (c == '"')
				{
					SetStyle(s, kLStringColor);
					s = i;
					ioState = TAG;
				}
				else if (c == '\n' or c == 0)
				{
					SetStyle(s, kLStringColor);
					leave = true;
				}
				break;
			
			case TAGSTRING2:
				if (c == '\'')
				{
					SetStyle(s, kLStringColor);
					s = i;
					ioState = TAG;
				}
				else if (c == '\n' or c == 0)
				{
					SetStyle(s, kLStringColor);
					leave = true;
				}
				break;
			
			case TAGATTRIBUTE:
				if (not isalnum(c) and c != '-' and c != '_' and c != ':')
				{
					SetStyle(s, kLAttribColor);
					s = --i;
					ioState = TAG;
				}
				break;
			
			case SPECIAL:
				if (c == 0 or c == '\n')
				{
					SetStyle(s, kLTextColor);
					ioState = START;
					leave = true;
				}
				else if (c == ';')
				{
					SetStyle(s, kLCharConstColor);
					s = i;
					ioState = START;
				}
				else if (isspace(c))
					ioState = START;
				break;
			
			case COMMENT_DTD:
				if (c == '-' and text[i] == '-' and i == s + 3 and text[i - 2] == '!' and text[i - 3] == '<')
				{
					SetStyle(s, kLTagColor);
					s = i - 1;
					++i;
					ioState = COMMENT;
				}
				else if (c == '>')
				{
					SetStyle(s, kLTagColor);
					s = i;
					ioState = START;
				}
				else if (c == 0 or c == '\n')
				{
					SetStyle(s, kLTagColor);
					leave = true;
				}
				break;
				
			case COMMENT:
				if (c == '-' and text[i] == '-')
				{
					SetStyle(s, kLCommentColor);
					s = ++i;
					ioState = COMMENT_END;
				}
				else if (c == 0 or c == '\n')
				{
					SetStyle(s, kLCommentColor);
					leave = true;
				}
				break;
			
			case COMMENT_END:
				if (c == '>')
				{
					SetStyle(s, kLTagColor);
					s = i;
					ioState = START;
				}
				else if (c == 0 or c == '\n')
				{
					SetStyle(s, kLTagColor);
					leave = true;
				}
				break;

			default:	// error condition, gracefully leave the loop
				leave = true;
				break;
		}
	}
	
	if (close) ioState |= 0x1000;
}

bool
MLanguageXML::Balance(
	const MTextBuffer&	inText,
	uint32&				ioOffset,
	uint32&				ioLength)
{
	return false;
}

bool
MLanguageXML::IsBalanceChar(
	wchar_t				inChar)
{
	return false;
}

bool
MLanguageXML::IsSmartIndentLocation(
	const MTextBuffer&	inText,
	uint32				inOffset)
{
//	MTextBuffer::const_iterator txt = inText.begin() + inOffset;
	bool result = false;
	
//	if (inOffset > 0 and *(txt - 1) == '>')
//	{
//		while (txt.GetOffset() > 0 and *(txt - 1) != '\n')
//			--txt;
//		
//		int openLevel = 0;
//		
//		while (txt.GetOffset() < inOffset)
//		{
//			if (*txt == '<')
//			{
//				++txt;
//				
//				if (*txt == '/')
//				
//				while (t
//			}
//			else if (*txt == '&')
//			
//			else
//				++txt;
//		}
//	}
	
	return result;
}

bool
MLanguageXML::IsSmartIndentCloseChar(
	wchar_t				inChar)
{
	return false;
}

static const
	char kPrefix[] = "<!--",
	kPostfix[] = "-->";

const uint32
	kPrefixLength = 4,
	kPostfixLength = 3;

void
MLanguageXML::CommentLine(
	string&				ioLine)
{
	ioLine.insert(0, kPrefix, kPrefixLength);

	if (ioLine.length() >= kPrefixLength + 2)
	{
		for (string::iterator ch = ioLine.begin() + kPrefixLength + 1; ch != ioLine.end(); ++ch)
		{
			if (*ch == '-' and *(ch - 1) == '-')
			{
				const char kBullet[] = "•";
				uint32 n = sizeof(kBullet);
				ioLine.insert(ch, kBullet, kBullet + n);
				ch += n + 1;
			}
		}
	}
	
	ioLine.insert(ioLine.length(), kPostfix, kPostfixLength);
}

void
MLanguageXML::UncommentLine(
	string&				ioLine)
{
	const char escapedDoubleDash[] = "-•-";
	
	if (ioLine.length() >= kPrefixLength + kPostfixLength and
		ioLine.compare(0, kPrefixLength, kPrefix) == 0 and
		ioLine.compare(ioLine.length() - kPostfixLength, kPostfixLength, kPostfix) == 0)
	{
		ioLine.erase(0, kPrefixLength);
		ioLine.erase(ioLine.length() - kPostfixLength, kPostfixLength);
		
		string::size_type p = ioLine.find(escapedDoubleDash);
		while (p != string::npos)
		{
			ioLine.erase(p + 1, 1);
			p = ioLine.find(escapedDoubleDash, p);
		}
	}
}

uint32
MLanguageXML::MatchLanguage(
	const std::string&	inFile,
	MTextBuffer&		inText)
{
	uint32 result = 0;

	if (FileNameMatches("*.xml;*.xslt;*.plist;*.xsd;*.dtd;*.xpgt", inFile))
	{
		result += 90;
	}
	else
	{
		const char kXMLString[] = "<?xml";
		char s[8] = {};
		uint32 l = 0;
		
		MTextBuffer::iterator txt(inText.begin());
		while (*txt and l < sizeof(kXMLString) - 1)
		{
			if (not isspace(*txt))
			{
				if (isprint(*txt))
					s[l++] = tolower(*txt);
				else
					break;
			}
			++txt;
		}
		
		if (strcmp(kXMLString, s) == 0)
			result += 80;
	}
	return result;
}

bool MLanguageXML::Softwrap() const
{
	return false;
}
