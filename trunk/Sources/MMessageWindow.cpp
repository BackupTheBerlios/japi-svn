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

#include "MJapieG.h"

#include <pcre.h>

#include "MMessageWindow.h"
#include "MListView.h"
#include "MDevice.h"
#include "MGlobals.h"
#include "MUtils.h"
#include "MUnicode.h"
//#include "MApplication.h"
#include "MDocument.h"
//#include "MTextViewContainer.h"
#include "MEditWindow.h"

using namespace std;

namespace
{
	
const uint32
	kListViewID = 200;
	
//static const Rect
//	sRect = { 0, 0, 150, 400 };

const double
	kDelay = 0.333;

const uint32
	kDotWidth				= 8,
	kIconColumnOffset		= 4,
	kFileColumnOffset		= kIconColumnOffset + kDotWidth + 2 * kIconColumnOffset,
	kLineColumnOffset		= kFileColumnOffset + 150,
	kMessageColumnOffset	= kLineColumnOffset + 30;

}

struct MMessageItem
{
	MMessageKind	mKind;
	uint32			mFileNr;
	uint32			mLineNr;
	uint32			mMinOffset, mMaxOffset;
	uint16			mMessageLength;
	char			mMessage[1];

	static MMessageItem*
					Create(MMessageKind inKind, uint32 inFileNr, uint32 inLineNr,
						uint32 inMinOffset, uint32 inMaxOffset, const string& inMessage)
					{
						MMessageItem* item = new(inMessage.length()) MMessageItem;
					
						item->mKind = inKind;
						item->mFileNr = inFileNr;
						item->mLineNr = inLineNr;
						item->mMinOffset = inMinOffset;
						item->mMaxOffset = inMaxOffset;
						item->mMessageLength = inMessage.length();
						inMessage.copy(item->mMessage, item->mMessageLength);
						
						return item;
					}

	uint32			Size() const									{ return sizeof(MMessageItem) + mMessageLength; }

	void*			operator new(size_t inSize, size_t inDataSize)	{ return malloc(inSize + inDataSize); }
	void*			operator new(size_t inSize);
	void			operator delete(void* inPtr)					{ free(inPtr); }
};

typedef vector<MMessageItem*> MMessageItemArray;

struct MMessageListImp
{
	MMessageItemArray	mArray;
	MFileTable			mFileTable;
};

MMessageList::MMessageList()
	: mImpl(new MMessageListImp)
{
}

MMessageList::~MMessageList()
{
	for (uint32 i = 0; i < mImpl->mArray.size(); ++i)
		delete mImpl->mArray[i];
	
	delete mImpl;
}

void MMessageList::AddMessage(
	MMessageKind	inKind,
	const MPath&		inFile,
	uint32			inLine,
	uint32			inMinOffset,
	uint32			inMaxOffset,
	const string&	inMessage)
{
	uint32 fileNr = 0;
	
	if (fs::exists(inFile))
	{
		MFileTable::iterator f = find(mImpl->mFileTable.begin(), mImpl->mFileTable.end(), inFile);
		if (f == mImpl->mFileTable.end())
			f = mImpl->mFileTable.insert(f, inFile);

		fileNr = f - mImpl->mFileTable.begin() + 1;
	}
	
	mImpl->mArray.push_back(MMessageItem::Create(inKind, fileNr, inLine,
		inMinOffset, inMaxOffset, inMessage));
}

uint32 MMessageList::GetCount() const
{
	return mImpl->mArray.size();
}

MMessageWindow::MMessageWindow()
	: eBaseDirChanged(this, &MMessageWindow::SetBaseDirectory)
	, mBaseDirectory("/")
	, mLastAddition(0)
{
    gtk_widget_set_size_request(GTK_WIDGET(GetGtkWidget()), 600, 200);
	
	mList = new MListView(this);
	gtk_container_add(GTK_CONTAINER(GetGtkWidget()), mList->GetGtkWidget());

	SetCallBack(mList->cbDrawItem, this, &MMessageWindow::DrawItem);
	SetCallBack(mList->cbRowSelected, this, &MMessageWindow::SelectItem);
	SetCallBack(mList->cbRowInvoked, this, &MMessageWindow::InvokeItem);
	
	gtk_widget_show_all(GetGtkWidget());

	Show();
}
	
