//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include "MProjectWindow.h"
#include "MProjectItem.h"
#include "MStrings.h"
#include "MError.h"
#include "MFindAndOpenDialog.h"
#include "MProject.h"
#include "MUtils.h"
#include "MPreferences.h"
#include "MAlerts.h"
#include "MJapiApp.h"
#include "MProjectInfoDialog.h"
#include "MLibraryInfoDialog.h"
#include "MControls.h"

//#include "MList.h"

namespace ba = boost::algorithm;

using namespace std;

namespace {

struct MProjectState
{
	uint16			mWindowPosition[2];
	uint16			mWindowSize[2];
	uint8			mSelectedTarget;
	uint8			mSelectedPanel;
	uint8			mFillers[2];
//	int32			mScrollPosition[ePanelCount];
//	uint32			mSelectedFile;
};

const char
	kJapieProjectState[] = "com.hekkelman.japi.ProjectState";

const uint32
	kMProjectStateSize = sizeof(MProjectState);

const string
	kFilesListViewID("files"),
	kLinkOrderViewID("link-order"),
	kResourceViewID("resources"),
	kTargetPopupID("target"),
	kNoteBookID("note-book");

}

////---------------------------------------------------------------------
//// MFileRowItem
//
//namespace MItemColumns
//{
//	struct name {};
//	struct text {};
//	struct data {};
//	struct dirty {};
//}
//
//class MProjectItemRow
//{
//  public:
//					MProjectItemRow(
//						MProjectItem*	inItem)
//						: eStatusChanged(this, &MProjectItemRow::StatusChanged)
//						, mItem(inItem)
//					{
//						AddRoute(mItem->eStatusChanged, eStatusChanged);
//					}
//
//	static const uint32 kDotSize = 6;
//	static const MColor	kOutOfDateColor, kCompilingColor;
//
//	GdkPixbuf*		GetOutOfDateDot()
//					{
//						static GdkPixbuf* kOutOfDateDot = CreateDot(kOutOfDateColor, kDotSize);
//						return kOutOfDateDot;
//					}
//	
//	GdkPixbuf*		GetCompilingDot()
//					{
//						static GdkPixbuf* kCompilingDot = CreateDot(kCompilingColor, kDotSize);
//						return kCompilingDot;
//					}
//
//	void			GetSize(
//						uint32		inSize,
//						string&		outSizeAsString)
//					{
//						stringstream s;
//						if (inSize >= 1024 * 1024 * 1024)
//							s << (inSize / (1024 * 1024 * 1024)) << 'G';
//						else if (inSize >= 1024 * 1024)
//							s << (inSize / (1024 * 1024)) << 'M';
//						else if (inSize >= 1024)
//							s << (inSize / (1024)) << 'K';
//						else if (inSize > 0)
//							s << inSize;
//						outSizeAsString = s.str();
//					}
//
//	MProjectItem*	GetProjectItem()			{ return mItem; }
//
//	void			SetProjectItem(
//						MProjectItem*	inItem)	{ mItem = inItem; }
//
//  protected:
//
//	virtual void	EmitRowChanged() = 0;
//
//	void			StatusChanged(
//						MProjectItem*	inItem)	{ EmitRowChanged(); }
//	MEventIn<void(MProjectItem*)>
//					eStatusChanged;
//
//	MProjectItem*	mItem;
//};
//
//const MColor
//	MProjectItemRow::kOutOfDateColor = MColor("#ff664c"),
//	MProjectItemRow::kCompilingColor = MColor("#ffbb6b");
//
//class MFileRowItem
//	: public MListRow<
//			MFileRowItem,
//			MItemColumns::name,		string,
//			MItemColumns::text,		string,
//			MItemColumns::data,		string,
//			MItemColumns::dirty,	GdkPixbuf*
//		>
//	, public MProjectItemRow
//{
//  public:
//					MFileRowItem(
//						MProjectItem*	inItem)
//						: MProjectItemRow(inItem)
//					{
//					}
//				
//	void			GetData(
//						const MItemColumns::name&,
//						string&			outName)
//					{
//						outName = mItem->GetDisplayName();
//					}
//
//	void			GetData(
//						const MItemColumns::text&,
//						string&			outSize)
//					{
//						GetSize(mItem->GetTextSize(), outSize);
//					}
//
//	void			GetData(
//						const MItemColumns::data&,
//						string&			outSize)
//					{
//						GetSize(mItem->GetDataSize(), outSize);
//					}
//
//	void			GetData(
//						const MItemColumns::dirty&,
//						GdkPixbuf*&		outPixbuf)
//					{
//						if (mItem->IsCompiling())
//							outPixbuf = GetCompilingDot();
//						else if (mItem->IsOutOfDate())
//							outPixbuf = GetOutOfDateDot();
//						else
//							outPixbuf = nil;
//					}
//
//	virtual void	ColumnEdited(
//						uint32			inColumnNr,
//						const string&	inName)
//					{
//						assert(dynamic_cast<MProjectGroup*>(mItem) != nil);
//						if (inName != mItem->GetName() and inColumnNr == 0)
//						{
//							mItem->SetName(inName);
//							EmitRowChanged();
//						}
//					}
//
//	virtual void	EmitRowChanged()			{ RowChanged(); }
//	virtual bool	RowDropPossible() const		{ return dynamic_cast<MProjectGroup*>(mItem) != nil; }
//};
//
//class MRsrcRowItem
//	: public MListRow<
//			MRsrcRowItem,
//			MItemColumns::name,		string,
//			MItemColumns::data,		string,
//			MItemColumns::dirty,	GdkPixbuf*
//		>
//	, public MProjectItemRow
//{
//  public:
//					MRsrcRowItem(
//						MProjectItem*	inItem)
//						: MProjectItemRow(inItem)
//					{
//					}
//				
//	void			GetData(
//						const MItemColumns::name&,
//						string&			outName)
//					{
//						outName = mItem->GetDisplayName();
//					}
//
//	void			SetData(
//						const MItemColumns::name&,
//						const string&	inName)
//					{
//						assert(dynamic_cast<MProjectGroup*>(mItem) != nil);
//						if (inName != mItem->GetName())
//						{
//							mItem->SetName(inName);
//							EmitRowChanged();
//						}
//					}
//
//	void			GetData(
//						const MItemColumns::data&,
//						string&			outSize)
//					{
//						GetSize(mItem->GetDataSize(), outSize);
//					}
//
//	void			GetData(
//						const MItemColumns::dirty&,
//						GdkPixbuf*&		outPixbuf)
//					{
//						if (mItem->IsCompiling())
//							outPixbuf = GetCompilingDot();
//						else if (mItem->IsOutOfDate())
//							outPixbuf = GetOutOfDateDot();
//						else
//							outPixbuf = nil;
//					}
//
//	virtual void	ColumnEdited(
//						uint32			inColumnNr,
//						const string&	inName)
//					{
//						assert(dynamic_cast<MProjectGroup*>(mItem) != nil);
//						if (inName != mItem->GetName() and inColumnNr == 0)
//						{
//							mItem->SetName(inName);
//							EmitRowChanged();
//						}
//					}
//
//	virtual void	EmitRowChanged()			{ RowChanged(); }
//	virtual bool	RowDropPossible() const		{ return dynamic_cast<MProjectGroup*>(mItem) != nil; }
//};

