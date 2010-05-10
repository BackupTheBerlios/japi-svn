//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <boost/algorithm/string.hpp>

#include "MJapiApp.h"

#include "MePubDocument.h"
#include "MePubWindow.h"
#include "MePubItem.h"
#include "MTextDocument.h"
#include "MGtkWrappers.h"
#include "MStrings.h"
#include "MUtils.h"
#include "MPreferences.h"
#include "MePubContentFile.h"
#include "MList.h"

#include "MError.h"

using namespace std;
namespace ba = boost::algorithm;

// ---------------------------------------------------------------------------

namespace {

struct MePubState
{
	uint16			mWindowPosition[2];
	uint16			mWindowSize[2];
	uint8			mSelectedPanel;
	uint8			mFillers[3];
	
	void			Swap();
};

const char
	kJapieePubState[] = "com.hekkelman.japi.ePubState";

const uint32
	kMePubStateSize = sizeof(MePubState);

void MePubState::Swap()
{
	net_swapper swap;
	
	mWindowPosition[0] = swap(mWindowPosition[0]);
	mWindowPosition[1] = swap(mWindowPosition[1]);
	mWindowSize[0] = swap(mWindowSize[0]);
	mWindowSize[1] = swap(mWindowSize[1]);
}

enum {
	kFilesListViewID		= 'tre1',
	
	kDCIDViewID				= 'dcID',
	kDCTitleViewID			= 'dcTI',
	kDCLanguageViewID		= 'dcLA',
	kDCCreatorViewID		= 'dcCR',
	kDCPublisherViewID		= 'dcPU',
	kDCDateViewID			= 'dcDT',
	kDCDescriptionViewID	= 'dcDE',
	kDCCoverageViewID		= 'dcCO',
	kDCSourceViewID			= 'dcSO',
	kDCRightsViewID			= 'dcRI',
	kDCSubjectViewID		= 'dcSU',
	
	kTOCListViewID			= 'tre3',
	
	kDocIDSchemeViewID		= 'docS',
	kDocIDGenerateButtonID	= 'genI',
	
	kNoteBookID				= 'note'
};

enum {
	kInfoPageNr,
	kFilesPageNr,
	kTOCPageNr
};

const char* kKnownMediaTypes[] = {
	"application/xhtml+xml",
	"application/xml",
	"application/x-dtbncx+xml",
	"application/x-dtbook+xml",
	"image/gif",
	"image/jpeg",
	"image/png",
	"image/svg+xml",
	"text/css",
	"text/x-oeb1-css",
	"text/x-oeb1-document"
};

const uint32 kKnownMediaTypesCount = sizeof(kKnownMediaTypes) / sizeof(const char*);

const string kBaseURL = "http://localhost:9090/";

}

// ---------------------------------------------------------------------------

enum {
	kePubFileNameColumn,
	kePubFileIDColumn,
	kePubFileLinearColumn,
	kePubFileMediaTypeColumn,
	kePubFileDataSizeColumn,
	kePubFileDirtyColumn,

	kePubFileColumnCount
};

// ---------------------------------------------------------------------------

namespace MePubFileFields
{
	struct name {};
	struct id {};
	struct linear {};
	struct mediatype {};
	struct size {};
	struct dirty {};
}

class MePubFileRow : public MListRow<
	MePubFileRow,
	MePubFileFields::name,		string,
	MePubFileFields::id,		string,
	MePubFileFields::linear,	bool,
	MePubFileFields::mediatype,	vector<string>,
	MePubFileFields::size,		string,
	MePubFileFields::dirty,		GdkPixbuf*
