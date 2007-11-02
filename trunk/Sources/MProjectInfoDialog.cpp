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
#include "MProjectInfoDialog.h"
#include "MDevice.h"

using namespace std;

namespace {

const uint32
	kPageIDs[] = { 0, 1291, 1293, 1294 },
	kPageCount = sizeof(kPageIDs) / sizeof(uint32) - 1,
	kTabControlID = 129;


enum {
	kProjectTypeControlID		= 1001,
	kBundleNameLabelID			= 10021,
	kBundleNameControlID		= 1002,
	kCreatorTypeLabelID			= 10031,
	kCreatorControlID			= 1003,
	kTypeControlID				= 1004,
	kPICControlID				= 1005,
	kFlatNamespaceControlID		= 1006,
	kTargetNameControlID		= 1007,
	kDisablePreBindingControlID	= 1008,
	kLinkerOutputControlID		= 1009,
	kArchitectureControlID		= 1010,
	kDebugInfoControlID			= 1011,
	
	kUserPathControlID			= 2001,
	
	kAnsiStrictControlID		= 3001,
	kPedanticControlID			= 3002,
	kDefinesControlID			= 3011,
	
	kWarningsControlID			= 4001
};

enum {
//	kTextBoxControlID = 2
};

}

MProjectInfoDialog::MProjectInfoDialog()
	: mProject(nil)
	, mTarget(nil)
{
	SetCloseImmediatelyFlag(false);
}

void MProjectInfoDialog::Initialize(
	MProject*		inProject,
	MProjectTarget*	inTarget)
{
//	MView::RegisterSubclass<MListView>();

	mProject = inProject;
	mTarget = inTarget;
	
//	MDialog::Initialize(CFSTR("ProjectInfo"), inProject);

//	ControlRef tabControl = FindControl(kTabControlID);
//	
	SetValue(kTabControlID, 1);
	SelectPage(1);

	// set up the first page
	
	stringstream s;
	string txt;
	
	int32 projectType;
	switch (mTarget->GetKind())
	{
		case eTargetApplicationPackage:	projectType = 1; break;
		case eTargetBundlePackage:		projectType = 2; break;
		case eTargetExecutable:			projectType = 4; break;
		case eTargetSharedLibrary:		projectType = 5; break;
		case eTargetBundle:				projectType = 6; break;
		case eTargetStaticLibrary:		projectType = 7; break;
		default:										 break;
	}
	
	SetValue(kProjectTypeControlID, projectType);
	SelectProjectType(projectType);
	
	SetText(kTargetNameControlID, mTarget->GetName());
//		kProductNameLabelID,

//	SetChecked(kPICControlID, mTarget->GetPIC());
//	SetChecked(kFlatNamespaceControlID, mTarget->GetFlatNamespace());
//	SetChecked(kDisablePreBindingControlID, mTarget->GetDisablePreBinding());
	SetChecked(kDebugInfoControlID, mTarget->GetDebugFlag());
	SetText(kBundleNameControlID, mTarget->GetBundleName());
	SetText(kLinkerOutputControlID, mTarget->GetLinkTarget());
//	kTargetNameLabelID,
//	kSeparatorID,

	SetText(kCreatorControlID, mTarget->GetCreator());
	SetText(kTypeControlID, mTarget->GetType());

	if (mTarget->GetArch() == eTargetArchPPC_32)
	{
		SetValue(kArchitectureControlID, 1);
	}
	else if (mTarget->GetArch() == eTargetArchx86_32)
	{
		SetValue(kArchitectureControlID, 2);
	}
	
//	kCPUControlID

	const vector<string>& defs = mTarget->GetDefines();
	copy(defs.begin(), defs.end(), ostream_iterator<string>(s, " "));
	SetText(kDefinesControlID, s.str());
	
	
	// page two
	
	SetChecked(kPedanticControlID, mTarget->IsPedantic());
	SetChecked(kAnsiStrictControlID, mTarget->IsAnsiStrict());

	// page three

	stringstream w;
	const vector<string>& warnings = mTarget->GetWarnings();
	copy(warnings.begin(), warnings.end(), ostream_iterator<string>(w, " "));
	SetText(kWarningsControlID, w.str());

	Show(inProject);
}

