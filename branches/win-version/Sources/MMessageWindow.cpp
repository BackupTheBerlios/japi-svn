//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <pcre.h>
#include <boost/algorithm/string/trim.hpp>
#include <cmath>

#include "MMessageWindow.h"
#include "MDevice.h"
#include "MUtils.h"
#include "MUnicode.h"
#include "MTextDocument.h"
#include "MEditWindow.h"
#include "MStrings.h"
#include "MError.h"
#include "MJapiApp.h"
#include "MList.h"
#include "MTextController.h"
#include "MTextView.h"

using namespace std;
namespace ba = boost::algorithm;

namespace
{
	
const uint32
	kListViewID = 'tree';
	
//static const Rect
//	sRect = { 0, 0, 150, 400 };

const double
	kDelay = 0.333;

const uint32
	kDotWidth				= 8,
	kIconColumnOffset		= 4,
	kFileColumnOffset		= kIconColumnOffset + kDotWidth + 2 * kIconColumnOffset,
	kLineColumnOffset		= kFileColumnOffset + 175,
	kMessageColumnOffset	= kLineColumnOffset + 35;

//GdkPixbuf* GetBadge(
//	MMessageKind		inKind)
//{
//	const MColor
//		kMsgNoneColor("#efff7f");
//	
//	const uint32
//		kDotSize = 9;
//	
//	static GdkPixbuf* sBadges[] = {
//		CreateDot(kMsgNoneColor, kDotSize),
//		CreateDot(kNoteColor, kDotSize),
//		CreateDot(kWarningColor, kDotSize),
//		CreateDot(kErrorColor, kDotSize)
//	};
//
//	return sBadges[inKind];
//}

}

// --------------------------------------------------------------------

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

typedef vector<MMessageItem*>	MMessageItemArray;
typedef std::vector<MFile>		MFileTable;

enum
{
	kIconColumn,
	kFileColumn,
	kLineColumn,
	kMsgColumn,
	
	kColumnCount	
};

namespace MMsgColumns
{
	struct icon {};
	struct file {};
	struct line {};
	struct msg {};
}

class MMessageRow
	: public MListRow<
			MMessageRow,
			MMsgColumns::icon,			GdkPixbuf*,
			MMsgColumns::file,			string,
			MMsgColumns::line,			string,
			MMsgColumns::msg,			string
		>
{
  public:
					MMessageRow(
						MMessageItem*	inItem,
						const string&	inFile)
						: mItem(inItem)
						, mFile(inFile)
					{
					}
				
	void			GetData(
						const MMsgColumns::icon&,
						GdkPixbuf*&		outIcon)
					{
						outIcon = GetBadge(mItem->mKind);
					}

	void			GetData(
						const MMsgColumns::file&,
						string&			outFile)
					{
						outFile = mFile;
					}

	void			GetData(
						const MMsgColumns::line,
						string&			outLine)
					{
						if (mItem->mLineNr > 0)
							outLine = boost::lexical_cast<string>(mItem->mLineNr);
					}

	void			GetData(
						const MMsgColumns::msg&,
						string&			outMsg)
					{
						outMsg.assign(mItem->mMessage, mItem->mMessageLength);
					}

	virtual bool	RowDropPossible() const		{ return false; }

	MMessageItem*	mItem;
	string			mFile;
};

// --------------------------------------------------------------------

struct MMessageListImp
{
	MMessageItemArray	mArray;
	MFileTable			mFileTable;
	int32				mRefCount;
};

MMessageList::MMessageList()
	: mImpl(new MMessageListImp)
{
	mImpl->mRefCount = 1;
}

MMessageList::MMessageList(
	const MMessageList&	rhs)
	: mImpl(rhs.mImpl)
{
	++mImpl->mRefCount;
}

MMessageList& MMessageList::operator=(
	const MMessageList&	rhs)
{
	if (--mImpl->mRefCount <= 0)
	{
		for (uint32 i = 0; i < mImpl->mArray.size(); ++i)
			delete mImpl->mArray[i];
		
		delete mImpl;
	}
	
	mImpl = rhs.mImpl;

	++mImpl->mRefCount;
	
	return *this;
}

MMessageList::~MMessageList()
{
	if (--mImpl->mRefCount <= 0)
	{
		for (uint32 i = 0; i < mImpl->mArray.size(); ++i)
			delete mImpl->mArray[i];
		
		delete mImpl;
	}
}

