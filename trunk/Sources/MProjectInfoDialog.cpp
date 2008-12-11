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
#include "MGlobals.h"
#include "MProjectInfoDialog.h"
#include "MDevice.h"
#include "MGtkWrappers.h"
#include "MError.h"

using namespace std;

namespace {

enum {
	kNotebookControlID			= 'note',

	kTargetPopupID				= 'targ',

	kTargetNameControlID		= 'name',
	kLinkerOutputControlID		= 'link',
	kProjectTypeControlID		= 'kind',
	kArchitectureControlID		= 'cpu ',
	kDebugInfoControlID			= 'debu',
	kProfileControlID			= 'prof',
	kPICControlID				= 'pic ',

	kSystemPathsListID			= 'sysp',
	kUserPathsListID			= 'usrp',
	kLibrariesListID			= 'libp',

	kAnsiStrictControlID		= 3001,
	kPedanticControlID			= 3002,
	kDefinesControlID			= 3011,
	
	kWarningsControlID			= 4001
};

}

MProjectInfoDialog::MProjectInfoDialog()
	: MDialog("project-info-dialog")
	, eTargetChanged(this, &MProjectInfoDialog::TargetChanged)
	, mProject(nil)
{
	eTargetChanged.Connect(GetGladeXML(), "on_targ_changed");
}

MProjectInfoDialog::~MProjectInfoDialog()
{
	g_object_unref(G_OBJECT(mSysPaths));
	g_object_unref(G_OBJECT(mUsrPaths));
	g_object_unref(G_OBJECT(mLibPaths));
}

void MProjectInfoDialog::Initialize(
	MProject*		inProject)
{
//	MView::RegisterSubclass<MListView>();

	mProject = inProject;
	mProject->GetInfo(mProjectInfo);
	
	// setup the targets
	MGtkComboBox targetPopup(GetWidget(kTargetPopupID));
	eTargetChanged.Block(targetPopup, "on_targ_changed");
	
	targetPopup.RemoveAll();
	
	for (vector<MProjectTarget>::iterator t = mProjectInfo.mTargets.begin(); t != mProjectInfo.mTargets.end(); ++t)
		targetPopup.Append(t->mName);
	
	targetPopup.SetActive(mProject->GetSelectedTarget());

	eTargetChanged.Unblock(targetPopup, "on_targ_changed");

	TargetChanged();
	
	// page 1
	
	GtkWidget* list = GetWidget(kSystemPathsListID);
	if (list == nil)
		THROW(("Missing list"));
	
	mSysPaths = gtk_tree_store_new(1, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(mSysPaths));

	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
		"", renderer, "text", 0, nil);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	
	for (vector<fs::path>::iterator path = mProjectInfo.mSysSearchPaths.begin(); path != mProjectInfo.mSysSearchPaths.end(); ++path)
	{
		GtkTreeIter iter;
		gtk_tree_store_append(mSysPaths, &iter, nil);
		gtk_tree_store_set(mSysPaths, &iter, 0, path->string().c_str(), -1);
	}

	list = GetWidget(kUserPathsListID);
	if (list == nil)
		THROW(("Missing list"));
	
	mUsrPaths = gtk_tree_store_new(1, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(mUsrPaths));

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer, "text", 0, nil);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	
	for (vector<fs::path>::iterator path = mProjectInfo.mUserSearchPaths.begin(); path != mProjectInfo.mUserSearchPaths.end(); ++path)
	{
		GtkTreeIter iter;
		gtk_tree_store_append(mUsrPaths, &iter, nil);
		gtk_tree_store_set(mUsrPaths, &iter, 0, path->string().c_str(), -1);
	}

	list = GetWidget(kLibrariesListID);
	if (list == nil)
		THROW(("Missing list"));
	
	mLibPaths = gtk_tree_store_new(1, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(mLibPaths));

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer, "text", 0, nil);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	
	for (vector<fs::path>::iterator path = mProjectInfo.mLibSearchPaths.begin(); path != mProjectInfo.mLibSearchPaths.end(); ++path)
	{
		GtkTreeIter iter;
		gtk_tree_store_append(mLibPaths, &iter, nil);
		gtk_tree_store_set(mLibPaths, &iter, 0, path->string().c_str(), -1);
	}

	
