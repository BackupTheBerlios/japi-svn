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

#include "MJapi.h"

#include <sstream>
#include <iterator>
#include <vector>

#include "MFile.h"
#include "MProject.h"
#include "MPreferences.h"
#include "MView.h"
#include "MUnicode.h"
#include "MListView.h"
#include "MProjectTarget.h"
#include "MGlobals.h"
#include "MProjectPathsDialog.h"
#include "MDevice.h"
#include "MUtils.h"

using namespace std;

namespace {

const uint32
	kPageIDs[] = { 0, 1281, 1282, 1283, 1284 },
	kPageCount = sizeof(kPageIDs) / sizeof(uint32) - 1,
	kTabControlID = 128;


enum {
	kPathsBaseID			= 1000,

	kListViewControlID		= 1,
	kAddControlID			= 2,
	kDelControlID			= 3
};

}

MProjectPathsDialog::MProjectPathsDialog()
	: mProject(nil)
	, mTarget(nil)
{
	SetCloseImmediatelyFlag(false);
}

void MProjectPathsDialog::Initialize(
	MProject*				inProject,
	const vector<MPath>&	inUserPaths,
	const vector<MPath>&	inSysPaths,
	const vector<MPath>&	inLibPaths,
	const vector<MPath>&	inFrameworks)
{
//	MView::RegisterSubclass<MListView>();
//
	mProject = inProject;
	
//	MDialog::Initialize(CFSTR("AccessPaths"), inProject);
//
//	::SetAutomaticControlDragTrackingEnabledForWindow(GetSysWindow(), true);
//
//	ControlID id = { kJapieSignature };
//	ControlRef cntrl;
//
//	ControlButtonContentInfo addBtnInfo = { kControlContentCGImageRef };
//	addBtnInfo.u.imageRef = LoadImage("TableViewPlus");
//	ControlButtonContentInfo delBtnInfo = { kControlContentCGImageRef };
//	delBtnInfo.u.imageRef = LoadImage("TableViewMinus");
//
//	for (int page = 1; page <= 4; ++page)
//	{
//		uint32 baseID = kPathsBaseID * page;
//		
//		id.id = baseID + kAddControlID;
//		THROW_IF_OSERROR(::HIViewFindByID(GetContentViewRef(), id, &cntrl));
//		THROW_IF_OSERROR(::SetBevelButtonContentInfo(cntrl, &addBtnInfo));
//
//		id.id = baseID + kDelControlID;
//		THROW_IF_OSERROR(::HIViewFindByID(GetContentViewRef(), id, &cntrl));
//		THROW_IF_OSERROR(::SetBevelButtonContentInfo(cntrl, &delBtnInfo));
//
//		MListView* listView = FindViewByID<MListView>(baseID + kListViewControlID);
//		
//		listView->SetDrawBox(true);
//		
//		SetCallBack(listView->cbDrawItem, this, &MProjectPathsDialog::DrawPath);
//		SetCallBack(listView->cbItemDragged, this, &MProjectPathsDialog::ItemDragged);
//		SetCallBack(listView->cbFilesDropped, this, &MProjectPathsDialog::FilesDropped);
//		
//	//	AddRoute(listView->eRowInvoked, eInvokeProjectItem);
//	//	AddRoute(listView->eRowDeleted, eDeleteProjectItem);
//	
//		vector<MPath>::const_iterator b, e;
//		
//		switch (page)
//		{
//			case 1:	b = inUserPaths.begin();	e = inUserPaths.end();	break;
//			case 2:	b = inSysPaths.begin();		e = inSysPaths.end();	break;
//			case 3:	b = inLibPaths.begin();		e = inLibPaths.end();	break;
//			case 4:	b = inFrameworks.begin();	e = inFrameworks.end();	break;
//		}
//		
//		for (; b != e; ++b)
//			listView->InsertItem(kListItemLast, b->string().c_str(), b->string().length());
//	}
//
//	ControlRef tabControl = FindControl(kTabControlID);
//	
//	::SetControlValue(tabControl, 1);
//	SelectPage(1);

	Show(inProject);
}

