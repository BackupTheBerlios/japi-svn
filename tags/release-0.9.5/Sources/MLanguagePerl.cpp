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

#include "MJapi.h"

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
	START, IDENT, LCOMMENT, STRING,
	REGEX1, REGEX2, SCOPE,
	VAR1, VAR2, VAR3,
	QUOTE1, QUOTE2, QUOTE3, QUOTE4, QUOTE5,
	SUB1, SUB2, POD1, POD2,
	HERE_DOC_0, HERE_DOC_1,
	HERE_DOC_END_0, HERE_DOC_END_1,
	STATE_COUNT
};

enum MStateSelector {
	mSTART,
	mIDENT,
	mLCOMMENT,
	mSTRING,
	mREGEX,
	mSCOPE,
	mVAR,
	mQUOTE,
	mSUB,
	mPOD,
	mHERE_DOC
};

//union MState
//{
//	uint16				value;
//	struct {
//		MStateSelector	selector	: 4;
//		int 			matchchar	: 7;
//		int 			count		: 5;
//	};
//};
//
void ParseMState(
	uint16&		ioState,
	bool&		outRegex,
	uint32&		outCount,
	char&		outMatchChar)
{
	uint32 selector = (ioState >> 12) & 0x000F;
	uint32 matchchar = (ioState >> 5) & 0x007F;
	uint32 count = ioState & 0x001F;
	
	outRegex = false;
	outCount = 0;
	outMatchChar = 0;
	
	switch (selector)
	{
		case mSTART:
			ioState = START;
			break;
			
		case mIDENT:
			ioState = IDENT;
			break;
			
		case mLCOMMENT:
			ioState = LCOMMENT;
			break;
			
		case mSTRING:
			ioState = STRING;
			outMatchChar = matchchar;
			break;
			
		case mREGEX:
			ioState = REGEX1 + count;
			outMatchChar = matchchar;
			break;
			
		case mSCOPE:
			ioState = SCOPE;
			break;
			
		case mVAR:
			ioState = VAR1 + count;
			break;
			
		case mQUOTE:
			ioState = QUOTE1 + (count & 0x03);
			outRegex = (count & 0x04) != 0;
			outMatchChar = matchchar;
			break;
			
		case mSUB:
			ioState = SUB1 + count;
			break;
			
		case mPOD:
			ioState = POD1 + count;
			break;
			
		case mHERE_DOC:
			ioState = HERE_DOC_END_0;
			outMatchChar = matchchar;
			outCount = count;
			break;
	}
}

void StoreMState(
	uint16&		ioState,
	bool		inRegex,
	uint32		inCount,
	char		inMatchChar)
{
	uint32 selector = 0, matchchar = 0, count = 0;
	
	switch (ioState)
	{
		case START:
			selector = mSTART;
			break;
		
		case IDENT:
			selector = mIDENT;
			break;
			
		case LCOMMENT:
			selector = mLCOMMENT;
			break;
			
		case STRING:
			selector = mSTRING;
			matchchar = inMatchChar;
			break;
			
		case REGEX1:
		case REGEX2:
			selector = mREGEX;
			count = ioState - REGEX1;
			matchchar = inMatchChar & 0x007F;
			break;
			
		case SCOPE:
			selector = mSCOPE;
			break;
			
		case VAR1:
		case VAR2:
		case VAR3:
			selector = mVAR;
			count = ioState - VAR1;
			break;
			
		case QUOTE1:
		case QUOTE2:
		case QUOTE3:
		case QUOTE4:
		case QUOTE5:
			selector = mQUOTE;
			count = ioState - QUOTE1;
			if (inRegex)
				count |= 0x04;
			matchchar = inMatchChar & 0x007F;
			break;
			
		case SUB1:
		case SUB2:
			selector = mSUB;
			count = ioState - SUB1;
			break;
			
		case POD1:
		case POD2:
			selector = mPOD;
			count = ioState - POD1;
			break;
			
		case HERE_DOC_END_0:
		case HERE_DOC_END_1:
			selector = mHERE_DOC;
			matchchar = inMatchChar & 0x007F;
			count = inCount;
			break;
	}
	
	ioState =
		((selector  & 0x000F) << 12) |
		((matchchar & 0x007F) <<  5) |
		((count     & 0x001F) <<  0);
}

