//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MLanguagePascal.cpp 95 2006-12-07 14:23:52Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 15:26:34
*/

#include "MJapi.h"

#include "MLanguagePascal.h"
#include "MTextBuffer.h"
#include "MSelection.h"
#include "MUnicode.h"
#include "MFile.h"

#include <stack>
#include <cassert>

using namespace std;

enum State
{
	START, IDENT, OTHER, COMMENT1, COMMENT2,
	STRING1, STRING2, SET, LEAVE
};

MLanguagePascal::MLanguagePascal()
{
}

void MLanguagePascal::Init()
{
	const char* keywords[] = {
		"AND", "ARRAY", "BEGIN", "CASE", "CONST", "DIV", "DO", "DOWNTO",
		"ELSE", "END", "EXTERNAL", "FILE", "FOR", "FORWARD", "FUNCTION",
		"GOTO", "IF", "IMPLEMENTATION", "IN", "INHERITED", "INLINE", "INTERFACE", "LABEL", "MOD", "NIL", "NOT",
		"OBJECT", "OF", "OR", "OTHERWISE", "OVERRIDE", "PACKED", "PROCEDURE", "PROGRAM", "RECORD", "REPEAT", "SET",
		"STRING", "THEN", "TO", "TYPE", "UNIT", "UNIV", "UNTIL", "USES", "VAR", "WHILE", "WITH", "",
		"INTEGER", "LONGINT", "CHAR", "BOOLEAN", "TRUE", "FALSE",
		"LEAVE", "CYCLE", nil
	};
	
	AddKeywords(keywords);
	GenerateDFA();
}

void MLanguagePascal::StyleLine(
	const MTextBuffer&	inText,
	uint32				inOffset,
	uint32				inLength,
	uint16&				ioState)
{
	MTextBuffer::const_iterator text = inText.begin() + inOffset;
	MTextBuffer::const_iterator end = inText.end();
	uint32 i = 0, s = 0, kws = 0, esc = 0;
	bool leave = false;
	
	if (ioState == COMMENT1 or ioState == COMMENT2)
		SetStyle(0, kLCommentColor);
	else
		SetStyle(0, kLTextColor);
	
	if (inLength <= 0)
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
				if (isalpha(c) or c == '_')
				{
					kws = Move(toupper(c), 1);
					ioState = IDENT;
				}
				else if (c == '(' and text[i] == '*')
				{
					ioState = COMMENT1;
					++i;
				}
				else if (c == '{')
					ioState = COMMENT2;
				else if (c == '"')
					ioState = STRING1;
				else if (c == '\'')
					ioState = STRING2;
				else if (c == '{')
					ioState = SET;
				else if (c == '\n' or c == 0)
					leave = true;
					
				if (leave or (ioState != START and s < i))
				{
					SetStyle(s, kLTextColor);
					s = i - 1;
				}
				break;
			
			case COMMENT1:
				if ((s == 0 or i > s + 1) and c == '*' and text[i] == ')')
				{
					SetStyle(s - 1, kLCommentColor);
					s = ++i;
					ioState = START;
				}
				else if (c == 0 or c == '\n')
				{
					SetStyle(s - 1, kLCommentColor);
					leave = true;
				}
				break;

			case COMMENT2:
				if ((s == 0 or i > s + 1) and c == '}')
				{
					SetStyle(s, kLCommentColor);
					s = i;
					ioState = START;
				}
				else if (c == 0 or c == '\n')
				{
					SetStyle(s, kLCommentColor);
					leave = true;
				}
				break;

			case IDENT:
				if (not isalnum(c) and c != '_')
				{
					int kwc;

					if (i > s + 1 and (kwc = IsKeyWord(kws)) != 0)
						SetStyle(s, kLKeyWordColor);
					else
						SetStyle(s, kLTextColor);
					
					s = --i;
					ioState = START;
				}
				else if (kws)
					kws = Move(toupper(c), kws);
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
			
			default:	// error condition, gracefully leave the loop
				leave = true;
				break;
		}
	}
}

class PasBalance
{
  public:
					PasBalance(const MTextBuffer& inText);

	bool			Balance(uint32& ioBegin, uint32& ioLength);

  private:

	enum Token {
		token_EOF, token_Undefined, token_LParen, token_RParen,
		token_LBracket, token_RBracket,
		token_Begin, token_End, token_Repeat, token_Until,
		token_Case, token_Of, token_Record
	} 				lookahead;

	MTextBuffer::const_iterator
					text;
	long			tokenStart;
	uint32			start;
	uint32			end;
	bool			result;
	bool			done;
	
	void			GetNextToken();
	void			BeginEndBlock();
	void			RecordBlock();
	void			CaseBlock();
	void			RepeatBlock();
	void			Parens();
	void			Bracket();
	
	void			Return(long inOffset);
};

PasBalance::PasBalance(const MTextBuffer& inText)
	: text(inText.begin())
	, tokenStart(0)
	, start(0)
	, end(0)
	, result(false)
	, done(false)
{
}

bool PasBalance::Balance(uint32& ioBegin, uint32& ioLength)
{
	start = ioBegin;
	end = ioBegin + ioLength;
	
	GetNextToken();
	while (not done)
	{
		switch (lookahead)
		{
			case token_Begin:
				BeginEndBlock();
				break;
			case token_Record:
				RecordBlock();
				break;
			case token_LParen:
				Parens();
				break;
			case token_LBracket:
				Bracket();
				break;
			default:
				GetNextToken();
				break;
		}
	}
	
	ioBegin = start;
	ioLength = end - start;
	
	return result;
}

void PasBalance::Return(long inStart)
{
	if (not done)
	{
		if (inStart < tokenStart and inStart <= start and tokenStart >= end)
		{
			start = inStart;
			end = tokenStart;
			result = true;
			done = true;
		}
		else
			GetNextToken();
	}
}

void PasBalance::GetNextToken()
{
	uint32 ch;
	string s;
	lookahead = token_Undefined;
	while (not done and lookahead == token_Undefined)
	{
		tokenStart = text.GetOffset();
		ch = *text++;
		s = toupper(ch);

		switch (ch)
		{
			case '{':
				do	ch = *text++;
				while (ch and ch != '}');
				break;
			case '(':
				if (*text != '*')
					lookahead = token_LParen;
				else
				{
					do	ch = *text++;
					while (ch and not (*text == '*' and *(text + 1) == ')'));
					++text;
				}
				break;
			case ')':
				lookahead = token_RParen;
				break;
			case '[':
				lookahead = token_LBracket;
				break;
			case ']':
				lookahead = token_RBracket;
				break;
			case '\'':
				do	ch = *text++;
				while (ch and ch != '\'');
				break;
			case 0:
				done = true;
				break;
			default:
				if (isalpha(ch) or ch == '_')
				{
					do
					{
						ch = *text;
						if (isalpha(ch) or ch == '_')
						{
							s += toupper(ch);
							++text;
						}
					}
					while (isalpha(ch) or ch == '_');

					if (s == "BEGIN")
						lookahead = token_Begin;
					else if (s == "END")
						lookahead = token_End;
					else if (s == "REPEAT")
						lookahead = token_Repeat;
					else if (s == "UNTIL")
						lookahead = token_Until;
					else if (s == "CASE")
						lookahead = token_Case;
					else if (s == "RECORD")
						lookahead = token_Record;
					else if (s == "OF")
						lookahead = token_Of;
				}
				break;
		}
	}
}

void PasBalance::BeginEndBlock()
{
	long blockStart = text.GetOffset();
	GetNextToken();
	do
	{
		switch (lookahead)
		{
			case token_Begin:
				BeginEndBlock();
				break;
			case token_LParen:
				Parens();
				break;
			case token_LBracket:
				Bracket();
				break;
			case token_Repeat:
				RepeatBlock();
				break;
			case token_Case:
				CaseBlock();
				break;
			case token_End:
				break;
			default:
				GetNextToken();
				break;
		}
	}
	while (not done and lookahead != token_End);

	if (lookahead == token_End)
		Return(blockStart);
}

void PasBalance::RecordBlock()
{
	long blockStart = text.GetOffset();
	GetNextToken();
	do
	{
		switch (lookahead)
		{
			case token_LParen:
				Parens();
				break;
			case token_LBracket:
				Bracket();
				break;
			case token_End:
				break;
			default:
				GetNextToken();
				break;
		}
	}
	while (not done and lookahead != token_End);

	if (lookahead == token_End)
		Return(blockStart);
}