void MProjectPathsDialog::ButtonClicked(
	uint32		inButtonID)
{
	switch (inButtonID)
	{
//		case kTabControlID:
//			if (static_cast<uint32>(::GetControlValue(theControl)) != mCurrentPage)
//				SelectPage(::GetControlValue(theControl));
//			break;
//		
//		case (1 * kPathsBaseID) + kDelControlID:
//		case (2 * kPathsBaseID) + kDelControlID:
//		case (3 * kPathsBaseID) + kDelControlID:
//		case (4 * kPathsBaseID) + kDelControlID:
//		{
//			MListView* listView =
//				FindViewByID<MListView>(mCurrentPage * kPathsBaseID + kListViewControlID);
//			
//			int32 selected = listView->GetSelected();
//			if (selected >= 0 and static_cast<uint32>(selected) < listView->GetCount())
//				listView->RemoveItem(selected);
//			
//			break;
//		}
		
		case (1 * kPathsBaseID) + kAddControlID:
		case (2 * kPathsBaseID) + kAddControlID:
		case (3 * kPathsBaseID) + kAddControlID:
		case (4 * kPathsBaseID) + kAddControlID:
			ChooseDirectory();
			break;
	}
}

void MProjectPathsDialog::ChooseDirectory()
{//
//	NavDialogCreationOptions options;
//	::NavGetDefaultDialogCreationOptions(&options);
//	options.optionFlags |= kNavNoTypePopup | kNavAllowInvisibleFiles |
//		kNavSupportPackages | kNavAllowOpenPackages;
//	options.parentWindow = GetParentWindow()->GetSysWindow();
//	options.modality = kWindowModalityWindowModal;
//	
//	static NavEventUPP sNavEvent = ::NewNavEventUPP(&MProjectPathsDialog::NavEvent);
//
//	NavDialogRef navDialog;
//	THROW_IF_OSERROR(::NavCreateChooseFolderDialog(
//		&options, sNavEvent, nil, this, &navDialog));
//
//	::HideSheetWindow(GetSysWindow());
//
//	OSStatus err = ::NavDialogRun(navDialog);
//	
//	if (err != noErr)
//	{
//		::NavDialogDispose(navDialog);
//		THROW_IF_OSERROR(err);
//	}
}

//pascal void	MProjectPathsDialog::NavEvent(
//	NavEventCallbackMessage	inMessage,
//	NavCBRecPtr				inParams,
//	void*					inUserData)
//{
//	MProjectPathsDialog* obj = static_cast<MProjectPathsDialog*>(inUserData);
//							
//	try
//	{
//		if (inMessage == kNavCBUserAction)
//			obj->DoNavUserAction(inParams);
//		else if (inMessage == kNavCBTerminate)
//		{
//			::NavDialogDispose(inParams->context);
//			::ShowSheetWindow(obj->GetSysWindow(), obj->GetParentWindow()->GetSysWindow());
//		}
//	}
//	catch (...) { }
//}
//
//void MProjectPathsDialog::DoNavUserAction(NavCBRecPtr inParams)
//{
//	switch (inParams->userAction)
//	{
//		case kNavUserActionChoose:			// User wants to open a file
//		{
//			NavReplyRecord navReply = {};
//			
//			try
//			{
//				MListView* listView = FindViewByID<MListView>(
//					mCurrentPage * kPathsBaseID + kListViewControlID);
//				
//				THROW_IF_OSERROR(::NavDialogGetReply(inParams->context, &navReply));
//
//				long numDocs;
//				THROW_IF_OSERROR(::AECountItems(&navReply.selection, &numDocs));
//				
//				for (int32 ix = 1; ix <= numDocs; ++ix)
//				{
//					FSRef fileRef;
//					THROW_IF_OSERROR(::AEGetNthPtr(&navReply.selection, 1,
//						typeFSRef, nil, nil, &fileRef, sizeof(FSRef), nil));
//					
//					MPath path;
//					THROW_IF_OSERROR(FSRefMakePath(fileRef, path));
//					
//					listView->InsertItem(kListItemLast, path.string().c_str(),
//						path.string().length());
//				}
//			}
//			catch (std::exception& inErr)
//			{
//				DisplayError(inErr);
//			}
//			catch (...) {}
//			
//			::NavDisposeReply(&navReply);
//			break;
//		}
//			
//		case kNavUserActionCancel:
//			break;
//	}
//}

