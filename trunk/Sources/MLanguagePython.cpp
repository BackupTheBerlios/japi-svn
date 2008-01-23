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

/*	$Id: MLanguagePython.cpp 95 2006-12-07 14:23:52Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 15:26:34
*/

#include "MJapi.h"

#include "MLanguagePython.h"
#include "MTextBuffer.h"
#include "MSelection.h"
#include "MUnicode.h"
#include "MFile.h"

#include <stack>
#include <cassert>

using namespace std;

enum State
{
	START, IDENT, OTHER, COMMENT, LCOMMENT, STRING1, STRING2, STRING3,
	CHAR_CONST, LEAVE, PRAGMA1, PRAGMA2, PRAGMA3
};

const char
	kCommentPrefix[] = "#";

MLanguagePython::MLanguagePython()
{
}

void
MLanguagePython::Init()
{
	const char* keywords[] = {
		"access", "and", "break", "class", "continue",
		"def", "del", "elif", "else", "except", "exec",
		"finally", "for", "from", "global", "if",
		"import", "in", "is", "lambda", "not", "or", "pass",
		"print", "raise", "return", "try", "while",
		nil
	};
	
	AddKeywords(keywords);
	GenerateDFA();
}

void
MLanguagePython::StyleLine(
	const MTextBuffer&	inText,
	uint32				inOffset,
	uint32				inLength,
	uint16&				ioState)
{
	MTextBuffer::const_iterator text = inText.begin() + inOffset;
	MTextBuffer::const_iterator end = inText.end();
	uint32 i = 0, s = 0, kws = 0, esc = 0;
	bool leave = false;

	switch (ioState)
	{
		case COMMENT:		SetStyle(0, kLCommentColor);	break;
		case LCOMMENT:		SetStyle(0, kLCommentColor);	break;
		case STRING3:		SetStyle(0, kLStringColor);		break;
		default:			SetStyle(0, kLTextColor);		break;
	}
	
	if (inLength == 0)
		return;

	uint32 maxOffset = inOffset + inLength;
	
	while (not leave and i < maxOffset)
	{
		char c = 0;
		if (i < inLength)
			c = text[i];
		++i;
		
		switch (ioState)
		{
			case START:
				if (c == '#')
					ioState = LCOMMENT;
				else if (isalpha(c) or c == '_')
				{
					kws = Move(c, 1);
					ioState = IDENT;
				}
				else if (c == '"')
				{
					if (text[i] == '"' and text[i + 1] == '"')
						ioState = COMMENT;
					else
						ioState = STRING1;
				}
				else if (c == '\'')
				{
					if (text[i] == '\'' and text[i + 1] == '\'')
						ioState = STRING3;
					else
						ioState = STRING2;
				}
				else if (c == '\n' or c == 0)
					leave = true;
					
				if (leave or (ioState != START and s < i))
				{
					SetStyle(s, kLTextColor);
					s = i - 1;
				}
				break;
			
			case IDENT:
				if (not isalnum(c) and c != '_')
				{
					int kwc;

					if (i > s + 1 and (kwc = IsKeyWord(kws)) != 0)
					{
						switch (kwc)
						{
							case 1:	SetStyle(s, kLKeyWordColor); break;
//							case 2:	SetStyle(s, kLUser1); break;
//							case 3:	SetStyle(s, kLUser2); break;
//							case 4:	SetStyle(s, kLUser3); break;
//							case 5:	SetStyle(s, kLUser4); break;
//							default:ASSERT(false);
						}
					}
					else
					{
						SetStyle(s, kLTextColor);
					}
					
					s = --i;
					ioState = START;
				}
				else if (kws)
					kws = Move((int)(unsigned char)c, kws);
				break;
			
			case LCOMMENT:
				SetStyle(s, kLCommentColor);
				leave = true;
				if (text[inLength - 1] == '\n')
					ioState = START;
				break;
			
			case STRING1:
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
			
			case STRING2:
				if (c == '\'' and not esc)
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
			
			case STRING3:
				if (c == '\'' and text[i] == '\'' and text[i + 1] == '\'' and not esc)
				{
					SetStyle(s, kLStringColor);
					s = i + 2;
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
			
			case COMMENT:
				if (c == '"' and text[i] == '"' and text[i + 1] == '"')
				{
					SetStyle(s, kLCommentColor);
					s = i + 2;
					ioState = START;
				}
				else if (c == '\n' or c == 0)
				{
					SetStyle(s, kLCommentColor);
					leave = true;
				}
				break;
			
			default:	// error condition, gracefully leave the loop
				leave = true;
				break;
//
//
//
//			case START:
//				if (c == '#' and (i < 2 or text[i - 2] != '$'))
//					ioState = LCOMMENT;
//				else if (IsAlpha(c) or c == '_')
//				{
//					kws = Move(c, 1);
//					ioState = IDENT;
//					start_regex = true;
//				}
//				else if (c == '=' and isalpha(text[i]))
//					ioState = POD1;
//				else if (c == '=' and text[i] == '~')
//					start_regex = true;
//				else if (c == '!' and text[i] == '~')
//					start_regex = true;
//				else if (c == '"')
//				{
//					ioState = STRING;
//					mc = '"';
//				}
//				else if (c == '\'')
//				{
//					ioState = STRING;
//					mc = '\'';
//				}
//				else if (c == '`')
//				{
//					ioState = STRING;
//					mc = '`';
//				}
//				else if (c == '&')
//					ioState = SCOPE;
//				else if (c == '$')
//					ioState = VAR1;
//				else if (c == '@' or c == '*' or c == '%')
//					ioState = VAR1;
//				else if (c == '<' and text[i] == '<')
//				{
//					ioState = HERE_DOC_0;
//					++i;
//				}
//				else if (c == '/' and start_regex)
//				{
//					mc = '/';
//					ioState = REGEX1;
//				}
//				else if (c == '\n' or c == 0)
//					leave = true;
//				else if (not isspace(c))
//					start_regex = not (isalnum(c) or c == ')');
//					
//				if (leave or (ioState != START and s < i))
//				{
//					SetStyle(s, kLTextColor);
//					s = i - 1;
//				}
//				break;
//			
//			case LCOMMENT:
//				SetStyle(s, kLCommentColor);
//				leave = true;
//				if (text[inLength - 1] == '\n')
//					ioState = START;
//				break;
//			
//			case IDENT:
//				if (not isalnum(c) and c != '_')
//				{
//					int kwc;
//
//					if (i >= s + 1 and (kwc = IsKeyWord(kws)) != 0)
//					{
//						if (kwc & 1)
//							SetStyle(s, kLKeyWordColor);
//						else if (kwc & 2)
//							SetStyle(s, kLKeyWordColor);
////						else if (kwc & 2)
////							SetStyle(s, kLUser1);
////						else if (kwc & 4)
////							SetStyle(s, kLUser2);
////						else if (kwc & 8)
////							SetStyle(s, kLUser3);
////						else if (kwc & 16)
////							SetStyle(s, kLUser4);
//					}
//					else
//						SetStyle(s, kLTextColor);
//					
//					if (i == s + 2 and text[s - 1] != '-' and not isspace(text[s + 1]))
//					{
//						switch (text[s])
//						{
//							case 'm':
//								ioState = QUOTE1;
//								regex = true;
//								s = --i;
//								break;
//
//							case 's':
//								ioState = QUOTE3;
//								regex = true;
//								s = --i;
//								break;
//
//							case 'q':
//								ioState = QUOTE1;
//								regex = false;
//								s = --i;
//								break;
//
//							case 'y':
//								ioState = REGEX2;
//								break;
//
//							default: 
//								ioState = START;
//								break;
//						}
//					}
//					else if (i == s + 3)
//					{
//						if (Equal(text + s, end, "qq") or
//							Equal(text + s, end, "qx"))
//						{
//							ioState = QUOTE1;
//							regex = false;
//							s = --i;
//						}
//						else if (Equal(text + s, end, "qw"))
//						{
//							ioState = QUOTE1;
//							regex = false;
//							s = --i;
//						}
//						else if (Equal(text + s, end, "qr"))
//						{
//							ioState = QUOTE1;
//							regex = true;
//							s = --i;
//						}
//						else if (Equal(text + s, end, "tr"))
//						{
//							ioState = QUOTE3;
//							regex = true;
//							s = --i;
//						}
//						else
//							ioState = START;
//					}
//					else
//					{
//						if (i == s + 4 and Equal(text + s, end, "sub"))// and ci < 2)
//							ioState = SUB1;
//						else
//							ioState = START;
//					}
//					
//					if (ioState == START or ioState == SUB1)
//						s = --i;
//					else if (ioState != QUOTE1 and ioState != QUOTE3)
//					{
//						switch (c)
//						{
//							case '(':	mc = ')'; break;
//							case '{':	mc = '}'; break;
//							case '[':	mc = ']'; break;
//							case '<':	mc = '>'; break;
//							case ' ':	ioState = START; s = --i; break;
//							default:	mc = c; break;
//						}
//						
//						if (ioState != START)
//							s = i - 1;
//					}
//				}
//				else if (kws)
//					kws = Move(c, kws);
//				break;
//			
//			case SUB1:
//				if (IsAlpha(c))
//					ioState = SUB2;
//				else if (not IsSpace(c))
//				{
//					ioState = START;
//					s = --i;
//				}
//				break;
//			
//			case SUB2:
//				if (not isalnum(c) and c != '\'' and c != '_')
//				{
//					SetStyle(s, kLTextColor);
//					ioState = START;
//					s = --i;
//				}
//				break;
//			
//			case QUOTE1:
//			case QUOTE3:
//				if (c == 0 or c == '\n')
//					leave = true;
//				else if (not IsSpace(c))
//				{
//					switch (c)
//					{
//						case '(':	mc = ')'; break;
//						case '{':	mc = '}'; break;
//						case '[':	mc = ']'; break;
//						case '<':	mc = '>'; break;
//						case '\'':	mc = '\''; break;
//						default:	mc = c; break;
//					}
//
//					count = 0;
//					s = i - 1;
//					
//					ioState += 1;
//				}
//				break;
//			
//			case QUOTE2:
//			case QUOTE4:
//				if (c == 0 or c == '\n')
//				{
//					if (regex)
//						SetStyle(s, kLCharConstColor);
//					else
//						SetStyle(s, kLStringColor);
//
//					leave = true;
//				}
//				else if (esc)
//					esc = false;
//				else if (c == mc)
//				{
//					if (count-- == 0)
//					{
//						if (regex)
//							SetStyle(s, kLCharConstColor);
//						else
//							SetStyle(s, kLStringColor);
//
//						s = i;
//						
//						if (ioState == QUOTE2)
//							ioState = START;
//						else if (mc == '}' or mc == ')' or mc == ']' or mc == '>')
//							ioState = QUOTE5;
//						else
//						{
//							ioState = QUOTE2;
//							count = 0;
//						}
//					}
//				}
//				else if (c == '{' and mc == '}' and count < 3)
//					++count;
//				else if (c == '<' and mc == '>' and count < 3)
//					++count;
//				else if (c == '[' and mc == ']' and count < 3)
//					++count;
//				else if (c == '(' and mc == ')' and count < 3)
//					++count;
//				else 
//					esc = (c == '\\');
//				break;
//			
//			case QUOTE5:
//				if (esc)
//					esc = false;
//				else if (c == '{' and mc == '}')
//				{
//					ioState = QUOTE2;
//					count = 0;
//				}
//				else if (c == '<' and mc == '>')
//				{
//					ioState = QUOTE2;
//					count = 0;
//				}
//				else if (c == '[' and mc == ']')
//				{
//					ioState = QUOTE2;
//					count = 0;
//				}
//				else if (c == '(' and mc == ')')
//				{
//					ioState = QUOTE2;
//					count = 0;
//				}
//				else if (c == '\n')
//					leave = true;
//				else if (c == '\\')
//					esc = true;
//				else if (not IsSpace(c))
//				{
//					ioState = START;
//					SetStyle(s, kLTextColor);
//					s = --i;
//				}
//				break;
//			
//			case STRING:
//				if (c == mc and not esc)
//				{
//					SetStyle(s, kLStringColor);
//					s = i;
//					ioState = START;
//				}
//				else if (c == '\n' or c == 0)
//				{
//					SetStyle(s, kLStringColor);
//					s = inLength;
//					leave = true;
//				}
//				else if (esc)
//					esc = false;
//				else
//					esc = (c == '\\');
//				break;
//			
//			case REGEX1:
//				if (c == 0)	// don't like this
//				{
////					SetStyle(s, kLTextColor);
////					i = ++s;
////					ioState = START;
//					SetStyle(s, kLCharConstColor);
//					leave = true;
//				}
//				else if (c == mc and not esc)
//				{
//					SetStyle(s, kLCharConstColor);
//					s = i;
//					ioState = START;
//				}
//				else if (esc)
//					esc = false;
//				else
//					esc = (c == '\\');
//				break;
//			
//			case REGEX2:
//				if (c == 0)	// don't like this
//				{
//					SetStyle(s, kLTextColor);
//					i = ++s;
//					ioState = START;
//				}
//				else if (c == mc and not esc)
//				{
//					if (mc == ')' or mc == '}' or mc == ']' or mc == '>')
//					{
//						switch (text[i])
//						{
//							case '(':	mc = ')'; break;
//							case '{':	mc = '}'; break;
//							case '[':	mc = ']'; break;
//							case '<':	mc = '>'; break;
//							case ' ':	ioState = START; s = --i; break;
//							default:	mc = text[i]; break;
//						}
//						i++;
//					}
//
//					if (ioState != START)
//						ioState = REGEX1;
//				}
//				else if (esc)
//					esc = false;
//				else
//					esc = (c == '\\');
//				break;
//			
//			case SCOPE:
//				if (c == '\'' or (not isalnum(c) and c != '_'))
//				{
//					SetStyle(s, kLTextColor);
//					ioState = START;
//				}
//				break;
//			
//			case VAR1:
//				switch (c)
//				{
//					case '_':
//					case '.':
//					case '/':
//					case ',':
//					case '\\':
//					case '#':
//					case '%':
//					case '=':
//					case '-':
//					case '~':
//					case '|':
//					case '?':
//					case '&':
//					case '`':
//					case '\'':
//					case '+':
//					case '*':
//					case '0':
//					case '1':
//					case '2':
//					case '3':
//					case '4':
//					case '5':
//					case '6':
//					case '7':
//					case '8':
//					case '9':
//					case '[':
//					case ']':
//					case ';':
//					case '!':
//					case '@':
//					case '<':
//					case '>':
//					case '(':
//					case ')':
//					case ':':
//					case '^':
//					case '$':
//						ioState = VAR2;
//						break;
//					
//					case '{':
//						mc = 1;
//						ioState = VAR3;
//						break;
//					
//					case '"':
//						if (text[i] == '"')
//						{
//							++i;
//							ioState = VAR2;
//						}
//						break;
//					
//					default:
//						if (isalnum(c))
//							ioState = VAR2;
//						else
//						{
//							SetStyle(s, kLTextColor);
//							s = --i;
//							ioState = START;
//						}
//						break;
//				}
//				break;
//			
//			case VAR2:
//				if (not isalnum(c) and c != '_')
//				{
//					SetStyle(s, kLPreProcessorColor);
//					s = --i;
//					ioState = START;
//					start_regex = false;
//				}
//				break;
//			
//			case VAR3:
//				if (c == 0 or c == '\n')
//				{
//					SetStyle(s, kLTextColor);
//					ioState = START;
//				}
//				else if (c == '{')
//				{
//					++mc;
//				}
//				else if (c == '}')
//				{
//					if (--mc <= 0)
//					{
//						SetStyle(s, kLPreProcessorColor);
//						s = i;
//						ioState = START;
//						start_regex = false;
//					}
//				}
//				break;
//			
//			case POD1:
//				if (Equal(text + s, end, "=pod") or
//					Equal(text + s, end, "=head1") or
//					Equal(text + s, end, "=head2") or
//					Equal(text + s, end, "=begin") or
//					Equal(text + s, end, "=for") or
//					Equal(text + s, end, "=item") or
//					Equal(text + s, end, "=over") or
//					Equal(text + s, end, "=back"))
//				{
//					ioState = POD2;
//				}
//				else
//				{
//					s = --i;
//					ioState = START;
//				}
//				break;
//			
//			case POD2: {
//				if (c == 0 or c == '\n')
//				{
//					SetStyle(s, kLTextColor);
//					leave = true;
//				}
//				else if (c == '=' and Equal(text + i, end, "cut"))
//				{
//					SetStyle(s, kLTextColor);
//					i += 3;
//					s = i;
//					ioState = START;
//				}
//				break;
//			}
//			
//			case HERE_DOC_0:
//				if (c == 0 or c == '\n')
//				{
//					ioState = START;
//					leave = true;
//					SetStyle(s, kLTextColor);
//				}
//				else if (isalnum(c) or c == '_')
//				{
//					SetStyle(s, kLTextColor);
//					mc = c;
//					count = 1;
//					ioState = HERE_DOC_1;
//				}
//				break;
//
//			case HERE_DOC_1:
//				if (isalnum(c) or c == '_')
//				{
//					MiniCRC(mc, c);
//					++count;
//				}
//				else
//				{
//					ioState = HERE_DOC_END_0;
//					leave = true;
//				}
//				break;
//
//			case HERE_DOC_END_0:
//				SetStyle(s, kLTextColor);
//				if (isalnum(c) or c == '_')
//				{
//					crc = c;
//					ioState = HERE_DOC_END_1;
//				}
//				else
//					leave = true;
//				break;
//			
//			case HERE_DOC_END_1:
//				if (isalnum(c) or c == '_')
//					MiniCRC(crc, c);
//				else if ((c == 0 or c == '\n') and (crc == mc and i - s - 1 == count))
//					ioState = START;
//				else
//				{
//					ioState = HERE_DOC_END_0;
//					leave = true;
//				}
//				break;
//			
//			default:	// error condition, gracefully leave the loop
//				leave = true;
//				break;
		}
	}
}

static
MTextBuffer::const_iterator
skip(
	MTextBuffer::const_iterator	inText)
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
				if (inText == "\"\"\"")
				{
					while (*++inText and not (inText == "\"\"\""))
					{
						if (*inText == '"')
							break;
						if (*inText == '\\' and inText[1])
							inText++;
					}
				}
				else
				{
					while (*++inText)
					{
						if (*inText == '"')
							break;
						if (*inText == '\\' and inText[1])
							inText++;
					}
				}
				break;
				
			case '#':
				++inText;
				while (*inText and *inText != '\n')
					inText++;
				break;
			
			case '[':
			case '(':
			case ')':
			case ']':
				return inText;
		}
		inText++;
	}
	
	return inText;
}

