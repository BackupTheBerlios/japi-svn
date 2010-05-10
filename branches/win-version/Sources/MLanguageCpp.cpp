//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MLanguageCpp.cpp 145 2007-05-11 14:08:02Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 15:26:34
*/

#include "MJapi.h"

#include "MLanguageCpp.h"
#include "MTextBuffer.h"
#include "MFile.h"

#include <stack>

using namespace std;

typedef MTextBuffer::const_iterator	MTextPtr;

#define kLUser1 kLKeyWordColor

enum {
	START, IDENT, OTHER, COMMENT, LCOMMENT, STRING,
	CHAR_CONST, LEAVE, PRAGMA1, PRAGMA2,
	INCL1, INCL2, INCL3
};

const char
	kInclude[] = "include",
	kCommentPrefix[] = "//";

bool gAddPrototypes = false;

MLanguageCpp::MLanguageCpp()
{
}

void
MLanguageCpp::Init()
{
	const char* keywords[] = {
		"break", "asm", "auto", "bool", "case", "catch", "char", "class",
		"const", "const_cast", "continue", "default", "delete",
		"do", "double", "dynamic_cast", "else", "enum", "explicit",
		"export", "extern", "false", "float", "for", "friend", "goto",
		"if", "inline", "int", "long", "mutable", "namespace", "new",
		"operator", "private", "protected", "public", "register",
		"reinterpret_cast", "return", "short", "signed", "sizeof",
		"static", "static_cast", "struct", "switch",
		"template", "this", "throw", "true", "try", "typedef",
		"typeid", "typename", "union", "unsigned", "using", "virtual",
		"void", "volatile", "wchar_t", "while", "and", "and_eq",
		"bitand", "bitor", "compl", "not", "or", "or_eq", "xor", "xor_eq",
		"not_eq",
		// c++0x
		"nullptr", "constexpr", "decltype", "static_assert",
		nil
	};
	
	const char* preprocessorSymbols[] = {
		"define", "else", "elif", "endif", "error", "if", "ifdef",
		"ifndef", "include", "pragma", "undef", nil
	};
	
	const char* my_keywords[] = {
		"int8", "uint8", "int16", "uint16", "int32", "uint32", "int64", "uint64",
		"nil",
		nil
	};
	
	AddKeywords(keywords);
	AddKeywords(preprocessorSymbols);
	AddKeywords(my_keywords);
	GenerateDFA();
}