//	
////	MDialog::Initialize(CFSTR("ProjectInfo"), inProject);
//
////	ControlRef tabControl = FindControl(kTabControlID);
////	
//	SetValue(kTabControlID, 1);
//	SelectPage(1);
//
//	// set up the first page
//	
//	stringstream s;
//	string txt;
//	
//	int32 projectType = 1;
//	switch (mTarget->GetKind())
//	{
//		case eTargetApplicationPackage:	projectType = 1; break;
//		case eTargetBundlePackage:		projectType = 2; break;
//		case eTargetExecutable:			projectType = 4; break;
//		case eTargetSharedLibrary:		projectType = 5; break;
//		case eTargetBundle:				projectType = 6; break;
//		case eTargetStaticLibrary:		projectType = 7; break;
//		default:										 break;
//	}
//	
//	SetValue(kProjectTypeControlID, projectType);
//	SelectProjectType(projectType);
//	
//	SetText(kTargetNameControlID, mTarget->GetName());
////		kProductNameLabelID,
//
////	SetChecked(kPICControlID, mTarget->GetPIC());
////	SetChecked(kFlatNamespaceControlID, mTarget->GetFlatNamespace());
////	SetChecked(kDisablePreBindingControlID, mTarget->GetDisablePreBinding());
//	SetChecked(kDebugInfoControlID, mTarget->GetDebugFlag());
//	SetText(kBundleNameControlID, mTarget->GetBundleName());
//	SetText(kLinkerOutputControlID, mTarget->GetLinkTarget());
////	kTargetNameLabelID,
////	kSeparatorID,
//
//	SetText(kCreatorControlID, mTarget->GetCreator());
//	SetText(kTypeControlID, mTarget->GetType());
//
////	if (mTarget->GetTargetCPU() == eTargetArchPPC_32)
////	{
////		SetValue(kArchitectureControlID, 1);
////	}
////	else if (mTarget->GetTargetCPU() == eTargetArchx86_32)
////	{
////		SetValue(kArchitectureControlID, 2);
////	}
//	
////	kCPUControlID
//
//	const vector<string>& defs = mTarget->GetDefines();
//	copy(defs.begin(), defs.end(), ostream_iterator<string>(s, " "));
//	SetText(kDefinesControlID, s.str());
//	
//	
//	// page two
//	
//	SetChecked(kPedanticControlID, mTarget->IsPedantic());
//	SetChecked(kAnsiStrictControlID, mTarget->IsAnsiStrict());
//
//	// page three
//
//	stringstream w;
//	const vector<string>& warnings = mTarget->GetWarnings();
//	copy(warnings.begin(), warnings.end(), ostream_iterator<string>(w, " "));
//	SetText(kWarningsControlID, w.str());
//
//	Show(inProject);
}

// ---------------------------------------------------------------------------
//	TargetChanged

void MProjectInfoDialog::TargetChanged()
{
	MGtkComboBox targetPopup(GetWidget(kTargetPopupID));
	const MProjectTarget& target = mProjectInfo.mTargets[targetPopup.GetActive()];
	
	// name
	SetText(kTargetNameControlID, target.mName);
	SetText(kLinkerOutputControlID, target.mLinkTarget);
	
	switch (target.mTargetCPU)
	{
		case eCPU_native:		SetValue(kArchitectureControlID, 1); break;
		case eCPU_386:			SetValue(kArchitectureControlID, 2); break;
		case eCPU_x86_64:		SetValue(kArchitectureControlID, 3); break;
		case eCPU_PowerPC_32:	SetValue(kArchitectureControlID, 4); break;
		case eCPU_PowerPC_64:	SetValue(kArchitectureControlID, 5); break;
	}
	
	switch (target.mKind)
	{
		case eTargetExecutable:
			SetValue(kProjectTypeControlID, 1);
			break;

		case eTargetSharedLibrary:
			SetValue(kProjectTypeControlID, 2);
			break;
		
		case eTargetStaticLibrary:
			SetValue(kProjectTypeControlID, 3);
			break;
	}
	
	SetChecked(kDebugInfoControlID, target.mBuildFlags & eBF_debug);
	SetChecked(kProfileControlID, target.mBuildFlags & eBF_profile);
	SetChecked(kPICControlID, target.mBuildFlags & eBF_pic);
}

