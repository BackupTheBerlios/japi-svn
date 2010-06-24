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
#include "MUtils.h"

#include <stack>
#include <cassert>
#include <cstring>

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
		CDATA,
		PI,
		DOCTYPE,
		DOCTYPE_E,
		COMMENT_START,
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
	bool close = (ioState & 0x1000) != 0;
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
				{
					SetStyle(s, kLTagColor);
					
					s = i;
					while (isalnum(text[i]))
						++i;
					
					if (text + s == "xml" and isspace(text[i]))
						SetStyle(s, kLKeyWordColor);
					else
						SetStyle(s, kLAttribColor);
					
					ioState = PI;
					s = i;
				}
				else if (c == '!')
				{
					if (text + i == "[CDATA[")
						ioState = CDATA;
					else if (text + i == "DOCTYPE ")
					{
						SetStyle(s, kLTagColor);
						s += 2;
						SetStyle(s, kLKeyWordColor);
						s += 8;
						ioState = DOCTYPE;
					}
					else
						ioState = COMMENT_START;
				}
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
			
			case PI:
				if (c == '?' and text[i] == '>')
				{
					SetStyle(s, kLStringColor);
					s = i - 1;
					SetStyle(s, kLTagColor);
					s = ++i;
					ioState = START;
				}
				else if (c == 0 or c == '\n')
				{
					SetStyle(s, kLStringColor);
					leave = true;
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
			
			case COMMENT_START:
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

			case CDATA:
				if (c == ']' and text[i] == ']' and text[i + 1] == '>')
				{
					SetStyle(s, kLStringColor);
					i += 2;
					s = i;
					ioState = START;
				}
				else if (c == 0 or c == '\n')
				{
					SetStyle(s, kLStringColor);
					leave = true;
				}
				break;

			case DOCTYPE:
				if (c == '<' and text[i] == '!')
				{
					if (text + i + 1 == "ELEMENT " or
						text + i + 1 == "ATTLIST " or
						text + i + 1 == "ENTITY " or
						text + i + 1 == "NOTATION ")
					{
						SetStyle(s, kLTagColor);
						s = i;
						SetStyle(s, kLKeyWordColor);
						
						s = i + 7;
						if (isalnum(text[s]))
							++s;
						if (isalnum(text[s]))
							++s;
						ioState = DOCTYPE_E;
					}
				}
				else if (c == '>')
				{
					SetStyle(s, kLTagColor);
					ioState = START;
					s = i;
				}
				else if (c == 0 or c == '\n')
				{
					SetStyle(s, kLTagColor);
					leave = true;
				}
				break;
			
			case DOCTYPE_E:
				if (c == '>')
				{
					SetStyle(s, kLTagColor);
					s = i;
					ioState = DOCTYPE;
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
		
		if (c == 0 or c == '\n')
		{
			if (not leave)
				PRINT(("hey!"));
			break;
		}
	}
	
	if (close) ioState |= 0x1000;
}

