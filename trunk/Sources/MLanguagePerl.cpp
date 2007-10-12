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

/*	$Id: MLanguagePerl.cpp 95 2006-12-07 14:23:52Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 15:26:34
*/

#include "MJapieG.h"

#include "MLanguagePerl.h"
#include "MTextBuffer.h"
#include "MSelection.h"
#include "MUnicode.h"
#include "MFile.h"

#include <stack>
#include <cassert>

using namespace std;

enum State
{
	START, IDENT, OTHER, COMMENT, LCOMMENT, STRING1, STRING2,
	LEAVE, REGEX0, REGEX1, REGEX2, SCOPE,
	VAR1, VAR1a, VAR2, VAR3,
	QUOTE1, QUOTE2, QUOTE3, QUOTE4, QUOTE5,
	SUB1, SUB2, POD1, POD2,
	HERE_DOC_0, HERE_DOC_END, HERE_DOC_EOF,
	STATE_COUNT
};

const char
	kCommentPrefix[] = "#";

MLanguagePerl::MLanguagePerl()
{
}

void
MLanguagePerl::Init()
{
	const char* keywords[] = {
		"continue", "else", "elsif", "do", "for", "foreach",
		"if", "unless", "until", "while", "or", "and",
		"eq", "ne", "lt", "gt", "le", "ge", "cmp", "x",
		"my", "sub", "use", "package", "can", "isa", "format",
		"not", "xor", nil
	};
	
	const char* builtin[] = {
		"accept", "alarm", "atan2", "bind", "binmode", "caller",
		"chdir", "chmod", "chop", "chown", "chroot", "close",
		"closedir", "connect", "cos", "crypt", "dbmclose", "dbmopen",
		"defined", "delete", "die", "do", "dump", "each",
		"eof", "eval", "exec", "exists", "exit", "exp", "fcntl",
		"fileno", "flock", "fork", "getc", "getlogin", "getpeername",
		"getpgrp", "getppid", "getpriority", "getpwnam",
		"gethostbyname", "getnetbyname", "getprotobyname",
		"getpwuid", "getgrgid", "getservbyname", "gethostbyaddr",
		"getnetbyaddr", "getprotobynumber",
		"getservbyport", "getpwent", "getgrent", "gethostent",
		"getnetent", "getprotoent", "getservent",
		"setpwent", "setgrent", "sethostent", "setnetent",
		"setprotoent", "setservent", "endpwent", "endgrent",
		"endhostent", "endnetent", "endprotoent", "endservent",
		"getsockname", "getsockopt", "gmtime",
		"goto", "grep", "hex", "index", "int", "ioctl", "join",
		"keys", "kill", "last", "lc", "length", "link", "listen", "local",
		"localtime", "log", "lstat", "m", "mkdir", "msgctl", "msgget",
		"msgsnd", "msgrcv", "next", "oct", "open", "opendir",
		"ord", "pack", "pipe", "pop", "print", "printf", "push", "q",
		"qq", "qx", "rand", "read", "readdir", "readlink", "recv",
		"redo", "rename", "require", "reset", "return", "reverse",
		"rewinddir", "rindex", "rmdir", "s", "scalar", "seek",
		"seekdir", "select", "semctl", "semget", "semop", "send",
		"setpgrp", "setpriority", "setsockopt", "shift",
		"shmctl", "shmget", "shmread", "shmwrite", "shutdown", "sin",
		"sleep", "socket", "socketpair", "sort", "splice",
		"split", "sprintf", "sqrt", "srand", "stat", "study", "substr",
		"symlink", "syscall", "sysread", "system",
		"syswrite", "tell", "telldir", "time", "tr", "y", "truncate",
		"umask", "undef", "unlink", "unpack", "unshift", "utime",
		"values", "vec", "wait", "waitpid", "wantarray", "warn", "write",
		"abs", "bless", "chomp", "chr", "formline", "glob", "import",
		"lc", "lcfirst", "map", "no", "our", "prototype", "qw",
		"readline", "readpipe", "ref", "sysopen", "tie", "tied",
		"uc", "ucfirst", "untie",
		nil
	};
	
	AddKeywords(keywords);
	AddKeywords(builtin);
	GenerateDFA();
}

