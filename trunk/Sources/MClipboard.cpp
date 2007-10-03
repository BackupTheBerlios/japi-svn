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

/*	$Id: MClipboard.cpp 158 2007-05-28 10:08:18Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 21 2004 18:21:56
*/

#include "Japie.h"

#include <list>

#include "MTypes.h"
#include "MClipboard.h"
#include "MUnicode.h"
#include "MError.h"

using namespace std;

MClipboard::Data::Data(const ustring& inText, bool inBlock)
	: mText(inText)
	, mBlock(inBlock)
{
}

void MClipboard::Data::SetData(const ustring& inText, bool inBlock)
{
	mText = inText;
	mBlock = inBlock;
}

void MClipboard::Data::AddData(const ustring& inText)
{
	mText += inText;
	mBlock = false;
}

MClipboard::MClipboard()
	: mCount(0)
	, mScrap(kScrapClipboardScrap)
{
}

MClipboard::~MClipboard()
{
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
	MClipboard::Instance().LoadOSScrapIfNewer();
	
	return mCount > 0;
}

bool MClipboard::IsBlock()
{
	MClipboard::Instance().LoadOSScrapIfNewer();
	
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

void MClipboard::GetData(ustring& outText, bool& outIsBlock)
{
	if (mCount == 0)
		THROW(("clipboard error"));

	outText = mRing[0]->mText;
	outIsBlock = mRing[0]->mBlock;
}

void MClipboard::SetData(const ustring& inText, bool inBlock)
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
	
	mScrap.PutScrap(inText);
}

void MClipboard::AddData(const ustring& inText)
{
	if (mCount == 0)
		SetData(inText, false);
	else
		mRing[0]->AddData(inText);
}

void MClipboard::LoadOSScrapIfNewer()
{
	ustring text;
	if (mScrap.LoadOSScrapIfNewer(text))
		SetData(text, false);
}