>
{
  public:
					MePubFileRow(
						MProjectItem*	inItem,
						MePubDocument*	inEPub)
						: mItem(inItem)
						, mEPubItem(dynamic_cast<MePubItem*>(inItem))
					{
						AddRoute(inEPub->eItemRenamed, eItemRenamed);
					}
	
	void			GetData(
						const MePubFileFields::name&,
						string&		outData)
					{
						outData = mItem->GetName();
					}
	
	void			GetData(
						const MePubFileFields::id&,
						string&		outData)
					{
						if (mEPubItem != nil)
							outData = mEPubItem->GetID();
					}

	void			GetData(
						const MePubFileFields::linear&,
						bool&		outData)
					{
						if (mEPubItem != nil)
							outData = mEPubItem->IsLinear();
					}

	void			GetData(
						const MePubFileFields::mediatype&,
						string&		outData)
					{
						if (mEPubItem != nil)
							outData = mEPubItem->GetMediaType();
					}

	void			GetData(
						const MePubFileFields::size&,
						string&		outData)
					{
						uint32 size = mItem->GetDataSize();
						
						stringstream s;
						if (size >= 1024 * 1024 * 1024)
							s << (size / (1024 * 1024 * 1024)) << 'G';
						else if (size >= 1024 * 1024)
							s << (size / (1024 * 1024)) << 'M';
						else if (size >= 1024)
							s << (size / (1024)) << 'K';
						else if (size > 0)
							s << size;
						
						outData = s.str();
					}

	void			GetData(
						const MePubFileFields::dirty&,
						GdkPixbuf*&		outData)
					{
						const uint32 kDotSize = 6;
						const MColor
							kDirtyColor = MColor("#ff664c");
						
						static GdkPixbuf* kDirtyDot = CreateDot(kDirtyColor, kDotSize);

						if (mItem->IsOutOfDate())
							outData = kDirtyDot;
						else
							outData = nil;
					}

	virtual void	ColumnEdited(
						uint32			inColumnNr,
						const string&	inNewText)
					{
						switch (inColumnNr)
						{
							case kePubFileNameColumn:
							{
								string oldName = mItem->GetName();
								mItem->SetName(inNewText);
								RowChanged();
								eItemRenamed(mEPubItem, oldName, inNewText);
								break;
							}
							
							case kePubFileIDColumn:
								mEPubItem->SetID(inNewText);
								RowChanged();
								break;
							
							case kePubFileMediaTypeColumn:
								mEPubItem->SetMediaType(inNewText);
								RowChanged();
								break;
						}
					}
	
	virtual void	ColumnToggled(
						uint32			inColumnNr)
					{
						if (mEPubItem != nil and inColumnNr == kePubFileLinearColumn)
						{
							mEPubItem->SetLinear(not mEPubItem->IsLinear());
							RowChanged();
						}
					}


	virtual bool	RowDropPossible() const		{ return dynamic_cast<MProjectGroup*>(mItem) != nil; }

	MProjectItem*	GetProjectItem() const				{ return mItem; }
	void			SetProjectItem(
						MProjectItem*	inItem)			{ mItem = inItem; mEPubItem = dynamic_cast<MePubItem*>(inItem); }
	
	MePubItem*		GetEPubItem() const					{ return mEPubItem; }
	
	MEventOut<void(MProjectItem*,const string&,const string&)>
					eItemRenamed;
	
  private:
	MProjectItem*	mItem;
	MePubItem*		mEPubItem;
};

// ---------------------------------------------------------------------------

enum {
	kTOCTitleColumn,
	kTOCIDColumn,
	kTOCSrcColumn,
	kTOCClassColumn,
	kTOCPlayOrderColumn,

	kTOCColumnCount
};

namespace MePubTOCFields
{
	struct title {};
	struct id {};
	struct src {};
	struct cls {};
	struct po {};
}

class MePubTOCRow : public MListRow<
	MePubTOCRow,
	MePubTOCFields::title,	string,
	MePubTOCFields::id,		string,
	MePubTOCFields::src,	string,
	MePubTOCFields::cls,	string,
	MePubTOCFields::po,		uint32
