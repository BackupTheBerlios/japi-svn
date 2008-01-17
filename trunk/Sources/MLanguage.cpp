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

/*	$Id: MLanguage.cpp 108 2007-04-21 20:28:14Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 14:05:56
*/

#include "MJapieG.h"

#include "MLanguage.h"
#include "MLanguageCpp.h"
#include "MLanguagePerl.h"
#include "MLanguageTeX.h"
#include "MLanguageHTML.h"
#include "MLanguageXML.h"
#include "MMenu.h"

#include <boost/bind.hpp>

#include <set>
#include <map>

using namespace std;

const uint32
	kMaxStringLength		= 256,
	kMaxChars				= 256,
	kHashTableSize			= (1 << 20),
	kHashTableElementSize	= (1 << 10),
	kMaxStateCount			= (1 << 15),	// <-- wel wat klein misschien?
	kHashMagicNumber1		= 324027,
	kHashMagicNumber2		= 13;

struct MRecognizer
{
						MRecognizer();
						~MRecognizer();

	void				AddWord(
							string			inWord,
							uint8			inTag);
							
	void				CreateAutomaton();
	
	uint32				Move(
							uint8			inChar,
							uint32			inState);

	uint8				IsKeyWord(
							uint32			inState);

	void				CollectKeyWordsBeginningWith(
							string			inPattern,
							vector<string>&	ioStrings);

  private:

	void				CollectKeyWordsStartingFromState(
							uint32			inState,
							string			inWord,
							set<string>&	inKeys,
							vector<string>&	outWords);
	
	union MTransition
	{
		struct
		{
			bool		last	: 1;
			uint32		dest	: 15;
			uint8		term	: 8;
			uint8		attr	: 8;
		}				b;
		uint32			d;
	};
	
	typedef vector<MTransition>	MAutomaton;

	static const MTransition kNullTransition;

	struct MHashTable
	{
						MHashTable();
						~MHashTable();
		
		static uint32	Hash(MTransition* inStates, uint32 inStateCount);
		uint32			Lookup(MTransition* inStates, uint32 inStateCount,
							MAutomaton& ioAutomaton);
		
		struct MBucket
		{
			uint32			mAddr;
			uint32			mSize;
			uint32			mNext;
		};
	
		uint32*			mTable;
		vector<MBucket>	mBuckets;
		int32			mLastPos;
	};
	
	struct MCompareData
	{
		bool	operator()(const pair<string,uint8>& inA, const pair<string,uint8>& inB) const
			{ return inA.first < inB.first; }
	};
	
	MAutomaton			mAutomaton;
	auto_ptr<map<string,uint8> >
						mData;
};

const MRecognizer::MTransition MRecognizer::kNullTransition = {};


MRecognizer::MHashTable::MHashTable()
	: mTable(new uint32[kHashTableSize])
	, mLastPos(-1)
{
	memset(mTable, 0, kHashTableSize * sizeof(uint32));
	mBuckets.push_back(MBucket());	// dummy
}

MRecognizer::MHashTable::~MHashTable()
{
	delete[] mTable;
}

uint32 MRecognizer::MHashTable::Hash(MTransition* inStates, uint32 inStateCount)
{
	uint32 r = 0;

	for (uint32 i = 0; i < inStateCount; ++i)
		r += inStates[i].d;
	
	return ((r * kHashMagicNumber1) >> kHashMagicNumber2) % kHashTableSize;
}

uint32 MRecognizer::MHashTable::Lookup(
	MTransition* inStates, uint32 inStateCount, MAutomaton& ioAutomaton)
{
	if (inStateCount == 0)
		inStates[inStateCount++] = kNullTransition;
	
	inStates[inStateCount - 1].b.last = true;

	uint32 addr = Hash(inStates, inStateCount);
	bool found = false;
	
	for (uint32 ix = mTable[addr]; ix != 0 and not found; ix = mBuckets[ix].mNext)
	{
		if (mBuckets[ix].mSize == inStateCount)
		{
			found = true;
			
			for (uint32 i = 0; i < inStateCount; ++i)
			{
				if (ioAutomaton[mBuckets[ix].mAddr + i].d != inStates[i].d)
				{
					found = false;
					break;
				}
			}
			
			if (found)
				addr = mBuckets[ix].mAddr;
		}
	}
	
	if (not found)
	{
		uint32 size = ioAutomaton.size();

		if (size + inStateCount > kMaxStateCount)
			THROW(("Automaton becoming too large"));
		
		for (uint32 i = 0; i < inStateCount; ++i)
			ioAutomaton.push_back(inStates[i]);
		
		MBucket b;
		b.mAddr = size;
		b.mSize = inStateCount;
		b.mNext = mTable[addr];
		mTable[addr] = mBuckets.size();
		
		addr = size;
		
		mBuckets.push_back(b);
	}
	
	return addr;
}