namespace
{
	
typedef MTextBuffer::const_iterator	MTextPtr;
	
bool name_is_same(
	MTextPtr			inA,
	MTextPtr			inB)
{
	bool same = (isalpha(*inA) or *inA == '_' or *inA == ':') and *inA == *inB;
	
	while (same)
	{
		++inA;
		++inB;
	
		bool isNameCharA = 
			isalnum(*inA) or *inA == '.' or *inA == '-' or *inA == ':' or *inA == '_';

		bool isNameCharB = 
			isalnum(*inB) or *inB == '.' or *inB == '-' or *inB == ':' or *inB == '_';
		
		if (not isNameCharA and not isNameCharB)
			break;
		
		same = *inA == *inB;	 
	}
	
	return same;
}

bool parse_tag(
	MTextPtr&			ioText,
	uint32&				ioBegin,
	uint32&				ioEnd);

bool parse_content(
	MTextPtr&			ioText,
	uint32&				ioBegin,
	uint32&				ioEnd)
{
	bool result = false;

	uint32 begin = ioText.GetOffset();

	while (*ioText)
	{
		if (*ioText == '<')
		{
			uint32 tagStart = ioText.GetOffset();
			
			if (*(ioText + 1) == '/')
				break;

			if (ioText == "<!--")		// comment
			{
				ioText += 4;
				while (*ioText and not (ioText == "--"))
					++ioText;
				ioText += 2;
				
				while (*ioText and *ioText != '>')
					++ioText;
				
				if (tagStart <= ioBegin and ioText.GetOffset() >= ioEnd)
				{
					ioBegin = tagStart;
					ioEnd = ioText.GetOffset();
					result = true;
					break;
				}

				continue;
			}

			if (ioText == "<![CDATA[")	// CDATA
			{
				ioText += 9;
				while (*ioText and not (ioText == "]]>"))
					++ioText;
				ioText += 3;
				
				if (tagStart <= ioBegin and ioText.GetOffset() >= ioEnd)
				{
					ioBegin = tagStart;
					ioEnd = ioText.GetOffset();
					result = true;
					break;
				}

				continue;
			}
			
			if (*(ioText + 1) == '?' or *(ioText + 1) == '!')
			{
				ioText += 2;
				while (*ioText and *ioText != '>')
					++ioText;
				++ioText;
				
				if (tagStart <= ioBegin and ioText.GetOffset() >= ioEnd)
				{
					ioBegin = tagStart;
					ioEnd = ioText.GetOffset();
					result = true;
					break;
				}

				continue;
			}
			
			if (parse_tag(ioText, ioBegin, ioEnd))
			{
				result = true;
				break;
			}

			assert(*ioText == 0 or *(ioText - 1) == '>');

			continue;
		}
		
		++ioText;
	}

	if (not result)
	{
		if (begin <= ioBegin and ioText.GetOffset() >= ioEnd)
		{
			ioBegin = begin;
			ioEnd = ioText.GetOffset();
			result = true;
		}
	}
	
	return result;
}

bool parse_tag(
	MTextPtr&			ioText,
	uint32&				ioBegin,
	uint32&				ioEnd)
{
	bool result = false;

	uint32 begin = ioText.GetOffset();
	
	assert(*ioText == '<');
	assert(*(ioText + 1) != '/');
	
	MTextPtr name = ioText + 1;
	
	while (*ioText)
	{
		++ioText;

		if (*ioText == '/') 	// just to be sure
			return false;
		
		while (*ioText and *ioText != '/' and *ioText != '>')
		{
			if (*ioText == '"')
			{
				++ioText;
				while (*ioText and *ioText != '"')
					++ioText;
				++ioText;
				continue;
			}
			
			if (*ioText == '\'')
			{
				++ioText;
				while (*ioText and *ioText != '\'')
					++ioText;
				++ioText;
				continue;
			}
			
			++ioText;
		}
		
		// empty tag perhaps?
		if (*ioText == '/')
		{
			++ioText;
			while (*ioText and *ioText != '>')
				++ioText;
			++ioText;
			
			if (begin <= ioBegin and ioText.GetOffset() >= ioEnd)
			{
				ioBegin = begin;
				ioEnd = ioText.GetOffset();
				result = true;
				break;
			}
		}
		
		if (*ioText == '>')
		{
			++ioText;
			result = parse_content(ioText, ioBegin, ioEnd);
			if (not result and *ioText == '<' and *(ioText + 1) == '/')
			{
				// check the name, we should now be located at end tag
				if (not name_is_same(ioText + 2, name))
					throw 1;
				
				ioText += 2;
				while (*ioText and *ioText != '>')
					++ioText;
				++ioText;
				
				if (begin <= ioBegin and ioText.GetOffset() >= ioEnd)
				{
					ioBegin = begin;
					ioEnd = ioText.GetOffset();
					result = true;
				}
			}
		}
		
		break;
	}

	return result;
}

void find_open_name(
	MTextPtr&			inText,
	MTextPtr			inEnd,
	string&				outName)
{
	while (inText < inEnd)
	{
		// skip to the first tag
		while (inText < inEnd and *inText != '<')
			++inText;
		
		// break out if we've hit an end tag
		if (*inText == '<' and *(inText + 1) == '/')
			break;
		
		++inText;
		
		// cdata?
		if (inText == "![CDATA[")
		{
			inText += 9;
			while (inText < inEnd and not (inText == "]]>"))
				++inText;
			inText += 3;
			
			continue;
		}

		// comment?
		if (inText == "!--")
		{
			inText += 4;
			while (inText < inEnd and not (inText == "--"))
				++inText;
			inText += 2;
			
			while (inText < inEnd and *inText != '>')
				++inText;
			
			continue;
		}
		
		if (*inText == '?' or *inText == '!')
		{
			++inText;
			while (inText < inEnd and *inText != '>')
				++inText;
			++inText;
			
			continue;
		}
		
		// collect the name
		string name;
		MTextPtr namePtr = inText;
		
		if (isalpha(*inText) or *inText == ':' or *inText == '_')
		{
			name += *inText++;
			while (isalnum(*inText) or *inText == '.' or *inText == '-' or *inText == ':' or *inText == '_')
				name += *inText++;
		}
		
		while (inText < inEnd and *inText != '/' and *inText != '>')
		{
			if (*inText == '"')
			{
				++inText;
				while (inText < inEnd and *inText != '"')
					++inText;
				++inText;
				continue;
			}
			
			if (*inText == '\'')
			{
				++inText;
				while (inText < inEnd and *inText != '\'')
					++inText;
				++inText;
				continue;
			}
			
			++inText;
		}
		
		if (*inText == '/')
		{
			++inText;
			while (inText < inEnd and *inText != '>')
				++inText;
			continue;
		}
		
		if (inText >= inEnd)
			break;

		++inText;
		// OK, so we're in the content section of <name> 
		
		string savedName = outName;
		outName = name;
		
		find_open_name(inText, inEnd, outName);
		
		if (inText >= inEnd)
			break;
		
		if (*inText != '<' or *(inText + 1) != '/')
			continue;
		
		if (not name_is_same(namePtr, inText + 2))
			throw 1;

		inText += 2 + name.length();
		while (inText < inEnd and *inText != '>')
			++inText;
		
		outName = savedName;
	}
}

}

