//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MClipboard.cpp 158 2007-05-28 10:08:18Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 21 2004 18:21:56
*/

#include "MLib.h"

#include <list>

#include "MTypes.h"
#include "MClipboardImpl.h"
#include "MUnicode.h"
#include "MError.h"

using namespace std;

template<class charT>
basic_ostream<charT>& operator<<(basic_ostream<charT>& lhs, MClipboard::Data& rhs)
{
	lhs << "Data block: '" << rhs.mText << '\'' << endl;
	
	return lhs;
} 

MClipboard::Data::Data(const string& inText, bool inBlock)
	: mText(inText)
	, mBlock(inBlock)
{
}

void MClipboard::Data::SetData(const string& inText, bool inBlock)
{
	mText = inText;
	mBlock = inBlock;
}

void MClipboard::Data::AddData(const string& inText)
{
	mText += inText;
	mBlock = false;
}

MClipboard::MClipboard()
	: mImpl(MClipboardImpl::Create(this))
	, mCount(0)
{
}

MClipboard::~MClipboard()
{
	mImpl->Commit();
	
	for (uint32 i = 0; i < mCount; ++i)
		delete mRing[i];
}

MClipboard& MClipboard::Instance()
{
	static MClipboard sInstance;
	return sInstance;
}

bool MClipboard::HasData()
{
	mImpl->LoadClipboardIfNeeded();
	
//	cout << "Clipboard now contains " << mCount << " data items:" << endl;
//	
//	for (uint32 i = 0; i < mCount; ++i)
//		cout << *mRing[i];
//	
//	cout << endl;

	return mCount > 0;
}

bool MClipboard::IsBlock()
{
	mImpl->LoadClipboardIfNeeded();
	
	return mCount > 0 and mRing[0]->mBlock;
}

void MClipboard::NextInRing()
{
	if (mCount > 0)
	{
		Data* front = mRing[0];
		for (uint32 i = 0; i < mCount - 1; ++i)
			mRing[i] = mRing[i + 1];
		mRing[mCount - 1] = front; 
	}
}

void MClipboard::PreviousInRing()
{
	Data* back = mRing[mCount - 1];
	for (int i = mCount - 2; i >= 0; --i)
		mRing[i + 1] = mRing[i];
	mRing[0] = back;
}

void MClipboard::GetData(string& outText, bool& outIsBlock)
{
	if (mCount == 0)
		THROW(("clipboard error"));

	outText = mRing[0]->mText;
	outIsBlock = mRing[0]->mBlock;
}

void MClipboard::SetData(const string& inText, bool inBlock)
{
	if (mCount >= kClipboardRingSize)
	{
		PreviousInRing();
		mRing[0]->SetData(inText, inBlock);
	}
	else
	{
		Data* newData = new Data(inText, inBlock);
		
		for (int32 i = mCount - 1; i >= 0; --i)
			mRing[i + 1] = mRing[i];

		mRing[0] = newData;
		++mCount;
	}

	mImpl->Commit();
}

void MClipboard::AddData(const string& inText)
{
	if (mCount == 0)
		SetData(inText, false);
	else
		mRing[0]->AddData(inText);
}