>
{
  public:
					MePubTOCRow(
						MProjectItem* inTOCItem,
						MePubDocument*	inEPub)
						: mItem(inTOCItem)
						, mTOCItem(dynamic_cast<MePubTOCItem*>(inTOCItem))
					{
					}
	
	void			GetData(
						const MePubTOCFields::title&,
						string&			outData)
					{
						outData = mItem->GetName();
					}

	void			GetData(
						const MePubTOCFields::id&,
						string&			outData)
					{
						outData = mTOCItem->GetId();
					}

	void			GetData(
						const MePubTOCFields::src&,
						string&			outData)
					{
						if (mTOCItem != nil)
							outData = mTOCItem->GetSrc();
					}

	void			GetData(
						const MePubTOCFields::cls&,
						string&			outData)
					{
						if (mTOCItem != nil)
							outData = mTOCItem->GetClass();
					}

	void			GetData(
						const MePubTOCFields::po&,
						uint32&			outData)
					{
						outData = mTOCItem->GetPlayOrder();
					}

	virtual void	ColumnEdited(
						uint32			inColumnNr,
						const string&	inNewText)
					{
						switch (inColumnNr)
						{
							case kTOCTitleColumn:
								mItem->SetName(inNewText);
								RowChanged();
								break;
							
							case kTOCIDColumn:
								mTOCItem->SetId(inNewText);
								RowChanged();
								break;
							
							case kTOCSrcColumn:
								mTOCItem->SetSrc(inNewText);
								RowChanged();
								break;
							
							case kTOCClassColumn:
								mTOCItem->SetClass(inNewText);
								RowChanged();
								break;

							case kTOCPlayOrderColumn:
								mTOCItem->SetPlayOrder(boost::lexical_cast<uint32>(inNewText));
								RowChanged();
								break;
							
						}
					}

	virtual bool	RowDropPossible() const		{ return dynamic_cast<MProjectGroup*>(mItem) != nil; }

	MProjectItem*	GetProjectItem()			{ return mItem; }
	void			SetProjectItem(
						MProjectItem*	inItem)	{ mItem = inItem; mTOCItem = dynamic_cast<MePubTOCItem*>(inItem); }

	MePubTOCItem*	GetTOCItem()				{ return mTOCItem; }

  private:
	MProjectItem*	mItem;
	MePubTOCItem*	mTOCItem;
};

// ---------------------------------------------------------------------------

MePubWindow::MePubWindow()
	: MDocWindow("epub-window")
	, eKeyPressEvent(this, &MePubWindow::OnKeyPressEvent)
	, eSelectFileRow(this, &MePubWindow::SelectFileRow)
	, eInvokeFileRow(this, &MePubWindow::InvokeFileRow)
	, eDraggedFileRow(this, &MePubWindow::DraggedFileRow)
	, eSelectTOCRow(this, &MePubWindow::SelectTOCRow)
	, eInvokeTOCRow(this, &MePubWindow::InvokeTOCRow)
	, eDraggedTOCRow(this, &MePubWindow::DraggedTOCRow)
	, eSubjectChanged(this, &MePubWindow::SubjectChanged)
	, eDateChanged(this, &MePubWindow::DateChanged)
	, mFileTree(nil)
	, mTOCTree(nil)
{
	mController = new MController(this);
	
	mMenubar.Initialize(GetWidget('mbar'), "epub-window-menu");
	mMenubar.SetTarget(mController);

	ConnectChildSignals();

	GtkWidget* wdgt = GetWidget(kDCSubjectViewID);
	if (wdgt)
		eSubjectChanged.Connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(wdgt))), "changed");

	wdgt = GetWidget(kDCDateViewID);
	if (wdgt)
		eDateChanged.Connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(wdgt))), "changed");
}

MePubWindow::~MePubWindow()
{
	delete mFileTree;
	mFileTree = nil;
	
	delete mTOCTree;
	mTOCTree = nil;
}