void MMessageWindow::AddMessage(
	MMessageKind		inKind,
	const MPath&			inFile,
	uint32				inLine,
	uint32				inMinOffset,
	uint32				inMaxOffset,
	const string&		inMessage)
{
	auto_ptr<MMessageItem> item(
		MMessageItem::Create(inKind, AddFileToTable(inFile),
			inLine, inMinOffset, inMaxOffset, inMessage));
	
	mList->InsertItem(mList->GetCount(), item.get(), item->Size());
}

void MMessageWindow::AddMessages(
	MMessageList&		inItems)
{
	MMessageListImp* imp = inItems.mImpl;
	
	mFileTable = imp->mFileTable;
	
	mList->RemoveAll();
	for (MMessageItemArray::iterator i = imp->mArray.begin(); i != imp->mArray.end(); ++i)
		mList->InsertItem(mList->GetCount(), (*i), (*i)->Size());
}

void MMessageWindow::SetBaseDirectory(
	const MPath&			inBaseDir)
{
	mBaseDirectory = inBaseDir;
}

void MMessageWindow::SelectItem(
	uint32				inItemNr)
{
	MMessageItem item;
	mList->GetItem(inItemNr, &item, sizeof(item));
	
	if (item.mFileNr > 0)
	{
		MDocument* doc = MDocument::GetDocumentForURL(mFileTable[item.mFileNr - 1], false);
		if (doc == nil)
			doc = new MDocument(&mFileTable[item.mFileNr - 1]);
		
//		if (doc == mController.GetDocument() or
//			mController.TryCloseController(kSaveChangesClosingDocument))
//		{
//			mController.SetDocument(doc);
//	
//			if (item.mMaxOffset > item.mMinOffset)
//				doc->Select(item.mMinOffset, item.mMaxOffset, kScrollToSelection);
//			else if (item.mLineNr > 0)
//				doc->GoToLine(item.mLineNr - 1);
//		}
	}
//	else
//		mController.TryCloseController(kSaveChangesClosingDocument);
}

void MMessageWindow::InvokeItem(
	uint32				inItemNr)
{
	MMessageItem item;
	mList->GetItem(inItemNr, &item, sizeof(item));
	
	if (item.mFileNr > 0)
	{
		MDocument* doc = gApp->OpenOneDocument(mFileTable[item.mFileNr - 1]);
		
		if (doc != nil)
		{
			MEditWindow::DisplayDocument(doc);
		
			if (item.mMaxOffset > item.mMinOffset)
				doc->Select(item.mMinOffset, item.mMaxOffset, kScrollToSelection);
			else if (item.mLineNr > 0)
				doc->GoToLine(item.mLineNr - 1);
		}
	}
}