void PasBalance::CaseBlock()
{
	long blockStart = text.GetOffset();
	bool gotOf = false;
	
	GetNextToken();
	do
	{
		switch (lookahead)
		{
			case token_Of:
				if (not gotOf)
				{
					blockStart = text.GetOffset();
					GetNextToken();
				}
				break;
			case token_Begin:
				BeginEndBlock();
				break;
			case token_LParen:
				Parens();
				break;
			case token_LBracket:
				Bracket();
				break;
			case token_Repeat:
				RepeatBlock();
				break;
			case token_Case:
				CaseBlock();
				break;
			case token_End:
				break;
			default:
				GetNextToken();
				break;
		}
	}
	while (not done and lookahead != token_End);

	if (lookahead == token_End)
		Return(blockStart);
}

void PasBalance::RepeatBlock()
{
	long blockStart = text.GetOffset();
	
	GetNextToken();
	do
	{
		switch (lookahead)
		{
			case token_Begin:
				BeginEndBlock();
				break;
			case token_LParen:
				Parens();
				break;
			case token_LBracket:
				Bracket();
				break;
			case token_Repeat:
				RepeatBlock();
				break;
			case token_Case:
				CaseBlock();
				break;
			case token_Until:
				break;
			default:
				GetNextToken();
				break;
		}
	}
	while (not done and lookahead != token_Until);

	if (lookahead == token_Until)
		Return(blockStart);
}

void PasBalance::Parens()
{
	long blockStart = text.GetOffset();
	GetNextToken();
	do
	{
		switch (lookahead)
		{
			case token_LParen:
				Parens();
				break;
			case token_LBracket:
				Bracket();
				break;
			case token_RParen:
				break;
			default:
				GetNextToken();
				break;
		}
	}
	while (not done and lookahead != token_RParen);

	if (lookahead == token_RParen)
		Return(blockStart);
}

void PasBalance::Bracket()
{
	long blockStart = text.GetOffset();
	GetNextToken();
	do
	{
		switch (lookahead)
		{
			case token_LParen:
				Parens();
				break;
			case token_LBracket:
				Bracket();
				break;
			case token_RBracket:
				break;
			default:
				GetNextToken();
				break;
		}
	}
	while (not done and lookahead != token_RBracket);

	if (lookahead == token_RBracket)
		Return(blockStart);
}

bool
MLanguagePascal::Balance(
	const MTextBuffer&	inText,
	uint32&				ioOffset,
	uint32&				ioLength)
{
	PasBalance b(inText);
	return b.Balance(ioOffset, ioLength);
}

bool
MLanguagePascal::IsBalanceChar(
	wchar_t				inChar)
{
	return inChar == ')' or inChar == ']' or inChar == '}';
}

//bool
//MLanguagePascal::IsSmartIndentLocation(
//	const MTextBuffer&	inText,
//	uint32				inOffset)
//{
//	return inOffset > 0 and inText.GetChar(inOffset - 1) == '{';
//}
//
//bool
//MLanguagePascal::IsSmartIndentCloseChar(
//	wchar_t				inChar,
//	const MTextBuffer&	inText,
//	uint32&				ioOpenOffset)
//{
//	return inChar == '}';
//}

void
MLanguagePascal::CommentLine(
	string&				ioLine)
{
//	ioLine.insert(0, kCommentPrefix);
}

void
MLanguagePascal::UncommentLine(
	string&				ioLine)
{
	//if (ioLine.length() >= 1 and ioLine[0] == '#')
	//	ioLine.erase(0, 1);
}

uint32
MLanguagePascal::MatchLanguage(
	const string&		inFile,
	MTextBuffer&		inText)
{
	uint32 result = 0;
	if (FileNameMatches("*.p;*.pas;*.mod", inFile))
	{
		result += 90;
	}
	return result;
}

bool MLanguagePascal::Softwrap() const
{
	return false;
}

void MLanguagePascal::Parse(
	const MTextBuffer&	inText,
	MNamedRange&		outRange,
	MIncludeFileList&	outIncludeFiles)
{
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
}



