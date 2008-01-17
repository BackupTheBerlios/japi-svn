/*
	Copyright (c) 2008, Maarten L. Hekkelman
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

#include "MProjectWindow.h"
#include "MProjectItem.h"
#include "MStrings.h"

using namespace std;

namespace {

enum {
	kFilesListViewID	= 'tre1',
	kLinkOrderViewID	= 'tre2',
	kResourceViewID		= 'tre3'
};

enum {
	kFilesDirtyColumn,
	kFilesNameColumn,
	kFilesTextSizeColumn,
	kFilesDataSizeColumn
};
	
}

MProjectWindow::MProjectWindow()
	: MDocWindow("project-window")
	, mInvokeFileRow(this, &MProjectWindow::InvokeFileRow)
{
	mController.SetWindow(this);
	
	mMenubar.Initialize(GetWidget('mbar'), "project-window-menus");

	GtkWidget* treeView = GetWidget(kFilesListViewID);
	THROW_IF_NIL((treeView));

	// create the tree store for the Files tree
	GtkTreeStore* ts = gtk_tree_store_new(
		5, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeView), GTK_TREE_MODEL(ts));
	
	// Add the columns and renderers

	// first the dirty mark	
	GtkCellRenderer* renderer = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
		_(" "), renderer, "pixbuf", kFilesDirtyColumn, nil);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

	// the name column
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("File"), renderer, "text", kFilesNameColumn, nil);
	g_object_set(G_OBJECT(column), "expand", true, nil);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);
	
	// the text size column
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Text"), renderer, "text", kFilesTextSizeColumn, nil);
	g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
	gtk_tree_view_column_set_alignment(column, 1.0f);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

	// the data size column
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Data"), renderer, "text", kFilesDataSizeColumn, nil);
	g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
	gtk_tree_view_column_set_alignment(column, 1.0f);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

	gtk_widget_show_all(treeView);

	mInvokeFileRow.Connect(treeView, "row-activated");
}

// ---------------------------------------------------------------------------
//	MProjectWindow::Initialize

void MProjectWindow::Initialize(
	MDocument*		inDocument)
{
	MDocWindow::Initialize(inDocument);
	
	if (inDocument->IsSpecified())
	{
		
	}
	else
	{
		
	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeFileRow

void MProjectWindow::InvokeFileRow(
	GtkTreePath*		inPath,
	GtkTreeViewColumn*	inColumn)
{
	cout << "Row invoked" << endl;
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
	bool result = true, isCompilable = false;
	
	outEnabled = false;
	
//	switch (mPanel)
//	{
//		case ePanelFiles:
//		{
//			int32 selected = mFileList->GetSelected();
//			
//			if (selected >= 0)
//				isCompilable = GetItem(selected)->IsCompilable();
//			break;
//		}
//		
////		case ePanelLinkOrder:
////		{
////			int32 selected = mLinkOrderList->GetSelected();
////			
////			if (selected >= 0)
////				isCompilable = GetItem(selected)->IsCompilable();
////			break;
////		}
//		
//		default:
//			break;
//	}
	
	switch (inCommand)
	{
//		case cmd_Save:
//			outEnabled = mModified;
//			break;
//		
//		case cmd_AddFileToProject:
//			break;
//
//		case cmd_Preprocess:
//		case cmd_CheckSyntax:
//		case cmd_Compile:
//		case cmd_Disassemble:
//			outEnabled = isCompilable;
//			break;
//
//		case cmd_RecheckFiles:
//		case cmd_BringUpToDate:
//		case cmd_MakeClean:
//		case cmd_Make:
//		case cmd_NewGroup:
//			outEnabled = true;
//			break;
//
//		case cmd_Run:
//			break;
//		
//		case cmd_Stop:
//			outEnabled = mCurrentJob.get() != nil;
//			break;
		
		default:
			result = MWindow::UpdateCommandStatus(
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
	uint32			inItemIndex)
{
	bool result = true;

	MProjectItem* item = nil;

//	switch (mPanel)
//	{
//		case ePanelFiles:
//		{
//			int32 selected = mFileList->GetSelected();
//			
//			if (selected >= 0)
//				item = GetItem(selected);
//			break;
//		}
//		
////		case ePanelLinkOrder:
////		{
////			int32 selected = mLinkOrderList->GetSelected();
////			
////			if (selected >= 0)
////				item = GetItem(selected);
////			break;
////		}
//		
//		default:
//			break;
//	}
	
	MProjectFile* file = dynamic_cast<MProjectFile*>(item);
	
	switch (inCommand)
	{
//		case cmd_Save:
//			SaveDocument();
//			break;
//
//		case cmd_SaveAs:
//			SaveDocumentAs(this, mProjectFile.leaf());
//			break;
//
//		case cmd_Revert:
//			TryDiscardChanges(mName, this);
//			break;
//		
//		case cmd_OpenIncludeFile:
//			new MFindAndOpenDialog(nil, this);
//			break;
//		
//		case cmd_RecheckFiles:
//			CheckIsOutOfDate();
//			break;
//		
//		case cmd_BringUpToDate:
//			BringUpToDate();
//			break;
//		
//		case cmd_Preprocess:
//			if (file != nil)
//				Preprocess(file->GetPath());
//			break;
//		
//		case cmd_CheckSyntax:
//			if (file != nil)
//				CheckSyntax(file->GetPath());
//			break;
//		
//		case cmd_Compile:
//			if (file != nil)
//				Compile(file->GetPath());
//			break;
//
//		case cmd_Disassemble:
//			if (file != nil)
//				Disassemble(file->GetPath());
//			break;
//		
//		case cmd_MakeClean:
//			MakeClean();
//			break;
//		
//		case cmd_Make:
//			Make();
//			break;
//		
//		case cmd_Stop:
//			StopBuilding();
//			break;
		
//		case cmd_NewGroup:
//		{
//			auto_ptr<MNewGroupDialog> dlog(new MNewGroupDialog);
//			dlog->Initialize(this);
//			dlog.release();
//			break;
//		}
//		
//		case cmd_EditProjectInfo:
//		{
//			auto_ptr<MProjectInfoDialog> dlog(new MProjectInfoDialog);
//			dlog->Initialize(this, mCurrentTarget);
//			dlog.release();
//			break;
//		}
		
//		case cmd_EditProjectPaths:
//		{
//			auto_ptr<MProjectPathsDialog> dlog(new MProjectPathsDialog);
//			dlog->Initialize(this, mUserSearchPaths, mSysSearchPaths,
//				mLibSearchPaths, mFrameworkPaths);
//			dlog.release();
//			break;
//		}
		
//		case cmd_ChangePanel:
//			switch (::GetControl32BitValue(mPanelSegmentRef))
//			{
//				case 1:	SelectPanel(ePanelFiles); break;
//				case 2: SelectPanel(ePanelLinkOrder); break;
//				case 3: SelectPanel(ePanelPackage); break;
//			}
//			break;
		
		default:
//			if ((inCommand & 0xFFFFFF00) == cmd_SwitchTarget)
//			{
//				uint32 target = inCommand & 0x000000FF;
//				SelectTarget(target);
////				Invalidate();
//				mFileList->Invalidate();
////				mLinkOrderList->Invalidate();
////				mPackageList->Invalidate();
//			}

			result = MDocWindow::ProcessCommand(inCommand, inMenu, inItemIndex);
			break;
	}
	
	return result;
}

