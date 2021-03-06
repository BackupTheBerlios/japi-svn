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
#include "MGlobals.h"
#include "MUtils.h"
#include "MUnicode.h"
#include "MTextDocument.h"
#include "MEditWindow.h"
#include "MStrings.h"
#include "MError.h"
#include "MJapiApp.h"

using namespace std;

namespace
{
	
const uint32
	kListViewID = 'tree';
	
enum {
	kBadgeColumn,
	kFileColumn,
	kLineColumn,
	kTextColumn
};

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

GdkPixbuf* GetBadge(
	MMessageKind		inKind)
{
	const MColor
		kMsgNoneColor("#efff7f");
	
	const uint32
		kDotSize = 9;
	
	static GdkPixbuf* sBadges[] = {
		CreateDot(kMsgNoneColor, kDotSize),
		CreateDot(kNoteColor, kDotSize),
		CreateDot(kWarningColor, kDotSize),
		CreateDot(kErrorColor, kDotSize)
	};

	return sBadges[inKind];
}

// --------------------------------------------------------------------

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

typedef vector<MMessageItem*>	MMessageItemArray;
typedef std::vector<MFile>		MFileTable;

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
	: MWindow("message-list-window", "window")
	, eBaseDirChanged(this, &MMessageWindow::SetBaseDirectory)
	, mInvokeRow(this, &MMessageWindow::InvokeRow)
	, mBaseDirectory("/")
	, mLastAddition(0)
{
	SetTitle(inTitle);
	
	GtkWidget* treeView = GetWidget(kListViewID);
	THROW_IF_NIL((treeView));

	GtkListStore* store = gtk_list_store_new(4,
		GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeView), (GTK_TREE_MODEL(store)));
	
	GtkTreeViewColumn* column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("File"));

	GtkCellRenderer* renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, false);
	gtk_tree_view_column_set_attributes(column, renderer, "pixbuf", kBadgeColumn, nil);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, true);
	gtk_tree_view_column_set_attributes(column, renderer, "text", kFileColumn, nil);

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

	renderer = gtk_cell_renderer_text_new();

	column = gtk_tree_view_column_new_with_attributes (
		_("Line"), renderer, "text", kLineColumn, NULL);

	g_object_set(G_OBJECT(renderer), "xalign", 1.0f, NULL);
	gtk_tree_view_column_set_alignment(column, 1.0f);

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Message"), renderer, "text", kTextColumn, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);
	
	mInvokeRow.Connect(treeView, "row-activated");

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
	mList.AddMessage(inKind, inFile, inLine, inMinOffset, inMaxOffset, inMessage);

	GtkWidget* treeView = GetWidget(kListViewID);
	GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)));

	string line;
	if (inLine > 0)
		line = NumToString(inLine);

	GtkTreeIter iter;
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);  /* Acquire an iterator */
	gtk_list_store_set(GTK_LIST_STORE(store), &iter,
		kBadgeColumn, GetBadge(inKind),
		kFileColumn, inFile.GetFileName().c_str(),
		kLineColumn, line.c_str(),
		kTextColumn, inMessage.c_str(),
		-1);
}

void MMessageWindow::SetMessages(
	const string&		inDescription,
	MMessageList&		inItems)
{
	SetTitle(inDescription);
	
	mList = inItems;

	GtkWidget* treeView = GetWidget(kListViewID);
	GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)));

	GtkTreeIter iter;

	for (uint32 ix = 0; ix < mList.GetCount(); ++ix)
	{
		MMessageItem& item = mList.GetItem(ix);
		
		string file;
		if (item.mFileNr > 0)
			file = mList.GetFile(item.mFileNr - 1).GetFileName();
		
		string msg(item.mMessage, item.mMessageLength);
		boost::trim_right(msg);
		
		string line;
		if (item.mLineNr > 0)
			line = NumToString(item.mLineNr);

		gtk_list_store_append(GTK_LIST_STORE(store), &iter);  /* Acquire an iterator */
		gtk_list_store_set(GTK_LIST_STORE(store), &iter,
			kBadgeColumn, GetBadge(item.mKind),
			kFileColumn, file.c_str(), 
			kLineColumn, line.c_str(),
			kTextColumn, msg.c_str(),
			-1);
	}
}

void MMessageWindow::SetBaseDirectory(
	const fs::path&			inBaseDir)
{
	mBaseDirectory = inBaseDir;
}

void MMessageWindow::SelectItem(
	uint32				inItemNr)
{
//	MMessageItem& item = mList.GetItem(inItemNr);
//	
//	if (item.mFileNr > 0)
//	{
//		MFile url(mList.GetFile(item.mFileNr - 1));
//		
//		MTextDocument* doc = MTextDocument::GetDocumentForFile(url, false);
//		if (doc == nil)
//		{
//			doc = new MTextDocument(&url);
//			doc->DoLoad();
//		}
//		
////		if (doc == mController.GetDocument() or
////			mController.TryCloseController(kSaveChangesClosingDocument))
////		{
////			mController.SetDocument(doc);
////	
////			if (item.mMaxOffset > item.mMinOffset)
////				doc->Select(item.mMinOffset, item.mMaxOffset, kScrollToSelection);
////			else if (item.mLineNr > 0)
////				doc->GoToLine(item.mLineNr - 1);
////		}
//	}
////	else
////		mController.TryCloseController(kSaveChangesClosingDocument);
}

void MMessageWindow::InvokeItem(
	uint32				inItemNr)
{
	MMessageItem& item = mList.GetItem(inItemNr);
	
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

	GtkWidget* treeView = GetWidget(kListViewID);
	GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)));
	gtk_list_store_clear(store);
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

void MMessageWindow::InvokeRow(
	GtkTreePath*		inPath,
	GtkTreeViewColumn*	inColumn)
{
	int32 item = -1;

	char* path = gtk_tree_path_to_string(inPath);
	if (path != nil)
	{
		item = atoi(path);
		g_free(path);
	}
	
	if (item >= 0 and item < int32(mList.GetCount()))
	{
		try
		{
			InvokeItem(item);
		}
		catch (...) {}
	}
}
