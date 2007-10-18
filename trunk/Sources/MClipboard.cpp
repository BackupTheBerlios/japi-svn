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

#include "MJapieG.h"

#include <list>

#include "MTypes.h"
#include "MClipboard.h"
#include "MUnicode.h"
#include "MError.h"

using namespace std;

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
	: mCount(0)
	, mOwnerChanged(true)
	, mGtkClipboard(gtk_clipboard_get_for_display(gdk_display_get_default(), GDK_SELECTION_CLIPBOARD))
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
	MClipboard::Instance().LoadClipboardIfNeeded();
	
	return mCount > 0;
}

bool MClipboard::IsBlock()
{
	MClipboard::Instance().LoadClipboardIfNeeded();
	
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

	GtkTargetEntry targets[] = {
		{"UTF8_STRING", 0, 0},
		{"COMPOUND_TEXT", 0, 0},
		{"TEXT", 0, 0},
		{"STRING", 0, 0},
	};

cout << "about to call set data" << endl;

	gtk_clipboard_set_with_data(mGtkClipboard, 
		targets, sizeof(targets) / sizeof(GtkTargetEntry),
		&MClipboard::GtkClipboardGet, &MClipboard::GtkClipboardClear, nil);
	
//	gtk_clipboard_set_text(mGtkClipboard, inText.c_str(), inText.length());
	
	mOwnerChanged = false;
}

void MClipboard::AddData(const string& inText)
{
	if (mCount == 0)
		SetData(inText, false);
	else
		mRing[0]->AddData(inText);
}

void MClipboard::GtkClipboardGet(
	GtkClipboard*		inClipboard,
	GtkSelectionData*	inSelectionData,
	guint				inInfo,
	gpointer			inUserDataOrOwner)
{
	MClipboard& self = Instance();	
	
	gtk_selection_data_set_text(inSelectionData, 
		self.mRing[0]->mText.c_str(), self.mRing[0]->mText.length());
}

void MClipboard::GtkClipboardClear(
	GtkClipboard*		inClipboard,
	gpointer			inUserDataOrOwner)
{
	Instance().mOwnerChanged = true;
}

void MClipboard::LoadClipboardIfNeeded()
{
	if (mOwnerChanged)
	{
		string text;
		
		if (gtk_clipboard_wait_is_text_available(mGtkClipboard))
		{
			gchar* text = gtk_clipboard_wait_for_text(mGtkClipboard);
			if (text != nil)
			{
				SetData(text, false);
				g_free(text);
			}
		}
		
		mOwnerChanged = false;
	}
}