MRecognizer::MRecognizer()
{
}

MRecognizer::~MRecognizer()
{
}

void MRecognizer::AddWord(string inWord, uint8 inTag)
{
	if (mData.get() == nil)
		mData.reset(new map<string, uint8>());
	
	map<string,uint8>::iterator i = mData->find(inWord);
	if (i == mData->end())
		mData->insert(pair<string,uint8>(inWord, (1 << inTag)));
	else
		(*i).second |= (1 << inTag);
}

void MRecognizer::CreateAutomaton()
{
	unsigned char s0[kMaxStringLength] = "";
	
	MTransition larval_state[kMaxStringLength + 1][kMaxChars] = {};
	uint32 l_state_len[kMaxStringLength + 1];
	uint8 is_terminal[kMaxStringLength];
	
	memset(l_state_len, 0, sizeof(uint32) * (kMaxStringLength + 1));
	
	MHashTable ht;
	
	uint32 i = 0, p, q;
	
	for (map<string,uint8>::iterator w = mData->begin(); w != mData->end(); ++w)
	{
		q = w->first.length();
		
		if (q == 0)
			continue;
		
		uint8 s1[kMaxStringLength];
		memcpy(s1, w->first.c_str(), q);
		s1[q] = 0;
		
		for (p = 0; s1[p] == s0[p]; ++p)
			;
		
		if (s1[p] < s0[p])
			THROW(("error, strings are unsorted"));
		
		while (i > p)
		{
			MTransition new_trans = {};

			new_trans.b.dest = ht.Lookup(larval_state[i], l_state_len[i], mAutomaton);
			new_trans.b.term = is_terminal[i];
			new_trans.b.attr = s0[--i];
			
			larval_state[i][l_state_len[i]++] = new_trans;
		}
		
		while (i < q)
		{
			s0[i] = s1[i];
			is_terminal[++i] = 0;
			l_state_len[i] = 0;
		}
		
		s0[q] = 0;
		is_terminal[q] = w->second;
	}
	
	while (i > 0)
	{
		MTransition new_trans = {};

		new_trans.b.dest = ht.Lookup(larval_state[i], l_state_len[i], mAutomaton);
		new_trans.b.term = is_terminal[i];
		new_trans.b.attr = s0[--i];
		
		larval_state[i][l_state_len[i]++] = new_trans;
	}
	
	uint32 start_state = ht.Lookup(larval_state[0], l_state_len[0], mAutomaton);
	
	MTransition t = { };
	t.b.dest = start_state;
	mAutomaton.push_back(t);
	
	mData.reset(nil);
}

uint32 MRecognizer::Move(uint8 inChar, uint32 inState)
{
	uint32 result = inState;
	
	if (inState > 0)
	{
		uint32 state = inState - 1;
		
		if (state == 0)
			state = mAutomaton.size() - 1;
		
		state = mAutomaton[state].b.dest;
		
		if (state > mAutomaton.size())
			THROW(("Error, state out of bounds"));
		
		while (mAutomaton[state].b.attr != inChar)
		{
			if (mAutomaton[state].b.last)
			{
				result = 0;
				break;
			}
			
			++state;
			
			if (state >= mAutomaton.size())
				THROW(("Error, state out of bounds"));
		}
		
		if (result != 0)
			result = state + 1;
	}
	
	return result;
}

uint8 MRecognizer::IsKeyWord(uint32 inState)
{
	uint8 result = 0;
	if (inState > 0 and inState < mAutomaton.size())
		result = mAutomaton[inState - 1].b.term;
	return result;
}

void MRecognizer::CollectKeyWordsBeginningWith(
	string			inPattern,
	vector<string>&	ioStrings)
{
	bool match = true;
	uint32 state = mAutomaton[mAutomaton.size() - 1].b.dest;
	
	for (uint32 i = 0; state != 0 and match and i < inPattern.length(); ++i)
	{
		while (mAutomaton[state].b.attr != inPattern[i])
		{
			if (mAutomaton[state].b.last)
			{
				match = false;
				break;
			}
			
			++state;
		}
		
		if (match and mAutomaton[state].b.attr == inPattern[i])
			state = mAutomaton[state].b.dest;
		else
			match = false;
	}
	
	if (match and state != 0)
	{
		set<string> keys(ioStrings.begin(), ioStrings.end());
		CollectKeyWordsStartingFromState(state, "", keys, ioStrings);
	}
}