//---------------------------------------------------------------------
// MProjectWindow

MProjectWindow::MProjectWindow()
	: MDocWindow("Untitled", MRect(0, 0, 300, 600),
		MWindowFlags(kMPostionDefault | kMDialogBackground), "project-window-menu")
	, eStatus(this, &MProjectWindow::SetStatus)
	, eSelectFileRow(this, &MProjectWindow::SelectFileRow)
	, eInvokeFileRow(this, &MProjectWindow::InvokeFileRow)
	, eDraggedFileRow(this, &MProjectWindow::DraggedFileRow)
	, eSelectRsrcRow(this, &MProjectWindow::SelectRsrcRow)
	, eInvokeRsrcRow(this, &MProjectWindow::InvokeRsrcRow)
	, eDraggedRsrcRow(this, &MProjectWindow::DraggedRsrcRow)
//	, eKeyPressEvent(this, &MProjectWindow::OnKeyPressEvent)
	, eTargetChanged(this, &MProjectWindow::TargetChanged)
	, eTargetsChanged(this, &MProjectWindow::TargetsChanged)
	, eInfoClicked(this, &MProjectWindow::InfoClicked)
	, eMakeClicked(this, &MProjectWindow::MakeClicked)
//	, eEditedFileGroupName(this, &MProjectWindow::EditedFileGroupName)
//	, eEditedRsrcGroupName(this, &MProjectWindow::EditedRsrcGroupName)
	, mProject(nil)
	, mBusy(false)
	, mEditingName(false)
{
	MRect bounds;

	// status bar
	GetBounds(bounds);
	bounds.y += bounds.height - kScrollbarWidth;
	bounds.height = kScrollbarWidth;

	int32 partWidths[2] = { -1 };
	mStatusbar = new MStatusbar("status", bounds, 1, partWidths);
	AddChild(mStatusbar);
	mStatusbar->GetBounds(bounds);

	int32 statusbarHeight = bounds.height;

	// top part
	GetBounds(bounds);
	
	MRect tr(bounds);
	tr.height = 2 * 7 + 23;
	MView* top = new MView("top", tr);
	top->SetBindings(true, true, true, false);
	AddChild(top);
	
	MRect pr(tr);
	pr.InsetBy(7, 7);
	pr.width -= 2 * 7 + 2 * 40;
	
	mTargetPopup = new MPopup(kTargetPopupID, pr);
	AddRoute(mTargetPopup->eValueChanged, eTargetChanged);
	mTargetPopup->SetBindings(true, true, true, false);
	top->AddChild(mTargetPopup);
	
	pr.x += pr.width + 7;
	pr.width = 40;
	MButton* button = new MButton("make-button", pr, "Make");
	AddRoute(eMakeClicked, button->eClicked);
	button->SetBindings(false, true, true, false);
	top->AddChild(button);

	pr.x += 7 + 40;
	button = new MButton("info-button", pr, "Info");
	AddRoute(eInfoClicked, button->eClicked);
	button->SetBindings(false, true, true, false);
	top->AddChild(button);

	int32 topHeight = 2 * 7 + 23;
	
	// notebook
	
	MRect nr(bounds);
	nr.y += topHeight;
	nr.x -= 1;
	nr.height -= topHeight + statusbarHeight;
	nr.width += 5;
	nr.height += 4;
	
	mNotebook = new MNotebook(kNoteBookID, nr);
	AddChild(mNotebook);
	mNotebook->SetBindings(true, true, true, true);

	mNotebook->GetBounds(nr);
	nr.InsetBy(4, 4);
	nr.y += 25;
	
	MView* page = new MView("page-1", nr);
	page->SetBindings(true, true, true, true);
	mNotebook->AddPage("Files", page);

	MRect r(nr);
	r.x = r.y = 0;
	r.height = 24;
	MListHeader* header = new MListHeader("list-header-1", r);
	page->AddChild(header);
	
	header->AppendColumn("Files", 200);
	header->AppendColumn("Text", 60);
	header->AppendColumn("Data", 60);

	page = new MView("page-2", nr);
	page->SetBindings(true, true, true, true);
//	page->AddChild(new MCaption("capt-2", r, "label 2"));
	mNotebook->AddPage("Link Order", page);

	page = new MView("page-3", nr);
	page->SetBindings(true, true, true, true);
//	page->AddChild(new MCaption("capt-3", r, "label 3"));
	mNotebook->AddPage("Resources", page);
}