void MMessageWindow::AddStdErr(
	const char*			inText,
	uint32				inSize)
{
//cout << "AddStdErr: '" << string(inText, inSize) << "'" << endl;

	static pcre* pattern = nil;
	static pcre_extra* info = nil;
	
	if (pattern == nil)
	{
		const char* errmsg;
		int errcode, erroffset;

		pattern = pcre_compile2("^([^:]+):((\\d+):)?( (note|warning|error):)?([^:].+)$",
			PCRE_UTF8 | PCRE_MULTILINE, &errcode, &errmsg, &erroffset, nil);
		
		if (pattern == nil or errcode != 0)
			THROW(("Error compiling regular expression: %s", errmsg));
		
		info = pcre_study(pattern, 0, &errmsg);
		if (errmsg != 0)
			THROW(("Error studying regular expression: %s", errmsg));
	}
		
	mText.append(inText, inSize);

	if (mLastAddition + kDelay < GetLocalTime() or mText.find('\n') != string::npos)
	{
		string::size_type n;
		
		while ((n = mText.find('\n')) != string::npos)
		{
			string line = mText.substr(0, n);
			mText.erase(0, n + 1);
			
//			
//			
//			boost::regex re("^([^:]+):((\\d+):)?( (note|warning|error):)?([^:].+)$",
//				boost::regex_constants::normal);
//		
//			boost::regex_constants::match_flag_type match_flags =
//				boost::regex_constants::match_not_dot_newline |
//				boost::regex_constants::match_not_dot_null |
//				boost::regex_constants::match_continuous;
//
//			boost::match_results<string::iterator> m;

			MPath spec;
			int m[33] = {};

//			if (boost::regex_search(line.begin(), line.end(), m, re, match_flags) and m[0].matched)
			if (pcre_exec(pattern, info, line.c_str(), line.length(), 0, PCRE_NOTEMPTY, m, 33) >= 0)
			{
				string file, warn, l_nr, mesg;
				
				if (m[2] >= 0 and m[3] > m[2])
					file = line.substr(m[2], m[3] - m[2]);
				
				if (m[10] >= 0 and m[11] > m[10])
					warn = line.substr(m[10], m[11] - m[10]);

				if (m[6] >= 0 and m[7] > m[6])
					l_nr = line.substr(m[6], m[7] - m[6]);
				
				if (m[12] >= 0 and m[13] > m[12])
					mesg = line.substr(m[12], m[13] - m[12]);
				
				spec = mBaseDirectory / file;
//	
//try {
//	StOKToThrow ok;
//	cout << "file: " << file << endl;
//	cout << "spec: " << spec.str() << endl;
//} catch (...) {}
				
				if (fs::exists(spec))
				{
					MMessageKind kind = kMsgKindNone;
					if (warn.length() > 0)
					{
						if (warn == "error")
							kind = kMsgKindError;
						else if (warn == "warning")
							kind = kMsgKindWarning;
						else if (warn == "note")
							kind = kMsgKindNote;
					}
					
					uint32 lineNr = 0;
					if (l_nr.length() > 0)
						lineNr = atoi(l_nr.c_str());
					AddMessage(kind, spec, lineNr, 0, 0, mesg);
					
					continue;
				}
			}

			AddMessage(kMsgKindNone, spec, 0, 0, 0, line);
		}
		
		mLastAddition = GetLocalTime();
	}
}

void MMessageWindow::ClearList()
{
	mList->RemoveAll();
	mFileTable.clear();
}

void MMessageWindow::DrawItem(
	MDevice&			inDevice,
	MRect				inFrame,
	uint32				inRow,
	bool				inSelected,
	const void*			inData,
	uint32				inDataLength)
{
	const MMessageItem* item = static_cast<const MMessageItem*>(inData);
	assert(item->Size() == inDataLength);
	
	float x, y;
	x = inFrame.x;
	y = inFrame.y + 1;

	switch (item->mKind)
	{
		case kMsgKindNone:		inDevice.SetForeColor(kMarkedLineColor); break;
		case kMsgKindNote:		inDevice.SetForeColor(kNoteColor); break;
		case kMsgKindWarning:	inDevice.SetForeColor(kWarningColor); break;
		case kMsgKindError:		inDevice.SetForeColor(kErrorColor); break;
	}
	
	float ny = inFrame.y + (inFrame.height - kDotWidth) / 2;
	
	inDevice.FillEllipse(MRect(x + kIconColumnOffset, ny, kDotWidth, kDotWidth));
	inDevice.SetForeColor(kBlack);
	
	if (item->mFileNr > 0)
		inDevice.DrawString(mFileTable[item->mFileNr - 1].leaf(), x + kFileColumnOffset, y, true);

	if (item->mLineNr > 0)
		inDevice.DrawString(NumToString(item->mLineNr), x + kLineColumnOffset, y, true);

	inDevice.DrawString(string(item->mMessage, item->mMessage + item->mMessageLength), 
		x + kMessageColumnOffset, y, true);
}

uint32 MMessageWindow::AddFileToTable(
	const MPath&			inFile)
{
	if (not exists(inFile))
		return 0;
	
	MFileTable::iterator f = find(mFileTable.begin(), mFileTable.end(), inFile);
	if (f == mFileTable.end())
		f = mFileTable.insert(f, inFile);

	return f - mFileTable.begin() + 1;
}

void MMessageWindow::DocumentChanged(
	MDocument*			inDocument)
{
}

bool MMessageWindow::DoClose()
{
//	return mController.TryCloseController(kSaveChangesClosingDocument);
	return MWindow::DoClose();
}