void
MLanguagePerl::StyleLine(
	const MTextBuffer&	inText,
	uint32				inOffset,
	uint32				inLength,
	uint16&				ioState)
{
	assert(STATE_COUNT < 32);
	
	MTextBuffer::const_iterator text = inText.begin() + inOffset;
	MTextBuffer::const_iterator end = inText.end();
	uint32 i = 0, s = 0, kws = 0, esc = 0;
	bool leave = false;
	
	if (ioState == COMMENT or ioState == LCOMMENT)
		SetStyle(0, kLCommentColor);
	else
		SetStyle(0, kLTextColor);
	
	if (inLength == 0)
		return;

	// save the match char for multiline regex
	
	char mc = (ioState >> 9) & 0x7f;
	int nest = (ioState >> 5) & 0x03;
	bool interpolate = (ioState & 0x0100) != 0;
	bool regex = (ioState & 0x0080) != 0;
	ioState &= 0x001F;
	
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
				if (c == '#' and (i < 2 or text[i - 2] != '$'))
					ioState = LCOMMENT;
				else if (IsAlpha(c) or c == '_')
				{
					kws = Move(c, 1);
					ioState = IDENT;
				}
				else if (c == '=' and IsAlpha(text[i]))
					ioState = POD1;
				else if (c == '"')
					ioState = STRING1;
				else if (c == '\'')
					ioState = STRING2;
				else if (c == '&')
					ioState = SCOPE;
				else if (c == '$')
					ioState = VAR1;
				else if (c == '@' or c == '*' or c == '%')
					ioState = VAR1a;
				else if (c == '~')
					ioState = REGEX0;
				else if (c == '<' and text[i] == '<')
				{
					ioState = HERE_DOC_0;
					++i;
				}
//				else if (c == '/')
//				{
//					mc = '/';
//					ioState = REGEX1;
//				}
				else if (c == '\n' or c == 0)
					leave = true;
					
				if (leave or (ioState != START and s < i))
				{
					SetStyle(s, kLTextColor);
					s = i - 1;
				}
				break;
			
			case LCOMMENT:
				SetStyle(s, kLCommentColor);
				leave = true;
				if (text[inLength - 1] == '\n')
					ioState = START;
				break;
			
			case IDENT:
				if (not isalnum(c) and c != '_')
				{
					int kwc;

					if (i >= s + 1 and (kwc = IsKeyWord(kws)) != 0)
					{
						if (kwc & 1)
							SetStyle(s, kLKeyWordColor);
						else if (kwc & 2)
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
					else
						SetStyle(s, kLTextColor);
					
					if (i == s + 2 and text[s - 1] != '-' and not isspace(text[s + 1]))
					{
						switch (text[s])
						{
							case 'm':
								ioState = QUOTE1;
								regex = true;
								interpolate = true;
								s = --i;
								break;

							case 's':
								ioState = QUOTE3;
								regex = true;
								interpolate = true;
								s = --i;
								break;

							case 'q':
								ioState = QUOTE1;
								regex = false;
								interpolate = true;
								s = --i;
								break;

							case 'y':
								ioState = REGEX2;
								break;

							default: 
								ioState = START;
								break;
						}
					}
					else if (i == s + 3)
					{
						if (Equal(text + s, end, "qq") or
							Equal(text + s, end, "qx"))
						{
							ioState = QUOTE1;
							regex = false;
							interpolate = true;
							s = --i;
						}
						else if (Equal(text + s, end, "qw"))
						{
							ioState = QUOTE1;
							regex = false;
							interpolate = false;
							s = --i;
						}
						else if (Equal(text + s, end, "qr"))
						{
							ioState = QUOTE1;
							regex = true;
							interpolate = true;
							s = --i;
						}
						else if (Equal(text + s, end, "tr"))
						{
							ioState = QUOTE3;
							regex = true;
							interpolate = false;
							s = --i;
						}
						else
							ioState = START;
					}
					else
					{
						if (i == s + 4 and Equal(text + s, end, "sub"))// and ci < 2)
							ioState = SUB1;
						else
							ioState = START;
					}
					
					if (ioState == START or ioState == SUB1)
						s = --i;
					else if (ioState != QUOTE1 and ioState != QUOTE3)
					{
						switch (c)
						{
							case '(':	mc = ')'; break;
							case '{':	mc = '}'; break;
							case '[':	mc = ']'; break;
							case '<':	mc = '>'; break;
							case ' ':	ioState = START; s = --i; break;
							default:	mc = c; break;
						}
						
						if (ioState != START)
							s = i - 1;
					}
				}
				else if (kws)
					kws = Move(c, kws);
				break;
			
			case SUB1:
				if (IsAlpha(c))
					ioState = SUB2;
				else if (not IsSpace(c))
				{
					ioState = START;
					s = --i;
				}
				break;
			
			case SUB2:
				if (not isalnum(c) and c != '\'' and c != '_')
				{
					SetStyle(s, kLTextColor);
					ioState = START;
					s = --i;
				}
				break;
			
			case QUOTE1:
			case QUOTE3:
				if (c == 0 or c == '\n')
					leave = true;
				else if (not IsSpace(c))
				{
					if (c == '#' and s < i)
					{
						SetStyle(s, kLCommentColor);
						leave = true;
					}
					else
					{
						switch (c)
						{
							case '(':	mc = ')'; break;
							case '{':	mc = '}'; break;
							case '[':	mc = ']'; break;
							case '<':	mc = '>'; break;
							case '\'':	mc = '\''; interpolate = false; break;
							default:	mc = c; break;
						}

						nest = 0;
						s = i - 1;
						
						ioState += 1;
					}
				}
				break;
			
			case QUOTE2:
			case QUOTE4:
				if (c == 0 or c == '\n')
				{
					if (regex)
						SetStyle(s, kLCharConstColor);
					else
						SetStyle(s, kLStringColor);

					leave = true;
				}
				else if (c == mc)
				{
					if (nest-- == 0)
					{
						if (regex)
							SetStyle(s, kLCharConstColor);
						else
							SetStyle(s, kLStringColor);

						s = i;
						
						if (ioState == QUOTE2)
							ioState = START;
						else if (mc == '}' or mc == ')' or mc == ']' or mc == '>')
							ioState = QUOTE5;
						else
						{
							ioState = QUOTE2;
							nest = 0;
						}
					}
				}
				else if (c == '{' and mc == '}' and nest < 3)
					++nest;
				else if (c == '<' and mc == '>' and nest < 3)
					++nest;
				else if (c == '[' and mc == ']' and nest < 3)
					++nest;
				else if (c == '(' and mc == ')' and nest < 3)
					++nest;
				break;
			
			case QUOTE5:
				if (c == '{' and mc == '}')
				{
					ioState = QUOTE2;
					nest = 0;
				}
				else if (c == '<' and mc == '>')
				{
					ioState = QUOTE2;
					nest = 0;
				}
				else if (c == '[' and mc == ']')
				{
					ioState = QUOTE2;
					nest = 0;
				}
				else if (c == '(' and mc == ')')
				{
					ioState = QUOTE2;
					nest = 0;
				}
				else if (c == '\n')
					leave = true;
				else if (not IsSpace(c))
				{
					ioState = START;
					SetStyle(s, kLTextColor);
					s = --i;
				}
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
					SetStyle(s, kLStringColor);
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
					SetStyle(s, kLStringColor);
					s = inLength;
					leave = true;
				}
				else
					esc = not esc and (c == '\\');
				break;
			
			case REGEX0:
				if (IsAlpha(c) or c == '_')
				{
					kws = Move(c, 1);
					ioState = IDENT;

					SetStyle(s, kLTextColor);
					s = i - 1;
				}
				else if (not IsSpace(c))
				{
					ioState = REGEX1;

					switch (c)
					{
						case '(':	mc = ')'; break;
						case '{':	mc = '}'; break;
						case '[':	mc = ']'; break;
						case '<':	mc = '>'; break;
						default:	mc = c; break;
					}
					
					SetStyle(s, kLStringColor);
					s = i - 1;
				}
				break;
			
			case REGEX1:
				if (c == 0)	// don't like this
				{
//					SetStyle(s, kLTextColor);
//					i = ++s;
//					ioState = START;
					SetStyle(s, kLCharConstColor);
					leave = true;
				}
				else if (c == mc and not esc)
				{
					SetStyle(s, kLCharConstColor);
					s = i;
					ioState = START;
				}
				else
					esc = not esc and (c == '\\');
				break;
			
			case REGEX2:
				if (c == 0)	// don't like this
				{
					SetStyle(s, kLTextColor);
					i = ++s;
					ioState = START;
				}
				else if (c == mc and not esc)
				{
					if (mc == ')' or mc == '}' or mc == ']' or mc == '>')
					{
						switch (text[i])
						{
							case '(':	mc = ')'; break;
							case '{':	mc = '}'; break;
							case '[':	mc = ']'; break;
							case '<':	mc = '>'; break;
							case ' ':	ioState = START; s = --i; break;
							default:	mc = text[i]; break;
						}
						i++;
					}

					if (ioState != START)
						ioState = REGEX1;
				}
				else
					esc = not esc and (c == '\\');
				break;
			
			case SCOPE:
				if (c == '\'' or (not isalnum(c) and c != '_'))
				{
					SetStyle(s, kLTextColor);
					ioState = START;
				}
				break;
			
			case VAR1:
				switch (c)
				{
					case '_':
					case '.':
					case '/':
					case ',':
					case '\\':
					case '#':
					case '%':
					case '=':
					case '-':
					case '~':
					case '|':
					case '?':
					case '&':
					case '`':
					case '\'':
					case '+':
					case '*':
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					case '[':
					case ']':
					case ';':
					case '!':
					case '@':
					case '<':
					case '>':
					case '(':
					case ')':
					case ':':
					case '^':
					case '$':
						ioState = VAR2;
						break;
					
					case '{':
						mc = 1;
						ioState = VAR3;
						break;
					
					case '"':
						if (text[i] == '"')
						{
							++i;
							ioState = VAR2;
						}
						break;
					
					default:
						if (isalnum(c))
							ioState = VAR2;
						else
						{
							SetStyle(s, kLTextColor);
							s = --i;
							ioState = START;
						}
						break;
				}
				break;
			
			case VAR1a:
				if (isalnum(c))
					ioState = VAR2;
				else if (c == '{')
				{
					mc = 1;
					ioState = VAR3;
				}
				else
				{
					SetStyle(s, kLTextColor);
					s = --i;
					ioState = START;
				}
				break;
			
			case VAR2:
				if (not isalnum(c) and c != '_')
				{
					SetStyle(s, kLPreProcessorColor);
					s = --i;
					ioState = START;
				}
				break;
			
			case VAR3:
				if (c == 0 or c == '\n')
				{
					SetStyle(s, kLTextColor);
					ioState = START;
				}
				else if (c == '{')
				{
					++mc;
				}
				else if (c == '}')
				{
					if (--mc <= 0)
					{
						SetStyle(s, kLPreProcessorColor);
						s = i;
						ioState = START;
					}
				}
				break;
			
			case POD1:
				if (Equal(text + s, end, "pod") or
					Equal(text + s, end, "head1") or
					Equal(text + s, end, "head2") or
					Equal(text + s, end, "begin") or
					Equal(text + s, end, "for") or
					Equal(text + s, end, "item") or
					Equal(text + s, end, "over") or
					Equal(text + s, end, "back"))
				{
					ioState = POD2;
				}
				else
					ioState = START;
				break;
			
			case POD2: {
				if (c == 0 or c == '\n')
				{
					SetStyle(s, kLTextColor);
					leave = true;
				}
				else if (c == '=' and Equal(text + i, end, "cut"))
				{
					SetStyle(s, kLTextColor);
					i += 3;
					s = i;
					ioState = START;
				}
				break;
			}
			
			case HERE_DOC_0:
				SetStyle(s, kLTextColor);

				if (Equal(text + i - 1, end, "END") and
					not isalnum(text[i + 2]))
				{
					ioState = HERE_DOC_END;
					leave = true;
				}
				else if (Equal(text + i - 1, end, "EOF") and
					not isalnum(text[i + 2]))
				{
					ioState = HERE_DOC_EOF;
					leave = true;
				}
				else
				{
					ioState = START;
					s = i;
				}
				break;

			case HERE_DOC_END:
				SetStyle(s, kLTextColor);
				if (Equal(text, end, "END") and inLength <= 4)
					ioState = START;
				leave = true;
				break;
			
			case HERE_DOC_EOF:
				SetStyle(s, kLTextColor);
				if (Equal(text, end, "EOF") and inLength <= 4)
					ioState = START;
				leave = true;
				break;
			
			default:	// error condition, gracefully leave the loop
				leave = true;
				break;
		}
	}
	
	ioState = ioState & 0x001FUL;
	ioState |= (nest & 0x03UL) << 5;
	ioState |= (mc & 0x7FUL) << 9;
	if (regex) ioState |= 0x0080;
	if (interpolate) ioState |= 0x0100;
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
					while (*inText and ! (*inText == '*' and inText[1] == '/'))
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
MLanguagePerl::Balance(
	const MTextBuffer&	inText,
	uint32&				ioOffset,
	uint32&				ioLength)
{
	MTextBuffer::const_iterator txt = inText.begin();
	
	stack<int32> bls, sbls, pls;
	
	while (txt.GetOffset() < ioOffset + ioLength)
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
	
	char ec = 0, oc;
	stack<int32> *s;
	
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
MLanguagePerl::IsBalanceChar(
	wchar_t				inChar)
{
	return inChar == ')' or inChar == ']' or inChar == '}';
}

bool
MLanguagePerl::IsSmartIndentLocation(
	const MTextBuffer&	inText,
	uint32				inOffset)
{
	return inOffset > 0 and inText.GetChar(inOffset - 1) == '{';
}

bool
MLanguagePerl::IsSmartIndentCloseChar(
	wchar_t				inChar)
{
	return inChar == '}';
}

void
MLanguagePerl::CommentLine(
	string&				ioLine)
{
	ioLine.insert(0, kCommentPrefix);
}

void
MLanguagePerl::UncommentLine(
	string&				ioLine)
{
	if (ioLine.length() >= 1 and ioLine[0] == '#')
		ioLine.erase(0, 1);
}

uint32
MLanguagePerl::MatchLanguage(
	const std::string&	inFile,
	MTextBuffer&		inText)
{
	uint32 result = 0;
	if (FileNameMatches("*.pl;*.pm;*.cgi", inFile))
	{
		result += 90;
	}
	else
	{
		MSelection s;
		
		if (inText.Find(0, "^#!\\s*.*?perl.*\\n",
				kDirectionForward, false, true, s) and
			s.GetAnchor() == 0)
		{
			result += 75;
		}			
	}
	return result;
}

bool MLanguagePerl::Softwrap() const
{
	return false;
}