// ---------------------------------------------------------------------------
//	MProjectWindow::~MProjectWindow

MProjectWindow::~MProjectWindow()
{
}

// ---------------------------------------------------------------------------
//	MProjectWindow::SetDocument

void MProjectWindow::SetDocument(
	MDocument*		inDocument)
{
	mProject = dynamic_cast<MProject*>(inDocument);
	if (mProject == nil)
		THROW(("Invalid document type passed"));
	
	AddRoute(mProject->eStatus, eStatus);
	AddRoute(mProject->eTargetsChanged, eTargetsChanged);
	
	MDocWindow::SetDocument(inDocument);

//	// Files tree
//	
//	MList<MFileRowItem>* fileTree = new MList<MFileRowItem>(GetWidget(kFilesListViewID));
//	mFileTree = fileTree;
//	mFileTree->AllowMultipleSelectedItems();
//	AddRoute(fileTree->eRowSelected, eSelectFileRow);
//	AddRoute(fileTree->eRowInvoked, eInvokeFileRow);
//	AddRoute(fileTree->eRowDragged, eDraggedFileRow);
////	AddRoute(fileTree->eRowColumnEdited, eEditedFileGroupName);
//	AddRoute(fileTree->eRowsReordered, mProject->eProjectItemMoved);
//	mFileTree->SetColumnTitle(0, _("File"));
//	mFileTree->SetExpandColumn(0);
//	mFileTree->SetColumnTitle(1, _("Tekst"));
//	mFileTree->SetColumnAlignment(1, 1.0f);
//	mFileTree->SetColumnTitle(2, _("Data"));
//	mFileTree->SetColumnAlignment(2, 1.0f);
//	
//	AddItemsToList(mProject->GetFiles(), nil, fileTree);
//	mFileTree->ExpandAll();
//
//	eKeyPressEvent.Connect(G_OBJECT(fileTree->GetGtkWidget()), "key-press-event");
//	
//	// Resources tree
//
//	MList<MRsrcRowItem>* rsrcTree = new MList<MRsrcRowItem>(GetWidget(kResourceViewID));
//	mRsrcTree = rsrcTree;
//	mRsrcTree->AllowMultipleSelectedItems();
//	AddRoute(rsrcTree->eRowSelected, eSelectRsrcRow);
//	AddRoute(rsrcTree->eRowInvoked, eInvokeRsrcRow);
//	AddRoute(rsrcTree->eRowDragged, eDraggedRsrcRow);
////	AddRoute(rsrcTree->eRowColumnEdited, eEditedRsrcGroupName);
//	AddRoute(rsrcTree->eRowsReordered, mProject->eProjectItemMoved);
//	mRsrcTree->SetColumnTitle(0, _("File"));
//	mRsrcTree->SetExpandColumn(0);
//	mRsrcTree->SetColumnTitle(1, _("Size"));
//	mRsrcTree->SetColumnAlignment(1, 1.0f);
//	
//	AddItemsToList(mProject->GetResources(), nil, rsrcTree);
//	mRsrcTree->ExpandAll();
//
//	eKeyPressEvent.Connect(G_OBJECT(rsrcTree->GetGtkWidget()), "key-press-event");

	// read the project's state, if any
	
	bool useState = false;
	MProjectState state = {};
	
	if (Preferences::GetBoolean("save state", true))
	{
		int32 r = inDocument->GetFile().ReadAttribute(kJapieProjectState, &state, kMProjectStateSize);
		useState = static_cast<uint32>(r) == kMProjectStateSize;
	}
	
	if (useState)
	{
		if (state.mWindowSize[0] > 50 and state.mWindowSize[1] > 50 and
			state.mWindowSize[0] < 2000 and state.mWindowSize[1] < 2000)
		{
			MRect r(
				state.mWindowPosition[0], state.mWindowPosition[1],
				state.mWindowSize[0], state.mWindowSize[1]);
		
			SetWindowPosition(r);
		}
		
		mNotebook->SelectPage(state.mSelectedPanel);

////		treeView = MGtkTreeView(GetWidget(kFilesListViewID));
////		treeView.SetScrollPosition(state.mScrollPosition[ePanelFiles]);
////
////		treeView = MGtkTreeView(GetWidget(kResourceViewID));
////		treeView.SetScrollPosition(state.mScrollPosition[ePanelPackage]);

		mProject->SelectTarget(state.mSelectedTarget);
	}
	else
	{
		mProject->SelectTarget(0);
	}

	// initialize interface
	SyncInterfaceWithProject();
}