//void MProjectInfoDialog::SelectProjectType(
//	int32			inType)
//{//
////	string name;
////	
////	switch (inType)
////	{
////		case 1:
////			GetText(kBundleNameControlID, name);
////			name = fs::basename(name);
////			SetText(kBundleNameControlID, name + ".app");
////			SetText(kLinkerOutputControlID, name);
////			SetVisible(kBundleNameControlID, true);
////			SetVisible(kBundleNameLabelID, true);
////			SetVisible(kCreatorTypeLabelID, true);
////			SetVisible(kCreatorControlID, true);
////			SetVisible(kTypeControlID, true);
////			break;
////
////		case 2:
////			GetText(kBundleNameControlID, name);
////			name = fs::basename(name);
////			SetText(kBundleNameControlID, name + ".bundle");
////			SetText(kLinkerOutputControlID, name);
////			SetVisible(kBundleNameControlID, true);
////			SetVisible(kBundleNameLabelID, true);
////			SetVisible(kCreatorTypeLabelID, true);
////			SetVisible(kCreatorControlID, true);
////			SetVisible(kTypeControlID, true);
////			break;
////
////		case 4:
////			GetText(kLinkerOutputControlID, name);
////			name = fs::basename(name);
////			SetText(kLinkerOutputControlID, name);
////			SetVisible(kBundleNameControlID, false);
////			SetVisible(kBundleNameLabelID, false);
////			SetVisible(kCreatorTypeLabelID, false);
////			SetVisible(kCreatorControlID, false);
////			SetVisible(kTypeControlID, false);
////			break;
////		
////		case 5:
////			GetText(kLinkerOutputControlID, name);
////			name = fs::basename(name);
////			SetText(kLinkerOutputControlID, name + ".dylib");
////			SetVisible(kBundleNameControlID, false);
////			SetVisible(kBundleNameLabelID, false);
////			SetVisible(kCreatorTypeLabelID, false);
////			SetVisible(kCreatorControlID, false);
////			SetVisible(kTypeControlID, false);
////			break;
////		
////		case 6:
////			GetText(kLinkerOutputControlID, name);
////			name = fs::basename(name);
////			SetText(kLinkerOutputControlID, name + ".bundle");
////			SetVisible(kBundleNameControlID, false);
////			SetVisible(kBundleNameLabelID, false);
////			SetVisible(kCreatorTypeLabelID, false);
////			SetVisible(kCreatorControlID, false);
////			SetVisible(kTypeControlID, false);
////			break;
////		
////		case 7:
////			GetText(kLinkerOutputControlID, name);
////			name = fs::basename(name);
////			SetText(kLinkerOutputControlID, name + ".a");
////			SetVisible(kBundleNameControlID, false);
////			SetVisible(kBundleNameLabelID, false);
////			SetVisible(kCreatorTypeLabelID, false);
////			SetVisible(kCreatorControlID, false);
////			SetVisible(kTypeControlID, false);
////			break;
////		
////	}
//}
//
void MProjectInfoDialog::ButtonClicked(
	uint32		inButtonID)
{//
//	switch (inButtonID)
//	{
//		case kProjectTypeControlID:
//			SelectProjectType(GetValue(kProjectTypeControlID));
//			break;
//		
//		case kTabControlID:
//			if (GetValue(kTabControlID) != mCurrentPage)
//				SelectPage(GetValue(kTabControlID));
//			break;
//	}
}

bool MProjectInfoDialog::OKClicked()
{
	mProject->SetInfo(mProjectInfo);
	return true;
}

void MProjectInfoDialog::ValueChanged(
	uint32			inID)
{
	MGtkComboBox targetPopup(GetWidget(kTargetPopupID));
	MProjectTarget& target = mProjectInfo.mTargets[targetPopup.GetActive()];
	
	switch (inID)
	{
		case kTargetNameControlID:		GetText(inID, target.mName); break;
		case kLinkerOutputControlID:	GetText(inID, target.mLinkTarget); break;
		case kProjectTypeControlID:
			switch (GetValue(inID))
			{
				case 1:	target.mKind = eTargetExecutable; break;
				case 2:	target.mKind = eTargetSharedLibrary; break;
				case 3:	target.mKind = eTargetStaticLibrary; break;
			}
			break;
		
		case kArchitectureControlID:
			switch (GetValue(inID))
			{
				case 1:	target.mTargetCPU = eCPU_native; break;
				case 2:	target.mTargetCPU = eCPU_386; break;
				case 3:	target.mTargetCPU = eCPU_x86_64; break;
				case 4:	target.mTargetCPU = eCPU_PowerPC_32; break;
				case 5:	target.mTargetCPU = eCPU_PowerPC_64; break;
			}
			break;
		
		case kDebugInfoControlID:
			if (IsChecked(inID))
				target.mBuildFlags |= eBF_debug;
			else
				target.mBuildFlags &= ~(eBF_debug);
			break;
		
		case kProfileControlID:
			if (IsChecked(inID))
				target.mBuildFlags |= eBF_profile;
			else
				target.mBuildFlags &= ~(eBF_profile);
			break;
		
		case kPICControlID:
			if (IsChecked(inID))
				target.mBuildFlags |= eBF_pic;
			else
				target.mBuildFlags &= ~(eBF_pic);
			break;
	}	
}