void MePubWindow::Initialize(
	MDocument*		inDocument)
{
	mEPub = dynamic_cast<MePubDocument*>(inDocument);
	
	if (mEPub == nil)
		THROW(("Invalid document type passed"));
	
	MDocWindow::Initialize(inDocument);
	
	vector<string> knownMediaTypes(
		kKnownMediaTypes, kKnownMediaTypes + kKnownMediaTypesCount);
	
	MList<MePubFileRow>* fileTree = new MList<MePubFileRow>(GetWidget(kFilesListViewID));
	mFileTree = fileTree;
	mFileTree->AllowMultipleSelectedItems();
	AddRoute(fileTree->eRowSelected, eSelectFileRow);
	AddRoute(fileTree->eRowInvoked, eInvokeFileRow);
	AddRoute(fileTree->eRowDragged, eDraggedFileRow);
	mFileTree->SetColumnTitle(kePubFileNameColumn, _("File"));
	mFileTree->SetExpandColumn(kePubFileNameColumn);
	mFileTree->SetColumnEditable(kePubFileNameColumn, true);
	mFileTree->SetColumnTitle(kePubFileIDColumn, _("ID"));
	mFileTree->SetColumnTitle(kePubFileLinearColumn, _("Linear"));
	mFileTree->SetColumnTitle(kePubFileMediaTypeColumn, _("Media-Type"));
	mFileTree->SetListOfOptionsForColumn(kePubFileMediaTypeColumn, knownMediaTypes);
	mFileTree->SetColumnTitle(kePubFileDataSizeColumn, _("Size"));
	mFileTree->SetColumnAlignment(kePubFileDataSizeColumn, 1.0f);
	AddItemsToList(mEPub->GetFiles(), nil, fileTree);
	mFileTree->ExpandAll();
	
	eKeyPressEvent.Connect(fileTree->GetGtkWidget(), "key-press-event");

	eKeyPressEvent.Connect(G_OBJECT(fileTree->GetGtkWidget()), "key-press-event");

	MList<MePubTOCRow>* tocTree = new MList<MePubTOCRow>(GetWidget(kTOCListViewID));
	mTOCTree = tocTree;
	AddRoute(tocTree->eRowSelected, eSelectTOCRow);
	AddRoute(tocTree->eRowInvoked, eInvokeTOCRow);
	AddRoute(tocTree->eRowDragged, eDraggedTOCRow);
	tocTree->SetColumnTitle(kTOCTitleColumn, _("Title"));
	tocTree->SetExpandColumn(kTOCTitleColumn);
	tocTree->SetColumnEditable(kTOCTitleColumn, true);
	tocTree->SetColumnTitle(kTOCIDColumn, _("ID"));
	tocTree->SetColumnTitle(kTOCSrcColumn, _("Source"));
	tocTree->SetColumnTitle(kTOCClassColumn, _("Class"));
	tocTree->SetColumnTitle(kTOCPlayOrderColumn, _("Play Order"));
	AddItemsToList(mEPub->GetTOC(), nil, tocTree);
	tocTree->ExpandAll();

	eKeyPressEvent.Connect(tocTree->GetGtkWidget(), "key-press-event");

	// fill in the information fields
	
	SetText(kDCIDViewID,			mEPub->GetDocumentID());
	SetText(kDocIDSchemeViewID,		mEPub->GetDocumentIDScheme());

	SetText(kDCTitleViewID,			mEPub->GetDublinCoreValue("title"));
	SetText(kDCLanguageViewID,		mEPub->GetDublinCoreValue("language"));
	SetText(kDCCreatorViewID,		mEPub->GetDublinCoreValue("creator"));
	SetText(kDCPublisherViewID,		mEPub->GetDublinCoreValue("publisher"));
	SetText(kDCDateViewID,			mEPub->GetDublinCoreValue("date"));
	SetText(kDCDescriptionViewID,	mEPub->GetDublinCoreValue("description"));
	SetText(kDCCoverageViewID,		mEPub->GetDublinCoreValue("coverage"));
	SetText(kDCSourceViewID,		mEPub->GetDublinCoreValue("source"));
	SetText(kDCRightsViewID,		mEPub->GetDublinCoreValue("rights"));
	SetText(kDCSubjectViewID,		mEPub->GetDublinCoreValue("subject"));

	bool useState = false;
	MePubState state = {};
	
	if (Preferences::GetInteger("save state", 1))
	{
		ssize_t r = mEPub->GetFile().ReadAttribute(kJapieePubState, &state, kMePubStateSize);
		useState = static_cast<uint32>(r) == kMePubStateSize;
	}
	
	if (useState)
	{
		state.Swap();

		if (state.mWindowSize[0] > 50 and state.mWindowSize[1] > 50 and
			state.mWindowSize[0] < 2000 and state.mWindowSize[1] < 2000)
		{
			MRect r(
				state.mWindowPosition[0], state.mWindowPosition[1],
				state.mWindowSize[0], state.mWindowSize[1]);
		
			SetWindowPosition(r);
		}
		
		MGtkNotebook book(GetWidget(kNoteBookID));
		book.SetPage(state.mSelectedPanel);
	}
}