template<class R>
void MProjectWindow::AddItemsToList(
	MProjectGroup*		inGroup,
	MListRowBase*		inParent,
	MList<R>*			inList)
{
//	for (auto iter = inGroup->GetItems().begin(); iter != inGroup->GetItems().end(); ++iter)
//	{
//		R* item = new R(*iter);
//		
//		inList->AppendRow(item, static_cast<R*>(inParent));
//		
//		MProjectGroup* group = dynamic_cast<MProjectGroup*>(*iter);
//		if (group != nil)
//			AddItemsToList(group, item, inList);
//	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::SelectFileRow

void MProjectWindow::SelectFileRow(
	MFileRowItem*		inFileRow)
{
//	MProjectItem* item = inFileRow->GetProjectItem();
//	MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
//	mFileTree->SetColumnEditable(0, group != nil);
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeFileRow

void MProjectWindow::InvokeFileRow(
	MFileRowItem*		inFileRow)
{
//	MProjectItem* item = inFileRow->GetProjectItem();
//	MProjectFile* file = dynamic_cast<MProjectFile*>(item);
//	if (file != nil)
//	{
//		fs::path p = file->GetPath();
//		gApp->OpenOneDocument(MFile(p));
//	}
//	else
//	{
//		MProjectLib* lib = dynamic_cast<MProjectLib*>(item);
//		if (lib != nil)
//		{
//			unique_ptr<MLibraryInfoDialog> dlog(new MLibraryInfoDialog);
//			dlog->Initialize(mProject, lib);
//			dlog->Show(this);
//			dlog.release();
//		}
//	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::DraggedFileRow

void MProjectWindow::DraggedFileRow(
	MFileRowItem*		inFileRow)
{
//	MProjectItem* item = inFileRow->GetProjectItem();
//	
//	MFileRowItem* parent;
//	uint32 position;
//	if (inFileRow->GetParentAndPosition(parent, position))
//	{
//		MProjectGroup* oldGroup = item->GetParent();
//		MProjectGroup* newGroup;
//		
//		if (parent != nil)
//			newGroup = dynamic_cast<MProjectGroup*>(parent->GetProjectItem());
//		else
//			newGroup = mProject->GetFiles();
//
//		if (oldGroup != nil)
//			oldGroup->RemoveProjectItem(item);
//
//		if (newGroup != nil)
//			newGroup->AddProjectItem(item, position);
//	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::SelectRsrcRow

void MProjectWindow::SelectRsrcRow(
	MRsrcRowItem*		inRsrcRow)
{
//	MProjectItem* item = inRsrcRow->GetProjectItem();
//	MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
//	mRsrcTree->SetColumnEditable(0, group != nil);
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeResourceRow

void MProjectWindow::InvokeRsrcRow(
	MRsrcRowItem*		inResourceRow)
{
//	MProjectItem* item = inResourceRow->GetProjectItem();
//	MProjectFile* file = dynamic_cast<MProjectFile*>(item);
//	if (file != nil)
//	{
//		fs::path p = file->GetPath();
//		
//		bool openSelf = true;
//		
//		if (FileNameMatches("*.glade", p) or FileNameMatches("*.ui", p))
//		{
//			// start up glade
//			string cmd = Preferences::GetString("glade", "glade-3");
//			cmd += " ";
//			cmd += p.string();
//			cmd += "&";
//			
//			int r = system(cmd.c_str());
//			if (r != 0)
//			{
//				DisplayAlert("error-alert",
//					string("Could not execute command:\n") + cmd);
//			}
//			else
//				openSelf = false;
//		}
//
//		if (openSelf)
//			gApp->OpenOneDocument(MFile(p));
//	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::DraggedRsrcRow

void MProjectWindow::DraggedRsrcRow(
	MRsrcRowItem*		inRsrcRow)
{
//	MProjectItem* item = inRsrcRow->GetProjectItem();
//	
//	MRsrcRowItem* parent;
//	uint32 position;
//	if (inRsrcRow->GetParentAndPosition(parent, position))
//	{
//		MProjectGroup* oldGroup = item->GetParent();
//		MProjectGroup* newGroup;
//		
//		if (parent != nil)
//			newGroup = dynamic_cast<MProjectGroup*>(parent->GetProjectItem());
//		else
//			newGroup = mProject->GetResources();
//
//		if (oldGroup != nil)
//			oldGroup->RemoveProjectItem(item);
//
//		if (newGroup != nil)
//			newGroup->AddProjectItem(item, position);
//	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::UpdateCommandStatus

bool MProjectWindow::UpdateCommandStatus(
	uint32				inCommand,
	MMenu*				inMenu,
	uint32				inItemIndex,
	bool&				outEnabled,
	bool&				outChecked)
{
	bool result = true;
	
	switch (inCommand)
	{
		case cmd_Preprocess:
		case cmd_CheckSyntax:
		case cmd_Compile:
		case cmd_Disassemble:
		{
			vector<MProjectFile*> selectedFiles;
			GetSelectedFiles(selectedFiles);
			
			outEnabled = find_if(selectedFiles.begin(), selectedFiles.end(),
				boost::bind(&MProjectFile::IsCompilable, _1) == true) != selectedFiles.end();
			break;
		}

		case cmd_AddFileToProject:
		case cmd_OpenIncludeFile:
			outEnabled = true;
			break;

		case cmd_NewGroup:
			outEnabled = mNotebook->GetSelectedPage() != ePanelLinkOrder;
			break;
		
		default:
			result = MDocWindow::UpdateCommandStatus(
				inCommand, inMenu, inItemIndex, outEnabled, outChecked);
			break;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProject::ProcessCommand

bool MProjectWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;

	vector<MProjectFile*> selectedFiles;
	GetSelectedFiles(selectedFiles);

	switch (inCommand)
	{
		case cmd_Preprocess:
			mProject->Preprocess(selectedFiles);
			break;
			
		case cmd_CheckSyntax:
			mProject->CheckSyntax(selectedFiles);
			break;

		case cmd_Compile:
			mProject->Compile(selectedFiles);
			break;

		case cmd_Disassemble:
			mProject->Disassemble(selectedFiles);
			break;

		case cmd_AddFileToProject:
		{
			if (mNotebook->GetSelectedPage() == ePanelFiles)
				AddFilesToProject();
			else
				AddRsrcsToProject();
			break;
		}
		
		case cmd_NewGroup:
			CreateNewGroup();
			break;
		
		case cmd_OpenIncludeFile:
			new MFindAndOpenDialog(mProject, this);
			break;
		
		default:
			result = MDocWindow::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
			break;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	SetStatus

void MProjectWindow::SetStatus(
	string			inStatus,
	bool			inBusy)
{
//	if (GTK_IS_LABEL(mStatusPanel))
//	{
//		gtk_label_set_text(GTK_LABEL(mStatusPanel), inStatus.c_str());
//		
//		if (mBusy != inBusy)
//		{
//			if (inBusy)
//				gtk_widget_show(mStatusPanel);
//			else
//				gtk_widget_hide(mStatusPanel);
//
//			mBusy = inBusy;
//		}
//	}
}

// ---------------------------------------------------------------------------
//	SyncInterfaceWithProject

void MProjectWindow::SyncInterfaceWithProject()
{
	const vector<MProjectTarget>& targets = mProject->GetTargets();
	
	vector<string> targetNames;
	transform(targets.begin(), targets.end(), back_inserter(targetNames),
		boost::bind(&MProjectTarget::GetName, _1));
	
	mTargetPopup->SetChoices(targetNames);
	mTargetPopup->SetValue(mProject->GetSelectedTarget() + 1);
}

// ---------------------------------------------------------------------------
//	TargetChanged

void MProjectWindow::TargetChanged(
	const string&		inTarget,
	int32				inIndex)
{
	if (mProject != nil)
		mProject->SelectTarget(inIndex - 1);
}

// ---------------------------------------------------------------------------
//	TargetsChanged

void MProjectWindow::TargetsChanged()
{
	SyncInterfaceWithProject();
}

// ---------------------------------------------------------------------------
//	DocumentChanged

void MProjectWindow::DocumentChanged(
	MDocument*		inDocument)
{
	if (inDocument != mProject)
		mProject = dynamic_cast<MProject*>(inDocument);
	
	MDocWindow::DocumentChanged(inDocument);
}

// ---------------------------------------------------------------------------
//	DoClose

bool MProjectWindow::DoClose()
{
	bool result = false;

	if (mBusy == false or
		DisplayAlert(this, "stop-building-alert", mProject->GetName()))
	{
		if (mBusy)
			mProject->StopBuilding();
		
		if (mProject != nil and mProject->IsSpecified())
			SaveState();
		
		result = MDocWindow::DoClose();
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	SaveState

void MProjectWindow::SaveState()
{
	MProjectState state = { };

	mProject->GetFile().ReadAttribute(kJapieProjectState, &state, kMProjectStateSize);
	
	state.mSelectedTarget = mProject->GetSelectedTarget();

	state.mSelectedPanel = mNotebook->GetSelectedPage();

	MRect r;
	GetWindowPosition(r);
	state.mWindowPosition[0] = r.x;
	state.mWindowPosition[1] = r.y;
	state.mWindowSize[0] = r.width;
	state.mWindowSize[1] = r.height;

	mProject->GetFile().WriteAttribute(kJapieProjectState, &state, kMProjectStateSize);
}

// ---------------------------------------------------------------------------
//	CreateNewGroup

void MProjectWindow::CreateNewGroup()
{
//	MListBase* list;
//	MListRowBase* row;
//	MProjectGroup* newGroup = new MProjectGroup(_("New Folder"), nil);
//	
//	if (mNotebook->GetSelectedPage() == ePanelFiles)
//	{
//		list = mFileTree;
//		row = new MFileRowItem(newGroup);
//	}
//	else
//	{
//		list = mRsrcTree;
//		row = new MRsrcRowItem(newGroup);
//	}
//
//	MListRowBase* cursor = list->GetCursorRow();
//	list->InsertRow(row, cursor);
//	
//	if (mNotebook->GetSelectedPage() == ePanelFiles)
//		DraggedFileRow(static_cast<MFileRowItem*>(row));
//	else
//		DraggedRsrcRow(static_cast<MRsrcRowItem*>(row));
//	
//	list->SelectRowAndStartEditingColumn(row, 0);
}

//void MProjectWindow::EditedFileGroupName(
//	MFileRowItem*		inRow,
//	uint32				inColumnNr,
//	const string&		inNewName)
//{
//	inRow->SetFileName(inNewName);
//}
//
//void MProjectWindow::EditedRsrcGroupName(
//	MRsrcRowItem*		inRow,
//	uint32				inColumnNr,
//	const string&		inNewName)
//{
//	inRow->SetFileName(inNewName);
//}

// ---------------------------------------------------------------------------
//	AddFilesToProject

void MProjectWindow::AddFilesToProject()
{
//	vector<MFile> urls;
//	if (ChooseFiles(true, urls))
//	{
//		MList<MFileRowItem>* fileTree = static_cast<MList<MFileRowItem>*>(mFileTree);
//		MFileRowItem* cursor = fileTree->GetCursorRow();
//		MFileRowItem* newCursor = nil;
//		
//		for (auto url = urls.begin(); url != urls.end(); ++url)
//		{
//			if (mProject->GetProjectFileForPath(url->GetPath()))
//				continue;
//			
//			unique_ptr<MProjectItem> item(mProject->CreateFileItem(url->GetPath()));
//			
//			if (not item)
//				continue;
//			
//			MFileRowItem* row = new MFileRowItem(item.get());
//			
//			if (cursor != nil and dynamic_cast<MProjectGroup*>(cursor->GetProjectItem()) != nil)
//			{
//				fileTree->AppendRow(row, cursor);
//				fileTree->ExpandRow(cursor, false);
//			}
//			else
//				fileTree->InsertRow(row, cursor);
//
//			MFileRowItem* parentRow;
//			uint32 position;
//			if (row->GetParentAndPosition(parentRow, position))
//			{
//				MProjectGroup* parent = mProject->GetFiles();
//				if (parentRow != nil)
//					parent = dynamic_cast<MProjectGroup*>(parentRow->GetProjectItem());
//				parent->AddProjectItem(item.get(), position);
//			}
//			
//			item.release();
//
//			if (newCursor == nil)
//				newCursor = row;
//		}
//		
//		if (newCursor)
//			fileTree->SelectRow(newCursor);
//	}
}

void MProjectWindow::AddRsrcsToProject()
{
//	vector<MFile> urls;
//	if (ChooseFiles(true, urls))
//	{
//		MList<MRsrcRowItem>* rsrcTree = static_cast<MList<MRsrcRowItem>*>(mRsrcTree);
//		MRsrcRowItem* cursor = rsrcTree->GetCursorRow();
//		MRsrcRowItem* newCursor = nil;
//		
//		for (auto url = urls.begin(); url != urls.end(); ++url)
//		{
//			if (mProject->GetProjectRsrcForPath(url->GetPath()))
//				continue;
//			
//			unique_ptr<MProjectItem> item(mProject->CreateRsrcItem(url->GetPath()));
//			
//			if (not item)
//				continue;
//			
//			MRsrcRowItem* row = new MRsrcRowItem(item.get());
//			
//			if (cursor != nil and dynamic_cast<MProjectGroup*>(cursor->GetProjectItem()) != nil)
//			{
//				rsrcTree->AppendRow(row, cursor);
//				rsrcTree->ExpandRow(cursor, false);
//			}
//			else
//				rsrcTree->InsertRow(row, cursor);
//			
//			MRsrcRowItem* parentRow;
//			uint32 position;
//			if (row->GetParentAndPosition(parentRow, position))
//			{
//				MProjectGroup* parent = mProject->GetResources();
//				if (parentRow != nil)
//					parent = dynamic_cast<MProjectGroup*>(parentRow->GetProjectItem());
//				parent->AddProjectItem(item.get(), position);
//			}
//			
//			item.release();
//
//			if (newCursor == nil)
//				newCursor = row;
//		}
//		
//		if (newCursor)
//			rsrcTree->SelectRow(newCursor);
//	}
}

// ---------------------------------------------------------------------------
//	GetSelectedItems

void MProjectWindow::GetSelectedItems(
	vector<MProjectItem*>&	outItems)
{
//	if (mNotebook->GetSelectedPage() == ePanelFiles)
//	{
//		auto rows = static_cast<MList<MFileRowItem>*>(mFileTree)->GetSelectedRows();
//		transform(rows.begin(), rows.end(), back_inserter(outItems),
//			boost::bind(&MFileRowItem::GetProjectItem, _1));
//	}
//	else
//	{
//		auto rows = static_cast<MList<MRsrcRowItem>*>(mRsrcTree)->GetSelectedRows();
//		transform(rows.begin(), rows.end(), back_inserter(outItems),
//			boost::bind(&MFileRowItem::GetProjectItem, _1));
//	}
}

// ---------------------------------------------------------------------------
//	GetSelectedFiles

void MProjectWindow::GetSelectedFiles(
	vector<MProjectFile*>&	outFiles)
{
//	vector<MProjectItem*> selectedItems;
//	GetSelectedItems(selectedItems);
//	
//	for (auto item = selectedItems.begin(); item != selectedItems.end(); ++item)
//	{
//		MProjectFile* file = dynamic_cast<MProjectFile*>(*item);
//		if (file != nil)
//		{
//			outFiles.push_back(file);
//			continue;
//		}
//		
//		MProjectGroup* group = dynamic_cast<MProjectGroup*>(*item);
//		if (group != nil)
//		{
//			vector<MProjectItem*> moreItems;
//			group->Flatten(moreItems);
//
//			for (auto item = moreItems.begin(); item != moreItems.end(); ++item)
//			{
//				MProjectFile* file = dynamic_cast<MProjectFile*>(*item);
//				if (file != nil)
//					outFiles.push_back(file);
//			}			
//		}
//	}
//	
//	outFiles.erase(unique(outFiles.begin(), outFiles.end()), outFiles.end());
}

// ---------------------------------------------------------------------------
//	DeleteSelectedItems

void MProjectWindow::DeleteSelectedItems()
{
//	if (mNotebook->GetSelectedPage() == ePanelFiles)
//	{
//		auto rows = static_cast<MList<MFileRowItem>*>(mFileTree)->GetSelectedRows();
//		
//		// first remove the project items from the MProject
//		MProjectGroup* files = mProject->GetFiles();
//		
//		for (auto row = rows.begin(); row != rows.end(); ++row)
//		{
//			MProjectItem* item = (*row)->GetProjectItem();
//			if (files->Contains(item))	// be very paranoid
//			{
//				assert(item->GetParent() != nil);
//
//				item->GetParent()->RemoveProjectItem(item);
//				delete item;
//				
//				(*row)->SetProjectItem(nil);
//			}
//		}
//		
//		for (auto row = rows.begin(); row != rows.end(); ++row)
//		{
//			mFileTree->RemoveRow(*row);
//			delete *row;
//		}
//	}
//	else
//	{
//		auto rows = static_cast<MList<MRsrcRowItem>*>(mRsrcTree)->GetSelectedRows();
//		
//		// first remove the project items from the MProject
//		MProjectGroup* files = mProject->GetResources();
//		
//		for (auto row = rows.begin(); row != rows.end(); ++row)
//		{
//			MProjectItem* item = (*row)->GetProjectItem();
//			if (files->Contains(item))	// be very paranoid
//			{
//				assert(item->GetParent() != nil);
//
//				item->GetParent()->RemoveProjectItem(item);
//				delete item;
//				
//				(*row)->SetProjectItem(nil);
//			}
//		}
//		
//		for (auto row = rows.begin(); row != rows.end(); ++row)
//		{
//			mRsrcTree->RemoveRow(*row);
//			delete *row;
//		}
//	}
}

// ---------------------------------------------------------------------------
//	InfoClicked

void MProjectWindow::InfoClicked(const std::string&)
{
	//unique_ptr<MProjectInfoDialog> dlog(new MProjectInfoDialog);
	//dlog->Initialize(mProject);
	//dlog->Show(this);
	//dlog.release();
}

// ---------------------------------------------------------------------------
//	MakeClicked

void MProjectWindow::MakeClicked(const std::string&)
{
	//mProject->Make();
}

// ---------------------------------------------------------------------------
//	OnKeyPressEvent

//bool MProjectWindow::OnKeyPressEvent(
//	GdkEventKey*	inEvent)
//{
//	bool result = false;
//	
//	uint32 modifiers = inEvent->state & gtk_accelerator_get_default_mod_mask();
//	uint32 keyValue = inEvent->keyval;
//	
//	if (modifiers == 0 and (keyValue == GDK_BackSpace or keyValue == GDK_Delete))
//	{
//		DeleteSelectedItems();
//		result = true;
//	}
//	
//	return result;
//}