void MiniCRC(
	char&	ioCRC,
	char	inChar)
{
	ioCRC <<= 1;
	if (ioCRC & 0x0080)
		ioCRC = (ioCRC & 0x007F) & 0x0021;
	ioCRC ^= inChar;
}

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
		"qq", "qr", "qx", "rand", "read", "readdir", "readlink", "recv",
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
	bool leave = false, start_regex = true;
	
	if (ioState == LCOMMENT)
		SetStyle(0, kLCommentColor);
	else
		SetStyle(0, kLTextColor);
	
	if (inLength == 0)
		return;

	// save the match char for multiline regex
	
	char mc, crc = 0;
	bool regex;
	uint32 count;
	
	ParseMState(ioState, regex, count, mc);
	
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
					start_regex = true;
				}
				else if (c == '=' and isalpha(text[i]))
					ioState = POD1;
				else if (c == '=' and text[i] == '~')
					start_regex = true;
				else if (c == '!' and text[i] == '~')
					start_regex = true;
				else if (c == '"')
				{
					ioState = STRING;
					mc = '"';
				}
				else if (c == '\'')
				{
					ioState = STRING;
					mc = '\'';
				}
				else if (c == '`')
				{
					ioState = STRING;
					mc = '`';
				}
				else if (c == '&')
					ioState = SCOPE;
				else if (c == '$')
					ioState = VAR1;
				else if (c == '@' or c == '*' or c == '%')
					ioState = VAR1;
				else if (c == '<' and text[i] == '<')
				{
					ioState = HERE_DOC_0;
					++i;
				}
				else if (c == '/' and start_regex)
				{
					mc = '/';
					ioState = REGEX1;
				}
				else if (c == '\n' or c == 0)
					leave = true;
				else if (not isspace(c))
					start_regex = not (isalnum(c) or c == ')');
					
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
								s = --i;
								break;

							case 's':
								ioState = QUOTE3;
								regex = true;
								s = --i;
								break;

							case 'q':
								ioState = QUOTE1;
								regex = false;
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
							s = --i;
						}
						else if (Equal(text + s, end, "qw"))
						{
							ioState = QUOTE1;
							regex = false;
							s = --i;
						}
						else if (Equal(text + s, end, "qr"))
						{
							ioState = QUOTE1;
							regex = true;
							s = --i;
						}
						else if (Equal(text + s, end, "tr"))
						{
							ioState = QUOTE3;
							regex = true;
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
					switch (c)
					{
						case '(':	mc = ')'; break;
						case '{':	mc = '}'; break;
						case '[':	mc = ']'; break;
						case '<':	mc = '>'; break;
						case '\'':	mc = '\''; break;
						default:	mc = c; break;
					}

					count = 0;
					s = i - 1;
					
					ioState += 1;
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
				else if (esc)
					esc = false;
				else if (c == mc)
				{
					if (count-- == 0)
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
							count = 0;
						}
					}
				}
				else if (c == '{' and mc == '}' and count < 3)
					++count;
				else if (c == '<' and mc == '>' and count < 3)
					++count;
				else if (c == '[' and mc == ']' and count < 3)
					++count;
				else if (c == '(' and mc == ')' and count < 3)
					++count;
				else 
					esc = (c == '\\');
				break;
			
			case QUOTE5:
				if (esc)
					esc = false;
				else if (c == '{' and mc == '}')
				{
					ioState = QUOTE2;
					count = 0;
				}
				else if (c == '<' and mc == '>')
				{
					ioState = QUOTE2;
					count = 0;
				}
				else if (c == '[' and mc == ']')
				{
					ioState = QUOTE2;
					count = 0;
				}
				else if (c == '(' and mc == ')')
				{
					ioState = QUOTE2;
					count = 0;
				}
				else if (c == '\n')
					leave = true;
				else if (c == '\\')
					esc = true;
				else if (not IsSpace(c))
				{
					ioState = START;
					SetStyle(s, kLTextColor);
					s = --i;
				}
				break;
			
			case STRING:
				if (c == mc and not esc)
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
				else if (esc)
					esc = false;
				else
					esc = (c == '\\');
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
				else if (esc)
					esc = false;
				else
					esc = (c == '\\');
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
				else if (esc)
					esc = false;
				else
					esc = (c == '\\');
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
			
			case VAR2:
				if (not isalnum(c) and c != '_')
				{
					SetStyle(s, kLPreProcessorColor);
					s = --i;
					ioState = START;
					start_regex = false;
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
						start_regex = false;
					}
				}
				break;
			
			case POD1:
				if (Equal(text + s, end, "=pod") or
					Equal(text + s, end, "=head1") or
					Equal(text + s, end, "=head2") or
					Equal(text + s, end, "=begin") or
					Equal(text + s, end, "=for") or
					Equal(text + s, end, "=item") or
					Equal(text + s, end, "=over") or
					Equal(text + s, end, "=back"))
				{
					ioState = POD2;
				}
				else
				{
					s = --i;
					ioState = START;
				}
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
				if (c == 0 or c == '\n')
				{
					ioState = START;
					leave = true;
					SetStyle(s, kLTextColor);
				}
				else if (isalnum(c) or c == '_')
				{
					SetStyle(s, kLTextColor);
					mc = c;
					count = 1;
					ioState = HERE_DOC_1;
				}
				break;

			case HERE_DOC_1:
				if (isalnum(c) or c == '_')
				{
					MiniCRC(mc, c);
					++count;
				}
				else
				{
					ioState = HERE_DOC_END_0;
					leave = true;
				}
				break;

			case HERE_DOC_END_0:
				SetStyle(s, kLTextColor);
				if (isalnum(c) or c == '_')
				{
					crc = c;
					ioState = HERE_DOC_END_1;
				}
				else
					leave = true;
				break;
			
			case HERE_DOC_END_1:
				if (isalnum(c) or c == '_')
					MiniCRC(crc, c);
				else if ((c == 0 or c == '\n') and (crc == mc and i - s - 1 == count))
					ioState = START;
				else
				{
					ioState = HERE_DOC_END_0;
					leave = true;
				}
				break;
			
			default:	// error condition, gracefully leave the loop
				leave = true;
				break;
		}
	}
	
	StoreMState(ioState, regex, count, mc);
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
				
			case '#':
				inText += 1;
				while (*inText and *inText != '\n')
					inText++;
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
	const string&		inFile,
	MTextBuffer&		inText)
{
	uint32 result = 0;
	if (FileNameMatches("*.pl;*.pm;*.cgi", inFile))
	{
		result += 90;
	}
	else
	{
		MSelection s(nil);
		
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

namespace {

typedef MTextBuffer::const_iterator	MTextPtr;

const uint32 kMaxNameSize = 256;

MTextPtr skip(MTextPtr text, int ch);
MTextPtr skip_nc(MTextPtr text, int ch);
MTextPtr skip_for_balance(MTextPtr txt);
MTextPtr skippod(MTextPtr text);
MTextPtr comment(MTextPtr text);
MTextPtr parens(MTextPtr text, char open);
MTextPtr regex(MTextPtr text, char mc);
MTextPtr regex2(MTextPtr text, char mc);
MTextPtr name_append(MTextPtr text, string& name);
MTextPtr package(MTextPtr text, MNamedRange& ns);

MTextPtr skip(MTextPtr text, int ch)
{
	while (*text)
	{
		if (*text == ch)
		{
			text++;
			break;
		}
		if (*text == '\\' and *(text + 1) != 0)
			++text;
		text++;
	}

	return text;
}

MTextPtr skip_nc(MTextPtr text, int ch)
{
	while (*text)
	{
		if (*text == ch)
		{
			text++;
			break;
		}
		else if (*text == '"')
			text = skip(text + 1, '"');
		else
			text++;
	}

	return text;
}

MTextPtr skippod(MTextPtr text)
{
	while (*text)
	{
		if (*text++ == '=')
		{
			if (text == "cut")
			{
				text += 3;
				break;
			}
		}
	}
	return text;
}

MTextPtr name_append(MTextPtr text, string& name)
{
	MTextPtr s(text);
	
	while (isalnum(*text) or *text == '_')
		name += *text++;
	
	return text;
}

MTextPtr regex(MTextPtr text, char mc)
{
	MTextPtr backRef(text);

	while (*text)
	{
		if (*text == '\n' or *text == 0)
			return backRef;
		else if (*text == mc)
		{
			++text;
			break;
		}
		else if (*text == '\\')
			text += 2;
		else
			++text;
	}
	return text;
}

MTextPtr regex2(MTextPtr text, char mc)
{
	MTextPtr backRef(text);
	
	switch (mc)
	{
		case '[':	mc = ']'; break;
		case '(':	mc = ')'; break;
		case '<':	mc = '>'; break;
		case '{':	mc = '}'; break;
		case ' ':	return text;
	}
	
	while (*++text)
	{
		if (*text == '\n' or *text == 0)
			return backRef;
		
		if (*text == mc)
		{
			if (mc == ')' or mc == '}' or mc == ']' or mc == '>')
			{
				switch (*++text)
				{
					case '(':	mc = ')'; break;
					case '{':	mc = '}'; break;
					case '[':	mc = ']'; break;
					case '<':	mc = '>'; break;
					case ' ':	return text;
					default:	mc = *text; break;
				}
			}
			return regex(text + 1, mc);
		}
		else if (*text == '\\')
			++text;
	}
	
	return text;
}

MTextPtr comment(MTextPtr text)
{
	do
	{
		while (isspace(*text))
			++text;
		
		if (*text == '=' and isalpha(*(text + 1)))
		{
			string ident;
			MTextPtr s(text);
			
			text = name_append(text + 1, ident);
			
			if (ident == "head0" or
				ident == "head1" or
				ident == "item" or
				ident == "over" or
				ident == "back" or
				ident == "pod" or
				ident == "for" or
				ident == "begin" or
				ident == "end")
			{
				text = skippod(text);
			}
			else
				text = s + 1;
		}
		else if (*text == '/')
		{
			text = regex(text + 1, '/');
			
			string opts; // regular expression options, if any
			
			if (isalpha(*text))
				text = name_append(text, opts);
		}
		else if (*text == '#' and *(text - 1) != '$')
			text = skip(text + 1, '\n');
		else if (isalpha(*text))
		{
			string name;
			MTextPtr s(text);
			
			text = name_append(text, name);
			
			if (name == "q" or
				name == "qq" or
				name == "qx" or
				name == "qw" or
				name == "qr" or
				name == "m")
			{
				text = regex(text, *text);
			}
			else if (name == "s" or
					 name == "y" or
					 name == "tr")
			{
				text = regex2(text, *text);
			}
			else
			{
				text = s;
				break;
			}
		}
		else
			break;
	}
	while (*text);

	return text;
}

MTextPtr parens(MTextPtr text, char open)
{
	int c;
	char close = 0;
	
	switch (open)
	{
		case '(':	close = ')'; break;
		case '{':	close = '}'; break;
		case '[':	close = ']'; break;
//		default:	ASSERT(false); return text;
	}

	while (true)
	{
		text = comment(text);

		c = *text++;
		
		if (c == '\'') 
		{
			text = skip(text, '\'');
			continue;
		}
		
		if (c == '"') 
		{
			text = skip(text, '"');
			continue;
		}
		
		if (c == open)
		{
			text = parens(text, open);
			continue;
		}
		
		if (c == '/')
		{
			text = regex(text, c);
			continue;
		}
		
		if (c == close)
			return text;
		
		if (isalpha(c))
		{
			while (isalpha(*text))
				++text;
		}
		
		if (c == '\0')
			return text - 1;
	}
}

MTextPtr package(MTextPtr text, MNamedRange& ns)
{
	while (*text)
	{
		text = comment(text);
		
		switch (*text)
		{
			case 0:
				break;
			case '(':
			case '{':
			case '[':
				text = parens(text + 1, *text);
				break;
			case '"':
			case '\'':
				text = skip(text + 1, *text);
				break;
			default:
				if (text == "sub")
				{
					MNamedRange n;
					
					n.begin = text.GetOffset();
					
					text = comment(text + 3);

					n.selectFrom = text.GetOffset();
					text = name_append(text, n.name);
					
					n.selectTo = text.GetOffset();
			
					text = comment(text);
					if (*text == '(')
					{
						text = parens(text + 1, '(');
						text = comment(text);
					}
					
					if (*text == '{')
						text = parens(text + 1, '{');
					
					n.end = text.GetOffset();
					
					ns.subrange.push_back(n);
				}
				else if (isalnum(*text))
				{
					if (text == "package")
						return text;
					
					while (isalnum(*text))
						++text;
				}
				else
					++text;
				break;
		}
	}
	
	return text;
}

}

void MLanguagePerl::Parse(
	const MTextBuffer&	inText,
	MNamedRange&		outRange,
	MIncludeFileList&	outIncludeFiles)
{
	outRange.name.clear();
	outRange.subrange.clear();
	outIncludeFiles.clear();

	MTextPtr text(inText.begin());

	outRange.begin = text.GetOffset();

	text = package(text, outRange);
	
	while (*text and text == "package")
	{
		MNamedRange p;
		p.begin = text.GetOffset();
		
		text = comment(text + 7);
		
		p.selectFrom = text.GetOffset();

		text = name_append(text, p.name);

		while (text == "::")
		{
			p.name += "::";
			text = name_append(text + 2, p.name);
		}

		p.selectTo = text.GetOffset();
		
		text = package(text, p);
		p.end = text.GetOffset();
		
		outRange.subrange.push_back(p);
	}

	outRange.end = text.GetOffset();
}