template<class R>
void MePubWindow::AddItemsToList(
	MProjectGroup*		inGroup,
	MListRowBase*		inParent,
	MList<R>*			inList)
{
	for (auto iter = inGroup->GetItems().begin(); iter != inGroup->GetItems().end(); ++iter)
	{
		R* item = new R(*iter, mEPub);
		
		inList->AppendRow(item, static_cast<R*>(inParent));
		
		MProjectGroup* group = dynamic_cast<MProjectGroup*>(*iter);
		if (group != nil)
			AddItemsToList(group, item, inList);
	}
}

bool MePubWindow::DoClose()
{
	bool result = true;

	if (GetDocument() != nil)
	{
		MProjectGroup* files = mEPub->GetFiles();
		
		for (MProjectGroup::iterator item = files->begin(); item != files->end(); ++item)
		{
			MProjectItem* pItem = &(*item);
			
			MePubItem* epubItem = dynamic_cast<MePubItem*>(pItem);
			if (epubItem == nil)
				continue;
			
			MFile file(new MePubContentFile(mEPub, epubItem->GetPath()));
	
			MDocument* doc = MDocument::GetDocumentForFile(file);
			if (doc == nil)
				continue;
			
			MController* controller = doc->GetFirstController();
			if (not controller->TryCloseController(kSaveChangesClosingDocument))
			{
				result = false;
				break;
			}
		}
	}
	
	if (result == true)
		result = MDocWindow::DoClose();
	
	return result;
}

// ---------------------------------------------------------------------------
//	SaveState

void MePubWindow::SaveState()
{
	try
	{
		MePubState state = { };

		mEPub->GetFile().ReadAttribute(kJapieePubState, &state, kMePubStateSize);
		
		state.Swap();

		MGtkNotebook book(GetWidget(kNoteBookID));
		state.mSelectedPanel = book.GetPage();

		MRect r;
		GetWindowPosition(r);
		state.mWindowPosition[0] = r.x;
		state.mWindowPosition[1] = r.y;
		state.mWindowSize[0] = r.width;
		state.mWindowSize[1] = r.height;

		state.Swap();
		
		mEPub->GetFile().WriteAttribute(kJapieePubState, &state, kMePubStateSize);
	}
	catch (...) {}
}

bool MePubWindow::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = true;
	
	MGtkNotebook notebook(GetWidget(kNoteBookID));

	switch (inCommand)
	{
		case cmd_NewGroup:
			outEnabled =
				notebook.GetPage() == kFilesPageNr;
			break;

		case cmd_EPubShowInBrowser:
			outEnabled = true;
			break;
		
		case cmd_AddFileToProject:
			outEnabled =
				notebook.GetPage() == kFilesPageNr or
				notebook.GetPage() == kTOCPageNr;
			break;
		
		default:
			result = MDocWindow::UpdateCommandStatus(inCommand, inMenu, inItemIndex, outEnabled, outChecked);
	}
	
	return result;
}

