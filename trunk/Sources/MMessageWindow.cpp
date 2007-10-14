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

#if DEBUG
#define BOOST_DISABLE_ASSERTS 1
#endif

#include <boost/regex.hpp>

#include "MMessageWindow.h"
//#include "MListView.h"
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
	const MURL&		inFile,
	uint32			inLine,
	uint32			inMinOffset,
	uint32			inMaxOffset,
	const string&	inMessage)
{
	uint32 fileNr = 0;
	
//	if (exists(inFile))
//	{
//		MFileTable::iterator f = find(mImpl->mFileTable.begin(), mImpl->mFileTable.end(), inFile);
//		if (f == mImpl->mFileTable.end())
//			f = mImpl->mFileTable.insert(f, inFile);
//
//		fileNr = f - mImpl->mFileTable.begin() + 1;
//	}
	
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
{//
//	MView::RegisterSubclass<MTextViewContainer>();
//	MView::RegisterSubclass<MListView>();
//	
//	MDocWindow::Initialize(nil, "Lijst");
//
//	HIViewRef listViewRef;
//	HIViewID id = { kJapieSignature, kListViewID };
//	THROW_IF_OSERROR(::HIViewFindByID(GetContentViewRef(), id, &listViewRef));
//	
//	mList = MView::GetView<MListView>(listViewRef);
//	if (mList == nil)
//		THROW(("Listview not found"));
//
//	SetCallBack(mList->cbDrawItem, this, &MMessageWindow::DrawItem);
//	SetCallBack(mList->cbRowSelected, this, &MMessageWindow::SelectItem);
//	SetCallBack(mList->cbRowInvoked, this, &MMessageWindow::InvokeItem);
//
//	Show();
}
	
void MMessageWindow::AddMessage(
	MMessageKind		inKind,
	const MURL&			inFile,
	uint32				inLine,
	uint32				inMinOffset,
	uint32				inMaxOffset,
	const string&		inMessage)
{
	auto_ptr<MMessageItem> item(
		MMessageItem::Create(inKind, AddFileToTable(inFile),
			inLine, inMinOffset, inMaxOffset, inMessage));
	
//	mList->InsertItem(mList->GetCount(), item.get(), item->Size());
}

void MMessageWindow::AddMessages(
	MMessageList&		inItems)
{
	MMessageListImp* imp = inItems.mImpl;
	
	mFileTable = imp->mFileTable;
	
//	mList->RemoveAll();
//	for (MMessageItemArray::iterator i = imp->mArray.begin(); i != imp->mArray.end(); ++i)
//		mList->InsertItem(mList->GetCount(), (*i), (*i)->Size());
}

void MMessageWindow::SetBaseDirectory(
	const MURL&			inBaseDir)
{
	mBaseDirectory = inBaseDir;
}

void MMessageWindow::SelectItem(
	uint32				inItemNr)
{
	MMessageItem item;
//	mList->GetItem(inItemNr, &item, sizeof(item));
	
	if (item.mFileNr > 0)
	{
		MDocument* doc = MDocument::GetDocumentForURL(mFileTable[item.mFileNr - 1], false);
		if (doc == nil)
			doc = new MDocument(&mFileTable[item.mFileNr - 1]);
		
		if (doc == mController.GetDocument() or
			mController.TryCloseController(kSaveChangesClosingDocument))
		{
			mController.SetDocument(doc);
	
			if (item.mMaxOffset > item.mMinOffset)
				doc->Select(item.mMinOffset, item.mMaxOffset, kScrollToSelection);
			else if (item.mLineNr > 0)
				doc->GoToLine(item.mLineNr - 1);
		}
	}
	else
		mController.TryCloseController(kSaveChangesClosingDocument);
}

void MMessageWindow::InvokeItem(
	uint32				inItemNr)
{
	MMessageItem item;
//	mList->GetItem(inItemNr, &item, sizeof(item));
	
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

	mText.append(inText, inSize);

	if (mLastAddition + kDelay < GetLocalTime() or mText.find('\n') != string::npos)
	{
		string::size_type n;
		
		while ((n = mText.find('\n')) != string::npos)
		{
			string line = mText.substr(0, n);
			mText.erase(0, n + 1);
			
			boost::regex re("^([^:]+):((\\d+):)?( (note|warning|error):)?([^:].+)$",
				boost::regex_constants::normal);
		
			boost::regex_constants::match_flag_type match_flags =
				boost::regex_constants::match_not_dot_newline |
				boost::regex_constants::match_not_dot_null |
				boost::regex_constants::match_continuous;

			boost::match_results<string::iterator> m;

			MURL spec;

			if (boost::regex_search(line.begin(), line.end(), m, re, match_flags) and m[0].matched)
			{
				string file(m[1].first, m[1].second);
				spec = mBaseDirectory / file;
//	
//try {
//	StOKToThrow ok;
//	cout << "file: " << file << endl;
//	cout << "spec: " << spec.str() << endl;
//} catch (...) {}
				
//				if (exists(spec))
//				{
//					MMessageKind kind = kMsgKindNone;
//					if (m[5].matched)
//					{
//						string k(m[5].first, m[5].second);
//						if (k == "error")
//							kind = kMsgKindError;
//						else if (k == "warning")
//							kind = kMsgKindWarning;
//						else if (k == "note")
//							kind = kMsgKindNote;
//					}
//					
//					uint32 lineNr = 0;
//					if (m[3].matched)
//						lineNr = atoi(string(m[3].first, m[3].second).c_str());
//					AddMessage(kind, spec, lineNr, 0, 0, string(m[6].first, m[6].second));
//					
//					continue;
//				}
			}

			AddMessage(kMsgKindNone, spec, 0, 0, 0, line);
		}
		
		mLastAddition = GetLocalTime();
	}
}

void MMessageWindow::ClearList()
{
//	mList->RemoveAll();
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
	
//	MDevice dev(inContext, inFrame);

	if (inSelected)
		inDevice.SetBackColor(gHiliteColor);
	else if ((inRow % 2) == 1)
		inDevice.SetBackColor(gOddRowColor);
	else
		inDevice.SetBackColor(kWhite);
	
	inDevice.EraseRect(inFrame);
	
	float x, y;
	x = inFrame.x;
	y = inFrame.y + inDevice.GetAscent() + 1;

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
		inDevice.DrawString(mFileTable[item->mFileNr - 1].leaf(), x + kFileColumnOffset, y);

	if (item->mLineNr > 0)
		inDevice.DrawString(NumToString(item->mLineNr), x + kLineColumnOffset, y);

	inDevice.DrawString(string(item->mMessage, item->mMessage + item->mMessageLength), x + kMessageColumnOffset, y);
}

uint32 MMessageWindow::AddFileToTable(
	const MURL&			inFile)
{
//try {
//	cout << "add: " << inFile.str() << endl;
//} catch (...) {}
//
//	if (not exists(inFile))
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
	return mController.TryCloseController(kSaveChangesClosingDocument);
}