bool MLanguageXML::Balance(
	const MTextBuffer&	inText,
	uint32&				ioOffset,
	uint32&				ioLength)
{
	uint32 begin = ioOffset;
	uint32 end = ioOffset + ioLength;
	
	bool result = false;
	
	try
	{
		MTextPtr text = inText.begin();

		if (parse_content(text, begin, end))
		{
			ioOffset = begin;
			ioLength = end - begin;
			if (ioLength > inText.GetSize() - ioOffset)
				ioLength = inText.GetSize() - ioOffset;
			result = true;
		}
	}
	catch (...) {}
	
	return result;
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
	bool result = false;
	MTextPtr text = inText.begin();
	
	if (inOffset > 0 and text[inOffset - 1] == '>')
	{
		--inOffset;
		while (inOffset >= 0 and
			text[inOffset] != '<' and text[inOffset] != '/' and
			text[inOffset] != '\n')
		{
			if (text[inOffset] == '\'')
			{
				--inOffset;
				while (inOffset >= 0 and text[inOffset] != '\'')
					--inOffset;
				--inOffset;
				continue;
			}

			if (text[inOffset] == '"')
			{
				--inOffset;
				while (inOffset >= 0 and text[inOffset] != '"')
					--inOffset;
				--inOffset;
				continue;
			}
			
			--inOffset;
		}
		
		result =
			inOffset >= 0 and
			text[inOffset] == '<' and
			not ((text + inOffset) == "<![CDATA[" or (text + inOffset) == "<!--");
	}
	
	return result;
}

bool
MLanguageXML::IsSmartIndentCloseChar(
	wchar_t				inChar,
	const MTextBuffer&	inText,
	uint32&				ioOpenOffset)
{
	bool result = false;
	MTextPtr text = inText.begin();
	
	if (inChar == '>' and ioOpenOffset > 0 and
		text[ioOpenOffset] != '/' and text[ioOpenOffset] != '\n')
	{
		--ioOpenOffset;
		while (ioOpenOffset >= 0 and text[ioOpenOffset] != '<' and text[ioOpenOffset] != '\n')
			--ioOpenOffset;
		
		result = text[ioOpenOffset] == '<';
	}
	
	return result;
}

bool
MLanguageXML::IsAutoCompleteChar(
	wchar_t				inChar,
	const MTextBuffer&	inText,
	uint32				inOffset,
	string&				outCompletionText,
	int32&				outCaretDelta)
{
	bool result = false;

	try
	{
		MTextPtr text = inText.begin();
	
		if (inChar == '/' and inOffset > 0 and inText[inOffset - 1] == '<')
		{
			find_open_name(text, text + inOffset - 1, outCompletionText);
			if (not outCompletionText.empty())
			{
				result = true;
				outCompletionText += '>';
				outCaretDelta = outCompletionText.length();
			}
		}
		else if (inChar == '>' and inOffset > 0)
		{
			string name;
			
			find_open_name(text, text + inOffset + 1, name);
			if (not name.empty())
			{
				result = true;
				outCompletionText = "</";
				outCompletionText += name;
				outCompletionText += '>';
				outCaretDelta = 0;
			}
		}
	}
	catch (...) {}
	
	return result;
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

	if (FileNameMatches("*.xml;*.xslt;*.plist;*.xsd;*.dtd;*.xpgt;*.xhtml", inFile))
		result += 90;

	if (FileNameMatches("*.ent;*.dtd", inFile))	// maybe an external DTD file?
		result += 50;

	if (FileNameMatches("*.mod", inFile))	// I've seen those as well for external DTD
		result += 5;

	if (result < 90)
	{
		const char kXMLString[] = "<?xml";
		char s[8] = {};
		uint32 l = 0;
		
		MTextBuffer::iterator txt(inText.begin());
		while (*txt and l < sizeof(kXMLString) - 1)
		{
			if (not isspace(*txt))		// this is wrong, <?xml should be the first characters
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
	
	if (result < 50)
	{
		const char kXMLString[] = "<!--";
		char s[8] = {};
		uint32 l = 0;
		
		MTextBuffer::iterator txt(inText.begin());
		while (*txt and l < sizeof(kXMLString) - 1 and txt.GetOffset() < 1000)
		{
			if (*txt == kXMLString[l])
				++l;
			else
				l = 0;
			++txt;
		}
		
		if (strcmp(kXMLString, s) == 0)
			result += 50;
	}
	
	return result;
}

bool MLanguageXML::Softwrap() const
{
	return false;
}

uint16 MLanguageXML::GetInitialState(
	const string&		inFile,
	MTextBuffer&		inText)
{
	uint16 result = START;
	if (FileNameMatches("*.ent;*.dtd", inFile))
		result = DOCTYPE;
	return result;
}