bool MePubWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;

	switch (inCommand)
	{
		case cmd_NewGroup:
			CreateNew(true);
			break;

		case cmd_AddFileToProject:
			CreateNew(false);
			break;
		
		case cmd_EPubShowInBrowser:
			OpenURI(kBaseURL + mEPub->GetDocumentID());
			break;
		
		default:
			result = MDocWindow::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
			break;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	OnKeyPressEvent

bool MePubWindow::OnKeyPressEvent(
	GdkEventKey*	inEvent)
{
	bool result = false;
	
	uint32 modifiers = inEvent->state & gtk_accelerator_get_default_mod_mask();
	uint32 keyValue = inEvent->keyval;
	
	if (modifiers == 0 and (keyValue == GDK_BackSpace or keyValue == GDK_Delete))
	{
		MGtkNotebook notebook(GetWidget(kNoteBookID));

		if (notebook.GetPage() == kFilesPageNr)
			DeleteSelectedFileItems();
		else
			DeleteSelectedTOCItems();

		result = true;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectWindow::SelectFileRow

void MePubWindow::SelectFileRow(
	MePubFileRow*	inRow)
{
	MePubItem* ePubItem = inRow->GetEPubItem();
	
	mFileTree->SetColumnEditable(kePubFileNameColumn, true);
	mFileTree->SetColumnEditable(kePubFileIDColumn, ePubItem != nil);
	mFileTree->SetColumnEditable(kePubFileMediaTypeColumn, ePubItem != nil);
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeFileRow

void MePubWindow::InvokeFileRow(
	MePubFileRow*	inRow)
{
	MePubItem* ePubItem = inRow->GetEPubItem();

	if (ePubItem != nil)
	{
		if (ePubItem->IsEncrypted())
			THROW(("Cannot open an encrypted ePub item"));
		
		fs::path path = ePubItem->GetPath();

		if (ba::starts_with(ePubItem->GetMediaType(), "image/") or
			ePubItem->GetMediaType() == "application/x-font-ttf")
		{
			string url = kBaseURL + mEPub->GetDocumentID() + '/' + path.string();
			OpenURI(url);
		}
		else
		{
			MFile file(new MePubContentFile(mEPub, path));
			
			MDocument* doc = MDocument::GetDocumentForFile(file);
	
			if (doc == nil)
			{
				doc = MDocument::Create<MTextDocument>(file);
				AddRoute(doc->eFileSpecChanged, eFileSpecChanged);
			}
	
			gApp->DisplayDocument(doc);
		}
	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::DraggedFileRow

void MePubWindow::DraggedFileRow(
	MePubFileRow*	inRow)
{
	MProjectItem* item = inRow->GetProjectItem();
	
	MePubFileRow* parent;
	uint32 position;
	if (inRow->GetParentAndPosition(parent, position))
	{
		MProjectGroup* oldGroup = item->GetParent();
		MProjectGroup* newGroup;
		
		if (parent != nil)
			newGroup = dynamic_cast<MProjectGroup*>(parent->GetProjectItem());
		else
			newGroup = mEPub->GetFiles();

		if (oldGroup != nil)
			oldGroup->RemoveProjectItem(item);

		if (newGroup != nil)
			newGroup->AddProjectItem(item, position);
	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::SelectTOCRow

void MePubWindow::SelectTOCRow(
	MePubTOCRow*	inRow)
{
	mTOCTree->SetColumnEditable(kTOCTitleColumn, true);
	mTOCTree->SetColumnEditable(kTOCIDColumn, true);
	mTOCTree->SetColumnEditable(kTOCSrcColumn, true);
	mTOCTree->SetColumnEditable(kTOCClassColumn, true);
	mTOCTree->SetColumnEditable(kTOCPlayOrderColumn, true);
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeTOCRow

void MePubWindow::InvokeTOCRow(
	MePubTOCRow*	inRow)
{
	MePubTOCItem* tocItem = inRow->GetTOCItem();

	if (tocItem != nil)
	{
		string source = tocItem->GetSrc();

		MFile file(mEPub->GetFileForSrc(source));
		MDocument* doc = MDocument::GetDocumentForFile(file);
	
		if (doc == nil)
		{
			doc = MDocument::Create<MTextDocument>(file);
			AddRoute(doc->eFileSpecChanged, eFileSpecChanged);
		}

		gApp->DisplayDocument(doc);
	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::DraggedTOCRow

void MePubWindow::DraggedTOCRow(
	MePubTOCRow*	inRow)
{
	MProjectItem* item = inRow->GetProjectItem();
	
	MePubTOCRow* parent;
	uint32 position;
	if (inRow->GetParentAndPosition(parent, position))
	{
		MProjectGroup* oldGroup = item->GetParent();
		MProjectGroup* newGroup;
		
		if (parent != nil)
			newGroup = dynamic_cast<MProjectGroup*>(parent->GetTOCItem());
		else
			newGroup = mEPub->GetFiles();

		if (oldGroup != nil)
			oldGroup->RemoveProjectItem(item);

		if (newGroup != nil)
			newGroup->AddProjectItem(item, position);
	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::ValueChanged

void MePubWindow::ValueChanged(
	uint32			inID)
{
	switch (inID)
	{
		case kDCIDViewID:
			mEPub->SetDocumentID(GetText(inID));
			break;

		case kDCTitleViewID:
			mEPub->SetDublinCoreValue("title", GetText(inID));
			break;

		case kDCLanguageViewID:
			mEPub->SetDublinCoreValue("language", GetText(inID));
			break;

		case kDCCreatorViewID:
			mEPub->SetDublinCoreValue("creator", GetText(inID));
			break;

		case kDCPublisherViewID:
			mEPub->SetDublinCoreValue("publisher", GetText(inID));
			break;

		case kDCDateViewID:
			mEPub->SetDublinCoreValue("date", GetText(inID));
			break;

		case kDCDescriptionViewID:
			mEPub->SetDublinCoreValue("description", GetText(inID));
			break;

		case kDCCoverageViewID:
			mEPub->SetDublinCoreValue("coverage", GetText(inID));
			break;

		case kDCSourceViewID:
			mEPub->SetDublinCoreValue("source", GetText(inID));
			break;

		case kDCRightsViewID:
			mEPub->SetDublinCoreValue("rights", GetText(inID));
			break;

		case kDCSubjectViewID:
			mEPub->SetDublinCoreValue("subject", GetText(inID));
			break;
		
		case kDocIDGenerateButtonID:
			mEPub->GenerateNewDocumentID();
			SetText(kDCIDViewID, mEPub->GetDocumentID());
			SetText(kDocIDSchemeViewID, mEPub->GetDocumentIDScheme());
			break;

		case kDocIDSchemeViewID:
			mEPub->SetDocumentIDScheme(GetText(inID));
			break;
	}
}

// ---------------------------------------------------------------------------
//	CreateNew

void MePubWindow::CreateNew(
	bool 		inDirectory)
{
	MGtkNotebook notebook(GetWidget(kNoteBookID));
	
	MListBase* list;
	MListRowBase* row;
	MProjectItem* item;
	
	if (notebook.GetPage() == kFilesPageNr)
	{
		list = mFileTree;
		if (inDirectory)
			item = new MProjectGroup(_("New Folder"), nil);
		else
			item = new MePubItem(_("New File"), nil);
		row = new MePubFileRow(item, mEPub);
	}
	else
	{
		list = mTOCTree;
		item = new MePubTOCItem(_("New TOC Entry"), nil);
		row = new MePubTOCRow(item, mEPub);
	}

	MListRowBase* cursor = list->GetCursorRow();
	list->InsertRow(row, cursor);
	
	if (notebook.GetPage() == kFilesPageNr)
		DraggedFileRow(static_cast<MePubFileRow*>(row));
	else
		DraggedTOCRow(static_cast<MePubTOCRow*>(row));
	
	list->SelectRowAndStartEditingColumn(row, 0);
}

void MePubWindow::DeleteSelectedFileItems()
{
	auto rows = static_cast<MList<MePubFileRow>*>(mFileTree)->GetSelectedRows();
	
	// first remove the project items from the MProject
	MProjectGroup* files = mEPub->GetFiles();
	
	for (auto row = rows.begin(); row != rows.end(); ++row)
	{
		MProjectItem* item = (*row)->GetProjectItem();
		if (files->Contains(item))	// be very paranoid
		{
			assert(item->GetParent() != nil);

			item->GetParent()->RemoveProjectItem(item);
			delete item;
			
			(*row)->SetProjectItem(nil);
		}
	}
	
	for (auto row = rows.begin(); row != rows.end(); ++row)
	{
		mFileTree->RemoveRow(*row);
		delete *row;
	}
}

void MePubWindow::DeleteSelectedTOCItems()
{
	auto rows = static_cast<MList<MePubTOCRow>*>(mTOCTree)->GetSelectedRows();
	
	// first remove the project items from the MProject
	MProjectGroup* files = mEPub->GetTOC();
	
	for (auto row = rows.begin(); row != rows.end(); ++row)
	{
		MProjectItem* item = (*row)->GetProjectItem();
		if (files->Contains(item))	// be very paranoid
		{
			assert(item->GetParent() != nil);

			item->GetParent()->RemoveProjectItem(item);
			delete item;
			
			(*row)->SetProjectItem(nil);
		}
	}
	
	for (auto row = rows.begin(); row != rows.end(); ++row)
	{
		mTOCTree->RemoveRow(*row);
		delete *row;
	}
}

void MePubWindow::SubjectChanged()
{
	ValueChanged(kDCSubjectViewID);
}

void MePubWindow::DateChanged()
{
	ValueChanged(kDCDateViewID);
}