void
MLanguageCpp::StyleLine(
	const MTextBuffer&	inText,
	uint32				inOffset,
	uint32				inLength,
	uint16&				ioState)
{
	MTextPtr text = inText.begin() + inOffset;
	uint32 i = 0, s = 0, kws = 0, esc = 0;
	bool leave = false;
	
	if (ioState == COMMENT or ioState == LCOMMENT)
		SetStyle(0, kLCommentColor);
	else
		SetStyle(0, kLTextColor);
	
	if (inLength == 0)
		return;
	
	uint32 maxOffset = inOffset + inLength;
	int includeState = 0;
	
	while (not leave and text.GetOffset() < maxOffset)
	{
		wchar_t c = 0;
		if (i < inLength)
			c = text[i];
		++i;
		
		switch (ioState)
		{
			case START:
				if (c == '#')
				{
					ioState = PRAGMA1;
				}
				else if (isalpha(c) or c == '_')
				{
					kws = Move(c, 1);
					ioState = IDENT;
				}
				else if (c == '/' and text[i] == '*')
				{
					++i;
					ioState = COMMENT;
				}
				else if (c == '/' and text[i] == '/')
				{
					++i;
					ioState = LCOMMENT;
				}
				else if (c == '"')
					ioState = STRING;
				else if (c == '\'')
				{
					ioState = CHAR_CONST;
				}
				else if (c == '\n' or c == 0)
					leave = true;
					
				if (leave or (ioState != START and s < i))
				{
					SetStyle(s, kLTextColor);
					s = i - 1;
				}
				break;
			
			case COMMENT:
				if ((s == 0 or i > s + 1) and c == '*' and text[i] == '/')
				{
					SetStyle(s - 1, kLCommentColor);
					s = i + 1;
					ioState = START;
				}
				else if (c == 0 or c == '\n')
				{
					SetStyle(s - 1, kLCommentColor);
					leave = true;
				}
				break;

			case LCOMMENT:
				SetStyle(s - 1, kLCommentColor);
				leave = true;
				if (inText.GetChar(inOffset + inLength - 1) == '\n')
					ioState = START;
				break;
			
			case IDENT:
				if (not isalnum(c) and c != '_')
				{
					int kwc;

					if (i > s + 1 and (kwc = IsKeyWord(kws)) != 0)
					{
						if (kwc & 1)
							SetStyle(s, kLKeyWordColor);
						else if (kwc & 4)
							SetStyle(s, kLUser1);
//						else if (kwc & 8)
//							SetStyle(s, kLUser2);
//						else if (kwc & 16)
//							SetStyle(s, kLUser3);
//						else if (kwc & 32)
//							SetStyle(s, kLUser4);
						else
							SetStyle(s, kLTextColor);
					}
					else
					{
						SetStyle(s, kLTextColor);
					}
					
					s = --i;
					ioState = START;
				}
				else if (kws)
					kws = Move(c, kws);
				break;
			
			case PRAGMA1:
				if (c == ' ' or c == '\t')
					;
				else if (islower(c))
				{
					kws = Move(c, 1);
					ioState = PRAGMA2;
					if (c == kInclude[0])
						includeState = 1;
					else
						includeState = 0;
				}
				else
				{
					SetStyle(s, kLTextColor);
					s = --i;
					ioState = START;
				}	
				break;
			
			case PRAGMA2:
				if (not islower(c))
				{
					int kwc;

					if (i > s + 2 and (kwc = IsKeyWord(kws)) != 0)
					{
						if (kwc & 2)
							SetStyle(s, kLPreProcessorColor);
						else if (kwc & 4)
							SetStyle(s, kLUser1);
//						else if (kwc & 8)
//							SetStyle(s, kLUser2);
//						else if (kwc & 16)
//							SetStyle(s, kLUser3);
//						else if (kwc & 32)
//							SetStyle(s, kLUser4);
						else
							SetStyle(s, kLTextColor);
					}
					else
					{
						SetStyle(s, kLTextColor);
					}
					
					if (includeState == 7)
						ioState = INCL1;
					else
						ioState = START;

					s = --i;
				}
				else if (kws)
				{
					kws = Move(c, kws);
					
					if (includeState and c == kInclude[includeState])
						++includeState;
					else
						includeState = 0;
				}
				break;
			
			case INCL1:
				if (c == '"')
					ioState = INCL2;
				else if (c == '<')
					ioState = INCL3;
				else if (c != ' ' and c != '\t')
				{
					ioState = START;
					--i;
				}
				break;
			
			case INCL2:
				if (c == '"')
				{
					SetStyle(s, kLStringColor);
					s = i;
					ioState = START;
				}
				else if (c == '\n' or c == 0)
				{
					SetStyle(s, kLTextColor);
					leave = true;
					ioState = START;
				}	
				break;
			
			case INCL3:
				if (c == '>')
				{
					SetStyle(s, kLStringColor);
					s = i;
					ioState = START;
				}
				else if (c == '\n' or c == 0)
				{
					SetStyle(s, kLTextColor);
					leave = true;
					ioState = START;
				}	
				break;
			
			case STRING:
				if (c == '"' and not esc)
				{
					SetStyle(s, kLStringColor);
					s = i;
					ioState = START;
				}
				else if (c == '\n' or c == 0)
				{
					if (text[i - 2] == '\\' and text[i - 3] != '\\')
					{
						SetStyle(s, kLStringColor);
					}
					else
					{
						SetStyle(s, kLTextColor);
						ioState = START;
					}
					
					s = inLength;
					leave = true;
				}
				else
					esc = not esc and (c == '\\');
				break;
			
			case CHAR_CONST:
				if (c == '\t' or c == '\n' or c == 0)	// don't like this
				{
					SetStyle(s, kLTextColor);
					s = i;
					ioState = START;
				}
				else if (c == '\'' and not esc)
				{
					SetStyle(s, kLCharConstColor);
					s = i;
					ioState = START;
				}
				else
				{
					esc = not esc and (c == '\\');
				}
				break;
			
			default:	// error condition, gracefully leave the loop
				leave = true;
				break;
		}
	}
}