void MProjectInfoDialog::SelectProjectType(
	int32			inType)
{
	string name;
	
	switch (inType)
	{
		case 1:
			GetText(kBundleNameControlID, name);
			name = fs::basename(name);
			SetText(kBundleNameControlID, name + ".app");
			SetText(kLinkerOutputControlID, name);
			SetVisible(kBundleNameControlID, true);
			SetVisible(kBundleNameLabelID, true);
			SetVisible(kCreatorTypeLabelID, true);
			SetVisible(kCreatorControlID, true);
			SetVisible(kTypeControlID, true);
			break;

		case 2:
			GetText(kBundleNameControlID, name);
			name = fs::basename(name);
			SetText(kBundleNameControlID, name + ".bundle");
			SetText(kLinkerOutputControlID, name);
			SetVisible(kBundleNameControlID, true);
			SetVisible(kBundleNameLabelID, true);
			SetVisible(kCreatorTypeLabelID, true);
			SetVisible(kCreatorControlID, true);
			SetVisible(kTypeControlID, true);
			break;

		case 4:
			GetText(kLinkerOutputControlID, name);
			name = fs::basename(name);
			SetText(kLinkerOutputControlID, name);
			SetVisible(kBundleNameControlID, false);
			SetVisible(kBundleNameLabelID, false);
			SetVisible(kCreatorTypeLabelID, false);
			SetVisible(kCreatorControlID, false);
			SetVisible(kTypeControlID, false);
			break;
		
		case 5:
			GetText(kLinkerOutputControlID, name);
			name = fs::basename(name);
			SetText(kLinkerOutputControlID, name + ".dylib");
			SetVisible(kBundleNameControlID, false);
			SetVisible(kBundleNameLabelID, false);
			SetVisible(kCreatorTypeLabelID, false);
			SetVisible(kCreatorControlID, false);
			SetVisible(kTypeControlID, false);
			break;
		
		case 6:
			GetText(kLinkerOutputControlID, name);
			name = fs::basename(name);
			SetText(kLinkerOutputControlID, name + ".bundle");
			SetVisible(kBundleNameControlID, false);
			SetVisible(kBundleNameLabelID, false);
			SetVisible(kCreatorTypeLabelID, false);
			SetVisible(kCreatorControlID, false);
			SetVisible(kTypeControlID, false);
			break;
		
		case 7:
			GetText(kLinkerOutputControlID, name);
			name = fs::basename(name);
			SetText(kLinkerOutputControlID, name + ".a");
			SetVisible(kBundleNameControlID, false);
			SetVisible(kBundleNameLabelID, false);
			SetVisible(kCreatorTypeLabelID, false);
			SetVisible(kCreatorControlID, false);
			SetVisible(kTypeControlID, false);
			break;
		
	}
}

void MProjectInfoDialog::ButtonClicked(
	uint32		inButtonID)
{
	switch (inButtonID)
	{
		case kProjectTypeControlID:
			SelectProjectType(GetValue(kProjectTypeControlID));
			break;
		
		case kTabControlID:
			if (GetValue(kTabControlID) != mCurrentPage)
				SelectPage(GetValue(kTabControlID));
			break;
	}
}

bool MProjectInfoDialog::OKClicked()
{
	string s;

	// page 1

	switch (GetValue(kProjectTypeControlID))
	{
		case 1: mTarget->SetKind(eTargetApplicationPackage);	break;
		case 2: mTarget->SetKind(eTargetBundlePackage);			break;
		case 4: mTarget->SetKind(eTargetExecutable);			break;
		case 5: mTarget->SetKind(eTargetSharedLibrary);			break;
		case 6: mTarget->SetKind(eTargetBundle);				break;
		case 7: mTarget->SetKind(eTargetStaticLibrary);			break;
	}

	GetText(kTargetNameControlID, s);
	mTarget->SetName(s);

	GetText(kBundleNameControlID, s);
	mTarget->SetBundleName(s);
	
	GetText(kLinkerOutputControlID, s);
	mTarget->SetLinkTarget(s);

	switch (GetValue(kArchitectureControlID))
	{
		case 1:	mTarget->SetArch(eTargetArchPPC_32); break;
		case 2:	mTarget->SetArch(eTargetArchx86_32); break;
	}
	
	GetText(kCreatorControlID, s);
	while (s.length() < 4)
		s += ' ';
	mTarget->SetCreator(s.substr(0, 4));

	GetText(kTypeControlID, s);
	while (s.length() < 4)
		s += ' ';
	mTarget->SetType(s.substr(0, 4));
	
	// page 2
	
	// page 3
	
	mTarget->SetPedantic(IsChecked(kPedanticControlID));
	mTarget->SetAnsiStrict(IsChecked(kAnsiStrictControlID));
	mTarget->SetDebugFlag(IsChecked(kDebugInfoControlID));

	// notify project of changes
	
	mProject->TargetUpdated();
	
	return true;
}

void MProjectInfoDialog::SelectPage(
	int32 		inPage)
{
	for (int32 page = 1; page <= kPageCount; ++page)
	{
		SetEnabled(kPageIDs[page], page == inPage);
		SetVisible(kPageIDs[page], page == inPage);
	}

	mCurrentPage = inPage;
}