bool MProjectPathsDialog::OKClicked()
{
//	vector<MPath> userPaths, sysPaths, libPaths, frameworks;
//	
//	for (int page = 1; page <= 4; ++page)
//	{
//		MListView* listView = FindViewByID<MListView>(page * kPathsBaseID + kListViewControlID);
//		
//		vector<MPath>* paths;
//		switch (page)
//		{
//			case 1:	paths = &userPaths; break;
//			case 2:	paths = &sysPaths; break;
//			case 3:	paths = &libPaths; break;
//			case 4:	paths = &frameworks; break;
//		}
//		
//		for (uint32 p = 0; p < listView->GetCount(); ++p)
//		{
//			uint32 size = listView->GetItem(p, nil, 0);
//			char* b = new char[size + 1];
//			listView->GetItem(p, b, size);
//			b[size] = 0;
//			
//			MPath path(b);
//			paths->push_back(path);
//			
//			delete[] b;
//		}
//	}
//	
//	mProject->SetProjectPaths(userPaths, sysPaths, libPaths, frameworks);
	
	return true;
}

void MProjectPathsDialog::SelectPage(
	uint32 		inPage)
{
//	ControlRef select = nil;
//	
//	for (UInt32 page = 1; page <= kPageCount; ++page)
//	{
//		ControlRef pane = FindControl(kPageIDs[page]);
//		
//		if (page == inPage)
//			select = pane;
//		else
//		{
//            ::SetControlVisibility(pane, false, false);
//            ::DisableControl(pane);
//		}
//	}
//	
//    if (select != NULL)
//    {
//        ::EnableControl(select);
//        ::SetControlVisibility(select, true, true);
//    }
//
//	mCurrentPage = inPage;
}

void MProjectPathsDialog::DrawPath(
	MDevice&			inDevice,
	MRect				inFrame,
	uint32				inRow,
	bool				inSelected,
	const void*			inData,
	uint32				inDataLength)
{
	if (inSelected)
		inDevice.SetBackColor(gHiliteColor);
	else if ((inRow % 2) == 0)
		inDevice.SetBackColor(gOddRowColor);
	else
		inDevice.SetBackColor(kWhite);
	
	inDevice.EraseRect(inFrame);
	
	int32 x, y;
	x = inFrame.x + 4;
	y = inFrame.y + 1;

	string s(reinterpret_cast<const char*>(inData), inDataLength);

	inDevice.DrawString(s, x, y, inFrame.width - x);
}

void MProjectPathsDialog::FilesDropped(
	uint32			inTargetRow,
	vector<MPath>	inFiles)
{
//	MListView* listView =
//		FindViewByID<MListView>(mCurrentPage * kPathsBaseID + kListViewControlID);
//	
//	for (vector<MPath>::iterator p = inFiles.begin(); p != inFiles.end(); ++p)
//	{
//		MPath path = *p;
//		if (not is_directory(path))
//			path = path.branch_path();
//		
//		listView->InsertItem(inTargetRow, path.string().c_str(), path.string().length());
//	}
}

void MProjectPathsDialog::ItemDragged(
	uint32				inTargetRow,
	uint32				inNewRow,
	bool				inDropUnder)
{
//	MListView* listView =
//		FindViewByID<MListView>(mCurrentPage * kPathsBaseID + kListViewControlID);
//
//	uint32 size = listView->GetItem(inTargetRow, nil, 0);
//	char* b = new char[size + 1];
//	listView->GetItem(inTargetRow, b, size);
//	b[size] = 0;
//	listView->RemoveItem(inTargetRow);
//	
//	if (inNewRow > inTargetRow)
//		inNewRow -= 1;
//	
//	listView->InsertItem(inNewRow, b, size);
//	
//	delete[] b;	
}