void MMessageList::AddMessage(
	MMessageKind	inKind,
	const MFile&	inFile,
	uint32			inLine,
	uint32			inMinOffset,
	uint32			inMaxOffset,
	const string&	inMessage)
{
	uint32 fileNr = 0;
	
	if (inFile.IsLocal() == false or fs::exists(inFile.GetPath()))
	{
		MFileTable::iterator f = find(mImpl->mFileTable.begin(), mImpl->mFileTable.end(), inFile);
		if (f == mImpl->mFileTable.end())
			f = mImpl->mFileTable.insert(f, inFile);

		fileNr = f - mImpl->mFileTable.begin() + 1;
	}
	
	mImpl->mArray.push_back(MMessageItem::Create(inKind, fileNr, inLine,
		inMinOffset, inMaxOffset, inMessage));
}

MMessageItem& MMessageList::GetItem(
	uint32				inIndex) const
{
	if (inIndex >= mImpl->mArray.size())
		THROW(("Message index out of range"));
	return *mImpl->mArray[inIndex];
}

MFile MMessageList::GetFile(
	uint32				inFileNr) const
{
	if (inFileNr >= mImpl->mFileTable.size())
		THROW(("File table index out of range"));
	return mImpl->mFileTable[inFileNr];
}

uint32 MMessageList::GetCount() const
{
	return mImpl->mArray.size();
}

MMessageWindow::MMessageWindow(
	const string& 	inTitle)
	: MDocWindow("message-list-window")
	, eBaseDirChanged(this, &MMessageWindow::SetBaseDirectory)
	, eSelectMsg(this, &MMessageWindow::SelectMsg)
	, eInvokeMsg(this, &MMessageWindow::InvokeMsg)
	, mBaseDirectory("/")
	, mLastAddition(0)
{
	SetTitle(inTitle);
	
	mListView = new MList<MMessageRow>(GetWidget(kListViewID));
	mListView->SetExpandColumn(kMsgColumn);
	
	AddRoute(static_cast<MList<MMessageRow>*>(mListView)->eRowSelected, eSelectMsg);
	AddRoute(static_cast<MList<MMessageRow>*>(mListView)->eRowInvoked, eInvokeMsg);
	
	mListView->SetColumnTitle(kFileColumn, _("File"));
	mListView->SetColumnTitle(kLineColumn, _("Line"));
	mListView->SetColumnAlignment(kLineColumn, 1.0f);
	mListView->SetColumnTitle(kMsgColumn, _("Message"));
	mListView->SetExpandColumn(kMsgColumn);

	// ----------------------------------------------------------------

	MTextController* textController = new MTextController(this);
	mController = textController;
	
	mMenubar.SetTarget(mController);

	// add status 
	
	GtkWidget* statusBar = GetWidget('stat');
	
	GtkShadowType shadow_type;
	gtk_widget_style_get(statusBar, "shadow_type", &shadow_type, nil);
	
	// selection status

	GtkWidget* frame = gtk_frame_new(nil);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow_type);
	
	mSelectionPanel = gtk_label_new("1, 1");
	gtk_label_set_single_line_mode(GTK_LABEL(mSelectionPanel), true);
	gtk_container_add(GTK_CONTAINER(frame), mSelectionPanel);	
	
	gtk_box_pack_start(GTK_BOX(statusBar), frame, false, false, 0);
	gtk_box_reorder_child(GTK_BOX(statusBar), frame, 0);
	gtk_widget_set_size_request(mSelectionPanel, 100, -1);
	
	// parse popups
	
//	mIncludePopup = new MParsePopup(50);
//	frame = gtk_frame_new(nil);
//	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow_type);
//	gtk_container_add(GTK_CONTAINER(frame), mIncludePopup->GetGtkWidget());
//	gtk_box_pack_start(GTK_BOX(statusBar), frame, false, false, 0);
//	gtk_box_reorder_child(GTK_BOX(statusBar), frame, 1);
//	mIncludePopup->SetController(mController, false);
//	
//	mParsePopup = new MParsePopup(200);
//	frame = gtk_frame_new(nil);
//	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow_type);
//	gtk_container_add(GTK_CONTAINER(frame), mParsePopup->GetGtkWidget());
//	gtk_box_pack_start(GTK_BOX(statusBar), frame, true, true, 0);
//	gtk_box_reorder_child(GTK_BOX(statusBar), frame, 2);	
//	mParsePopup->SetController(mController, true);

	// hscrollbar
	GtkWidget* hScrollBar = gtk_hscrollbar_new(nil);
	frame = gtk_frame_new(nil);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow_type);
	gtk_container_add(GTK_CONTAINER(frame), hScrollBar);
	gtk_box_pack_end(GTK_BOX(statusBar), frame, false, false, 0);