void MRecognizer::CollectKeyWordsStartingFromState(
	uint32			inState,
	string			inWord,
	set<string>&	inKeys,
	vector<string>&	outWords)
{
	uint32 state = inState;

	if (state == 0)
		return;

	for (;;)
	{
		char ch = mAutomaton[state].b.attr;

		if (mAutomaton[state].b.term)
		{
			string word = inWord + ch;
			if (inKeys.count(word) == 0)
			{
				inKeys.insert(word);
				outWords.push_back(word);
			}
		}

		if (mAutomaton[state].b.dest != 0)
		{
			CollectKeyWordsStartingFromState(
				mAutomaton[state].b.dest, inWord + ch, inKeys, outWords);
		}

		if (mAutomaton[state].b.last)
			break;

		++state;
	}
}

// --------------------------------------------------------------------

MNamedRange::MNamedRange()
	: begin(0)
	, end(0)
	, selectFrom(0)
	, selectTo(0)
	, index(0)
{
}

// --------------------------------------------------------------------

MLanguage::MLanguage()
	: mRecognizer(nil)
	, mStyles(nil)
	, mOffsets(nil)
{
}

MLanguage::~MLanguage()
{
	delete mRecognizer;
}

template<class L>
struct LanguageFactory
{
	static L*	Create();
};

template<class L>
L* LanguageFactory<L>::Create()
{
	static auto_ptr<L> sInstance;
	
	if (sInstance.get() == nil)
	{
		sInstance.reset(new L);
		sInstance->Init();
	}
	return sInstance.get();
}

MLanguage*
MLanguage::GetLanguageForDocument(
	const string&	inFile,
	MTextBuffer&	inText)
{
	MLanguage* result = nil;

	if (MLanguageCpp::MatchLanguage(inFile, inText))
		result = LanguageFactory<MLanguageCpp>::Create();
	else if (MLanguageHTML::MatchLanguage(inFile, inText))
		result = LanguageFactory<MLanguageHTML>::Create();
	else if (MLanguagePerl::MatchLanguage(inFile, inText))
		result = LanguageFactory<MLanguagePerl>::Create();
	else if (MLanguageTeX::MatchLanguage(inFile, inText))
		result = LanguageFactory<MLanguageTeX>::Create();
	else if (MLanguageXML::MatchLanguage(inFile, inText))
		result = LanguageFactory<MLanguageXML>::Create();
	
	return result;
}

MLanguage*
MLanguage::GetLanguage(
	const string&		inName)
{
	MLanguage* result = LanguageFactory<MLanguageCpp>::Create();
	if (inName != result->GetName())
		result = LanguageFactory<MLanguagePerl>::Create();
	if (inName != result->GetName())
		result = LanguageFactory<MLanguageHTML>::Create();
	if (inName != result->GetName())
		result = LanguageFactory<MLanguageTeX>::Create();
	if (inName != result->GetName())
		result = LanguageFactory<MLanguageXML>::Create();
	if (inName != result->GetName())
		result = nil;
	
	return result;
}

uint32
MLanguage::StyleLine(
	const MTextBuffer&	inText,
	uint32				inOffset,
	uint32				inLength,
	uint16&				ioStyle,
	uint32				outStyles[],
	uint32				outOffsets[])
{
	mStyles = outStyles;
	mOffsets = outOffsets;
	mLastStyleIndex = 0;
	
	if (mStyles and mOffsets)
	{
		mStyles[0] = 0;
		mOffsets[0] = 0;
	}

	StyleLine(inText, inOffset, inLength, ioStyle);

	mStyles = nil;
	mOffsets = nil;
	
	return mLastStyleIndex + 1;
}

void
MLanguage::SetStyle(
	uint32				inOffset,
	uint32				inStyle)
{
	if (mStyles != nil and mOffsets != nil)
	{
		if (mLastStyleIndex < kMaxStyles and inStyle != mStyles[mLastStyleIndex])
		{
			if (inOffset > mOffsets[mLastStyleIndex])
				++mLastStyleIndex;
			
			mStyles[mLastStyleIndex] = inStyle;
			mOffsets[mLastStyleIndex] = inOffset;
		}
	}
}

uint16
MLanguage::GetInitialState(
	const string&	inFile,
	MTextBuffer&	inText)
{
	return 0;
}