static
MTextPtr
skip(
	MTextPtr	inText)
{
	while (*inText)
	{
		switch (*inText)
		{
			case '\'':
				while (*++inText)
				{
					if (*inText == '\'')
						break;
					if (*inText == '\\' and inText[1])
						inText++;
				}
				break;
			
			case '"':
				while (*++inText)
				{
					if (*inText == '"')
						break;
					if (*inText == '\\' and inText[1])
						inText++;
				}
				break;
				
			case '/':
				if (inText[1] == '*')
				{
					inText += 2;
					while (*inText and not (*inText == '*' and inText[1] == '/'))
						inText++;
				}
				else if (inText[1] == '/')
				{
					inText += 2;
					while (*inText and *inText != '\n')
						inText++;
				}
				break;
			
			case '{':
			case '[':
			case '(':
			case ')':
			case ']':
			case '}':
				return inText;
		}
		inText++;
	}
	
	return inText;
}

bool
MLanguageCpp::Balance(
	const MTextBuffer&	inText,
	uint32&				ioOffset,
	uint32&				ioLength)
{
	MTextPtr txt = inText.begin();
	
	uint32 size = inText.GetSize();
	
	stack<int32> bls, sbls, pls;

	uint32 m = ioOffset + ioLength;
	if (m > size)
		m = size;
	
	while (txt.GetOffset() < m)
	{
		switch (*txt)
		{
			case '{':	bls.push(txt.GetOffset());			break;
			case '[':	sbls.push(txt.GetOffset());			break;
			case '(':	pls.push(txt.GetOffset());			break;
			case '}':	if (not bls.empty()) bls.pop();		break;
			case ']':	if (not sbls.empty()) sbls.pop();	break;
			case ')':	if (not pls.empty()) pls.pop();		break;
		}
		txt = skip(txt + 1);
	}
	
	char ec = 0, oc = 0;
	stack<int32> *s = nil;
	
	int db, dsb, dp;
	
	db = bls.empty() ? -1 : static_cast<int32>(ioOffset) - bls.top();
	dsb = sbls.empty() ? -1 : static_cast<int32>(ioOffset) - sbls.top();
	dp = pls.empty() ? -1 : static_cast<int32>(ioOffset) - pls.top();
	
	if (db < 0 and dsb < 0 and dp < 0)
		return false;
	
	if (db >= 0 and (dsb < 0 or db < dsb) and (dp < 0 or db < dp))
	{
		oc = '{';
		ec = '}';
		s = &bls;
	}
	
	if (dsb >= 0 and (db < 0 or dsb < db) and (dp < 0 or dsb < dp))
	{
		oc= '[';
		ec = ']';
		s = &sbls;
	}
	
	if (dp >= 0 and (dsb < 0 or dp < dsb) and (db < 0 or dp < db))
	{
		oc = '(';
		ec = ')';
		s = &pls;
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
MLanguageCpp::IsBalanceChar(
	wchar_t				inChar)
{
	return inChar == ')' or inChar == ']' or inChar == '}';
}

bool
MLanguageCpp::IsSmartIndentLocation(
	const MTextBuffer&	inText,
	uint32				inOffset)
{
	return inOffset > 0 and inText.GetChar(inOffset - 1) == '{';
}

bool
MLanguageCpp::IsSmartIndentCloseChar(
	wchar_t				inChar,
	const MTextBuffer&	inText,
	uint32&				ioOpenOffset)
{
	return inChar == '}';
}

void
MLanguageCpp::CommentLine(
	string&				ioLine)
{
	ioLine.insert(0, kCommentPrefix);
}

void
MLanguageCpp::UncommentLine(
	string&				ioLine)
{
	if (ioLine.substr(0, 2) == "//")
		ioLine.erase(0, 2);
}

uint32
MLanguageCpp::MatchLanguage(
	const std::string&	inFile,
	MTextBuffer&		inText)
{
	uint32 result = 0;
	if (FileNameMatches(
		"*.c;*.h;*.cpp;*.cp;*.cc;*.hh;*.hpp;*.inl", inFile))
	{
		result += 90;
	}
	return result;
}

bool MLanguageCpp::Softwrap() const
{
	return false;
}

// ------------------------------------------------------------------
// 
// The language parser
//

static inline bool
isidentf(
	wchar_t		inChar)
{
	return isalpha(inChar) or inChar == '_';
}

static inline bool
isident(
	wchar_t		inChar)
{
	return isalnum(inChar) or inChar == '_';
}

MTextPtr
MLanguageCpp::Identifier(
	MTextPtr		inText,
	MNamedRange&	outNamedRange,
	bool			inGetTemplateParameters,
	bool			inGetNameSpaces)
{
	bool destructor = false;
	
	uint32 start = inText.GetOffset();

	while (isidentf(*inText))
	{
		if (inText == "operator")	// special case
		{
			outNamedRange.name += "operator";
			inText = ParseComment(inText + 8);

			if (*inText == '(')		// operator()
			{
				inText = ParseParenthesis(inText + 1, '(');
				outNamedRange.selectTo = inText.GetOffset();
				inText = ParseComment(inText);
				outNamedRange.name += "()";
			}
			else
			{
				while (*inText != 0 and *inText != '(')
				{
					if (isidentf(*inText))
					{
						outNamedRange.name += ' ';
						inText = Identifier(inText, outNamedRange);
					}
					else
						outNamedRange.name += *inText++;

					outNamedRange.selectTo = inText.GetOffset();
					inText = ParseComment(inText);
				}
			}

			break;					// leave loop since operator should be last
		}

		do
		{
			outNamedRange.name += *inText++;
		}
		while (isident(*inText));
		outNamedRange.selectTo = inText.GetOffset();
		
		inText = ParseComment(inText);
		
		if (*inText == '<' and inGetTemplateParameters)
		{
			outNamedRange.name += '<';
			
			inText = ParseComment(inText + 1);
			
			while (isidentf(*inText))
			{
				inText = Identifier(inText, outNamedRange);
				
				if (*inText == ',')
				{
					outNamedRange.name += ',';
					inText = ParseComment(inText + 1);
				}
			}
	
			if (*inText == '>')
			{
				outNamedRange.name += '>';
				outNamedRange.selectTo = inText.GetOffset();
				inText = ParseComment(inText + 1);
			}
		}
		
		if (not destructor and inText == "::" and inGetNameSpaces)
		{
			MTextPtr p = ParseComment(inText + 2);
			
			if (*p == '~')
			{
				++p;
				destructor = true;
			}
			
			if (isidentf(*p))
			{
				outNamedRange.name += "::";
				if (destructor)
					outNamedRange.name += '~';
				inText = p;
				continue;
			}
		}
		
		break;
	}
	
	outNamedRange.selectFrom = start;
	
	return inText;
}

MTextPtr
MLanguageCpp::SkipToChar(
	MTextPtr	inText,
	char		inChar,
	bool		inIgnoreEscapes)
{
	while (*inText)
	{
		if (*inText == inChar)
		{
			++inText;
			break;
		}

		if (not inIgnoreEscapes and *inText == '\\' and *(inText + 1) != 0)
			++inText;

		++inText;
	}

	return inText;
}

MTextPtr
MLanguageCpp::ParseComment(
	MTextPtr	inText,
	bool		inStripPreProcessor)
{
	do
	{
		while (isspace (*inText))
			++inText;
		
		if (*inText == '/')
		{
			if (*(inText + 1) == '*')
			{
				inText += 2;

				do
				{
					inText = SkipToChar(inText, '*', true);
					if (*inText == '/')
					{
						++inText;
						break;
					}
				}
				while (*inText != '\0');
			}
			else if (*(inText + 1) == '/')
			{
				inText = SkipToChar(inText, '\n');
			}
			else
				break;
		}
		else if (*inText == '#' and *(inText + 1) != '#' and inStripPreProcessor)
			inText = SkipToChar(inText, '\n');
		else
			break;
	}
	while (*inText);

	return inText;
}

MTextPtr
MLanguageCpp::ParseParenthesis(
	MTextPtr	inText,
	char		inOpenChar)
{
	int c;
	char close = 0;
	
	switch (inOpenChar)
	{
		case '(':	close = ')'; break;
		case '{':	close = '}'; break;
		case '[':	close = ']'; break;
//		default:	ASSERT(false); return inText;
	}

	for (;;)
	{
		inText = ParseComment(inText);

		c = *inText++;
		
		if (c == '\'') 
		{
			inText = SkipToChar(inText, '\'');
			continue;
		}
		
		if (c == '"') 
		{
			inText = SkipToChar(inText, '"');
			continue;
		}
		
		if (c == '#') 
		{
			inText = SkipToChar(inText, '\n');
			continue;
		}
		
		if (c == inOpenChar)
		{
			inText = ParseParenthesis(inText, inOpenChar);
			continue;
		}
		
		if (c == close)
			return inText;
		
		if (c == '\0')
			return inText - 1;
	}
}

MTextPtr
MLanguageCpp::ParsePreProcessor(
	MTextPtr			inText,
	MNamedRange&		outRange,
	MIncludeFileList&	outIncludeFiles)
{
	string name;

	while (isspace(*inText))
		++inText;
	
	if (inText == "include")
	{
		inText += 7;

		while (isspace(*inText))
			++inText;

		if (*inText == '"')
		{
			++inText;
			
			while (*inText and *inText != '"' and *inText != '\n')
				name += *inText++;
			
			MIncludeFile file = { name, true };
			outIncludeFiles.push_back(file);
		}
		else if (*inText == '<')
		{
			++inText;
			
			while (*inText and *inText != '>' and *inText != '\n')
				name += *inText++;

			MIncludeFile file = { name, false };
			outIncludeFiles.push_back(file);
		}
	}
	else if (inText == "pragma")
	{
		inText += 6;
		
		while (isspace(*inText))
			++inText;
		
		if (inText == "mark")
		{
			inText += 4;
			
			while (isspace(*inText))
				++inText;
			
			MNamedRange ns;
			ns.selectFrom = ns.begin = inText.GetOffset();

			while (*inText != 0 and *inText != '\n')
				ns.name += *inText++;
			
			ns.end = inText.GetOffset();
			outRange.subrange.push_back(ns);
		}
	}

	if (*inText != '\n')
		inText = SkipToChar(inText, '\n');
	
	return inText;
}

MTextPtr
MLanguageCpp::ParseIdentifier(
	MTextPtr		inText,
	MNamedRange&	outRange)
{
	MTextPtr start = inText;
	bool destructor = false;

	MNamedRange ns;
	ns.selectFrom = ns.begin = inText.GetOffset() - 1;

	if (*inText == '~')
	{
		destructor = true;
		ns.name = '~';
		inText = Identifier(inText + 1, ns, true, false);
	}
	else
		inText = Identifier(inText, ns);
	
	if (*inText == '(')
	{
		ns.selectTo = inText.GetOffset();
		
		inText = ParseComment(ParseParenthesis(inText + 1, '('));
		
		while (*inText != 0 and *inText != ';' and *inText != '{')
		{
			if (isidentf(*inText))
			{
				while (isident(*inText))
					++inText;
				inText = ParseComment(inText);
				continue;
			}
			
			if (*inText == ':')
			{
				inText = SkipToChar(inText, '{') - 1;
				continue;
			}
			
			inText = ParseComment(inText + 1);
		}
		
		// so if *inText is now a '{' we are at the beginning of a function body
		if (*inText == '{' or (gAddPrototypes and *inText == ';'))
		{
			ns.name += "()";
			
			if (*inText == '{')
				inText = ParseParenthesis(inText + 1, '{');
			
			ns.end = inText.GetOffset();
			outRange.subrange.push_back(ns);
		}
	}
	
	return inText;
}

MTextPtr MLanguageCpp::ParseType(
	MTextPtr			inText,
	MNamedRange&		ioNameSpace)
{
	MTextPtr trackBack = inText;
	
	if (inText == "class" or
		inText == "struct" or
		inText == "union" or
		inText == "enum")
	{
		MNamedRange ns;
		ns.begin = inText.GetOffset();
		
		while (*inText)
		{
			while (isalpha(*inText))	// skip keyword
				++inText;
			inText = ParseComment(inText);
			
			inText = Identifier(inText, ns);
			
			if (*inText == ':')	// inheritance
				inText = SkipToChar(inText + 1, '{') - 1;
			
			if (*inText == '{')
			{
				inText = ParseParenthesis(inText + 1, '{');
				ns.end = inText.GetOffset();
				ioNameSpace.subrange.push_back(ns);

				inText = ParseComment(inText);
				
				while (isidentf(*inText))
				{
					ns.begin = inText.GetOffset();
					inText = Identifier(inText, ns);
					ns.end = inText.GetOffset();
					
					ioNameSpace.subrange.push_back(ns);
					
					while (*inText == ',' or *inText == '*')
						inText = ParseComment(inText + 1);
				}
				
				return inText;
			}
			else if (*inText == '(')
				break;
			else if (*inText == ';')
			{
				ns.end = inText.GetOffset();
				ioNameSpace.subrange.push_back(ns);

				return inText;
			}
			
			++inText;
		}
	}
	
	return ParseIdentifier(trackBack, ioNameSpace);
}

#pragma mark -

MTextPtr
MLanguageCpp::ParseNameSpace(
	MTextPtr			inText,
	MNamedRange&		outRange,
	MIncludeFileList&	outIncludeFiles)
{
	outRange.begin = inText.GetOffset();
	
	while (*inText)
	{
		inText = ParseComment(inText, false);
		
		switch (*inText)
		{
			case 0:
				outRange.end = inText.GetOffset();
				return inText;
			case '\'':
				inText = SkipToChar(inText + 1, '\'');
				break;
			case '"':
				inText = SkipToChar(inText + 1, '"');
				break;
			case '(':
			case '{':
			case '[':
				inText = ParseParenthesis(inText + 1, *inText);
				break;
			case '#':
				inText = ParsePreProcessor(inText + 1, outRange, outIncludeFiles);
				break;
			case '}':
				outRange.end = inText.GetOffset() + 1;
				return inText + 1;
				break;
			default:

				if (not (isidentf(*inText) or *inText == '~'))
					++inText;
				
				// several special cases
				// typedef first
				else if (inText == "typedef")
				{
					MNamedRange ns;
					
					ns.begin = inText.GetOffset();

					inText = ParseComment(inText + 7);
					
					while (*inText and *inText != ';' and *inText != '(')
					{
						ns.name.clear();
						if (isidentf(*inText))
							inText = Identifier(inText, ns);
						else
							inText = ParseComment(inText + 1);
					}
					
					if (*inText == ';')			// we found a datatype typedef
					{
						ns.end = inText.GetOffset();
						outRange.subrange.push_back(ns);
					}
					else if (*inText == '(')	// we found a function typedef
					{
						inText = ParseComment(inText + 1);
						if (*inText == '*')
						{
							inText = ParseComment(inText + 1);
							ns.name.clear();
							inText = Identifier(inText, ns);
							ns.end = inText.GetOffset();
							
							outRange.subrange.push_back(ns);
							
							inText = SkipToChar(inText, ';');
						}
					}
				}

				// an extern block
				else if (inText == "extern")
				{
					inText = ParseComment(inText + 6);

					if (*inText == '"')
					{
						inText = SkipToChar(inText + 1, '"');
						inText = ParseComment(inText);
						
						if (*inText == '{')
							inText = ParseNameSpace(inText + 1, outRange, outIncludeFiles);
					}
				}
				
				// a plain C++ namespace
				else if (inText == "namespace")
				{
					MNamedRange ns;
					ns.begin = inText.GetOffset();

					inText = ParseComment(inText + 9);
					inText = Identifier(inText, ns);
			
					if (*inText == '{')
					{
						inText = ParseNameSpace(inText + 1, ns, outIncludeFiles);
						outRange.subrange.push_back(ns);
					}
				}

				// a 'using' statement
				else if (inText == "using")
					inText = SkipToChar(inText, ';');

				// some type, maybe a type definition or a function result ... nasty
				else
					inText = ParseType(inText, outRange);
				break;
		}
	}
	
	outRange.end = inText.GetOffset();
	return inText;
}

void
MLanguageCpp::Parse(
	const MTextBuffer&	inText,
	MNamedRange&		outRange,
	MIncludeFileList&	outIncludeFiles)
{
	outRange.name.clear();
	outRange.subrange.clear();
	outIncludeFiles.clear();
	(void)ParseNameSpace(inText.begin(), outRange, outIncludeFiles);
}