//	gtk_box_reorder_child(GTK_BOX(statusBar), frame, 3);
	gtk_widget_set_size_request(hScrollBar, 150, -1);
	
	gtk_widget_show_all(statusBar);
	
	// text view
	
    mTextView = new MTextView(GetWidget('text'), GetWidget('vsbr'), hScrollBar);
	textController->AddTextView(mTextView);

	// ----------------------------------------------------------------

	ConnectChildSignals();

	Show();
}
	
void MMessageWindow::AddMessage(
	MMessageKind		inKind,
	const MFile&		inFile,
	uint32				inLine,
	uint32				inMinOffset,
	uint32				inMaxOffset,
	const string&		inMessage)
{
	string msg(inMessage);
	ba::trim_right(msg);
	
	mList.AddMessage(inKind, inFile, inLine, inMinOffset, inMaxOffset, msg);

	AddMessageToList(mList.GetItem(mList.GetCount() - 1));
}

void MMessageWindow::AddMessageToList(
	MMessageItem&		inItem)
{
	string file;
	if (inItem.mFileNr > 0)
		file = mList.GetFile(inItem.mFileNr - 1).GetFileName();

	mListView->AppendRow(new MMessageRow(&inItem, file), nil);
}

void MMessageWindow::SetMessages(
	const string&		inDescription,
	MMessageList&		inItems)
{
	SetTitle(inDescription);
	
	mList = inItems;

	for (uint32 ix = 0; ix < mList.GetCount(); ++ix)
		AddMessageToList(mList.GetItem(ix));
}

void MMessageWindow::SetBaseDirectory(
	const fs::path&			inBaseDir)
{
	mBaseDirectory = inBaseDir;
}

void MMessageWindow::AddStdErr(
	const char*			inText,
	uint32				inSize)
{
	static pcre* pattern = nil;
	static pcre_extra* info = nil;
	
	if (pattern == nil)
	{
		const char* errmsg;
		int errcode, erroffset;

		pattern = pcre_compile2("^([^:]+):((\\d+):)?( (note|warning|error|fout):)?([^:].+)$",
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
			
			fs::path spec;
			int m[33] = {};

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
				boost::trim_right(mesg);
				
				spec = mBaseDirectory / file;
				
				if (fs::exists(spec))
				{
					MMessageKind kind = kMsgKindNone;
					if (warn.length() > 0)
					{
						if (warn == "error" or warn == "fout")
							kind = kMsgKindError;
						else if (warn == "warning")
							kind = kMsgKindWarning;
						else if (warn == "note")
							kind = kMsgKindNote;
					}
					
					uint32 lineNr = 0;
					if (l_nr.length() > 0)
						lineNr = atoi(l_nr.c_str());
					AddMessage(kind, MFile(spec), lineNr, 0, 0, mesg);
					
					continue;
				}
			}

			AddMessage(kMsgKindNone, MFile(spec), 0, 0, 0, line);
		}
		
		mLastAddition = GetLocalTime();
	}
}

void MMessageWindow::ClearList()
{
	mList = MMessageList();
	mListView->RemoveAll();
}

void MMessageWindow::DocumentChanged(
	MDocument*			inDocument)
{
}

bool MMessageWindow::DoClose()
{
	return mController->TryCloseController(kSaveChangesClosingDocument);
}

void MMessageWindow::SelectMsg(
	MMessageRow*	inItem)
{
	MMessageItem& item = *inItem->mItem;
	
	if (item.mFileNr > 0)
	{
		MFile file = mList.GetFile(item.mFileNr - 1);

		MDocument* doc = MDocument::GetDocumentForFile(file);
		if (doc == nil)
			doc = MDocument::Create<MTextDocument>(file);
		
		if (doc != nil and
			(doc == mController->GetDocument() or mController->TryCloseDocument(kSaveChangesClosingDocument)))
		{
			MTextDocument* textDoc = dynamic_cast<MTextDocument*>(doc);
			if (textDoc != nil)
			{
				mController->SetDocument(doc);
			
				if (item.mMaxOffset > item.mMinOffset)
					textDoc->Select(item.mMinOffset, item.mMaxOffset, kScrollToSelection);
				else if (item.mLineNr > 0)
					textDoc->GoToLine(item.mLineNr - 1);
			}
		}
	}	
}

void MMessageWindow::InvokeMsg(
	MMessageRow*	inItem)
{
	MMessageItem& item = *inItem->mItem;
	
	if (item.mFileNr > 0)
	{
		MFile file = mList.GetFile(item.mFileNr - 1);
		MTextDocument* doc = dynamic_cast<MTextDocument*>(gApp->OpenOneDocument(file));
		
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