void
MLanguage::AddKeywords(
	const char* inKeywords[])
{
	if (mRecognizer == nil)
	{
		mTag = 0;
		mRecognizer = new MRecognizer;
	}
	
	for (const char** kw = inKeywords; *kw != nil; ++kw)
		mRecognizer->AddWord(*kw, mTag);
	
	++mTag;
}

void
MLanguage::GenerateDFA()
{
	mRecognizer->CreateAutomaton();
}

int
MLanguage::Move(
	char		inChar,
	int			inState)
{
	int result = 0;
	if (inState > 0 and inChar < numeric_limits<uint8>::max())
		result = mRecognizer->Move(inChar, inState);
	return result;
}

int
MLanguage::IsKeyWord(
	int			inState)
{
	int result = 0;

	if (inState > 0)
		result = mRecognizer->IsKeyWord(inState);

	return result;
}

bool
MLanguage::IsBalanceChar(
	wchar_t		inChar)
{
	return false;
}

bool
MLanguage::IsSmartIndentLocation(
	const MTextBuffer&	inText,
	uint32				inOffset)
{
	return false;
}

bool
MLanguage::IsSmartIndentCloseChar(
	wchar_t				inChar)
{
	return false;
}

bool
MLanguage::Equal(
	MTextBuffer::const_iterator inBegin,
	MTextBuffer::const_iterator inEnd,
	const char*					inString)
{
	bool result = true;
	while (result and *inString and inBegin != inEnd and *inBegin and *inBegin != '\n')
	{
		result = *inBegin++ == *inString++;
	}
	return result and *inString == 0;
}

bool MLanguage::Softwrap() const
{
	return true;
}

string
MLanguage::NameForPosition(
	const MNamedRange&	inRanges,
	uint32				inPosition)
{
	string result;
	
	if (inPosition >= inRanges.begin and inPosition < inRanges.end)
	{
		result = inRanges.name;
		
		if (inRanges.subrange.size() > 0)
		{
			int32 L = 0;
			int32 R = static_cast<int32>(inRanges.subrange.size()) - 1;
			
			while (R >= L)
			{
				int32 m = (L + R) / 2;
				
				if (inRanges.subrange[m].begin > inPosition)
					R = m - 1;
				else if (inRanges.subrange[m].end < inPosition)
					L = m + 1;
				else
				{
					if (inRanges.subrange[m].begin <= inPosition and
						inRanges.subrange[m].end > inPosition)
					{
						if (result.length() > 0)
							result += "::";
						result += NameForPosition(inRanges.subrange[m], inPosition);
					}
					
					break;
				}
			}
		}
	}
	
	return result;
}

void
MLanguage::Parse(
	const MTextBuffer&	inText,
	MNamedRange&		outRange,
	MIncludeFileList&	outIncludeFiles)
{
	outRange.name = "";
	outRange.begin = outRange.end = 0;

	outIncludeFiles.clear();
}

void
MLanguage::GetParsePopupItems(
	MNamedRange&		inRanges,
	const std::string&	inNSName,
	MMenu&				inMenu,
	uint32&				ioIndex)
{
	string ns = inNSName;
	if (ns.length() > 0)
		ns += "::";
	ns += inRanges.name;

	if (inRanges.name.length())
	{
		inRanges.index = ioIndex;
		++ioIndex;
		inMenu.AppendItem(ns, cmd_SelectFunctionFromMenu);
	}
	
	for (uint32 i = 0; i < inRanges.subrange.size(); ++i)
		GetParsePopupItems(inRanges.subrange[i], ns, inMenu, ioIndex);
}

bool MLanguage::GetSelectionForParseItem(
	const MNamedRange&	inRanges,
	uint32				inItem,
	uint32&				outSelectionStart,
	uint32&				outSelectionEnd)
{
	bool result = false;
	
	if (inItem == inRanges.index)
	{
		result = true;
		outSelectionStart = inRanges.selectFrom;
		outSelectionEnd = inRanges.selectTo;
	}
	else
	{
		for (vector<MNamedRange>::const_iterator nr = inRanges.subrange.begin();
			result == false and nr != inRanges.subrange.end(); ++nr)
		{
			result = GetSelectionForParseItem(*nr, inItem, outSelectionStart, outSelectionEnd);
		}
	}
	
	return result;
}

void MLanguage::CollectKeyWordsBeginningWith(
	string				inPattern,
	vector<string>&		ioStrings)
{
	if (mRecognizer != nil)
		mRecognizer->CollectKeyWordsBeginningWith(inPattern, ioStrings);
}