bool
MLanguagePython::Balance(
	const MTextBuffer&	inText,
	uint32&				ioOffset,
	uint32&				ioLength)
{
	MTextBuffer::const_iterator txt = inText.begin();
	
	stack<int32> sbls, pls;
	
	while (txt.GetOffset() < ioOffset + ioLength)
	{
		switch (*txt)
		{
			case '[':	sbls.push(txt.GetOffset());			break;
			case '(':	pls.push(txt.GetOffset());			break;
			case ']':	if (not sbls.empty()) sbls.pop();	break;
			case ')':	if (not pls.empty()) pls.pop();		break;
		}
		txt = skip(txt + 1);
	}
	
	char ec = 0, oc;
	stack<int32> *s;
	
	int dsb, dp;
	
	dsb = sbls.empty() ? -1 : static_cast<int32>(ioOffset) - sbls.top();
	dp = pls.empty() ? -1 : static_cast<int32>(ioOffset) - pls.top();
	
	if (dsb < 0 and dp < 0)
		return false;
	
	if (dsb >= 0 and (dp < 0 or dsb < dp))
	{
		oc= '[';
		ec = ']';
		s = &sbls;
	}
	
	if (dp >= 0 and (dsb < 0 or dp < dsb))
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
MLanguagePython::IsBalanceChar(
	wchar_t				inChar)
{
	return inChar == ')' or inChar == ']';
}

void
MLanguagePython::CommentLine(
	string&				ioLine)
{
	ioLine.insert(0, kCommentPrefix);
}

void
MLanguagePython::UncommentLine(
	string&				ioLine)
{
	if (ioLine.length() >= 1 and ioLine[0] == '#')
		ioLine.erase(0, 1);
}

uint32
MLanguagePython::MatchLanguage(
	const string&		inFile,
	MTextBuffer&		inText)
{
	uint32 result = 0;
	if (FileNameMatches("*.py", inFile))
	{
		result += 90;
	}
	else
	{
		MSelection s;
		
		if (inText.Find(0, "^#!\\s*.*?python.*\\n",
				kDirectionForward, false, true, s) and
			s.GetAnchor() == 0)
		{
			result += 75;
		}			
	}
	return result;
}

bool MLanguagePython::Softwrap() const
{
	return false;
}

//void MLanguagePython::Parse(
//	const MTextBuffer&	inText,
//	MNamedRange&		outRange,
//	MIncludeFileList&	outIncludeFiles)
//{
//	outRange.name.clear();
//	outRange.subrange.clear();
//	outIncludeFiles.clear();
//
//	MTextPtr text(inText.begin());
//
//	outRange.begin = text.GetOffset();
//
//	text = package(text, outRange);
//	
//	while (*text and text == "package")
//	{
//		MNamedRange p;
//		p.begin = text.GetOffset();
//		
//		text = comment(text + 7);
//		
//		p.selectFrom = text.GetOffset();
//
//		text = name_append(text, p.name);
//
//		while (text == "::")
//		{
//			p.name += "::";
//			text = name_append(text + 2, p.name);
//		}
//
//		p.selectTo = text.GetOffset();
//		
//		text = package(text, p);
//		p.end = text.GetOffset();
//		
//		outRange.subrange.push_back(p);
//	}
//
//	outRange.end = text.GetOffset();
//}



