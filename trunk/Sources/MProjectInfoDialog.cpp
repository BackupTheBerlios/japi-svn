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

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

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
#include "MPkgConfig.h"

using namespace std;
namespace ba = boost::algorithm;

namespace {

enum {
	kNotebookControlID			= 'note',

	kTargetPopupID				= 'targ',

	kTargetNameControlID		= 'name',
	kLinkerOutputControlID		= 'link',
	kProjectTypeControlID		= 'kind',
	kArchitectureControlID		= 'cpu ',
	
	kOutputDirControlID			= 'outd',
	kResourcesControlID			= 'rsrc',
	kResourceDirControlID		= 'rscd',

	kSystemPathsListID			= 'sysp',
	kUserPathsListID			= 'usrp',
	kLibrariesListID			= 'libp',

	kCompilerControlID			= 'comp',
	kDebugInfoControlID			= 'debu',
	kProfileControlID			= 'prof',
	kPICControlID				= 'pic ',
	
	
//	kAnsiStrictControlID		= 3001,
//	kPedanticControlID			= 3002,
//	kDefinesControlID			= 3011,
//	
//	kWarningsControlID			= 4001
};

}

MProjectInfoDialog::MProjectInfoDialog()
	: MDialog("project-info-dialog")
	, eTargetChanged(this, &MProjectInfoDialog::TargetChanged)
	, eDefinesChanged(this, &MProjectInfoDialog::DefinesChanged)
	, eWarningsChanged(this, &MProjectInfoDialog::WarningsChanged)
	, eSysPathsChanged(this, &MProjectInfoDialog::SysPathsChanged)
	, eUserPathsChanged(this, &MProjectInfoDialog::UserPathsChanged)
	, eLibPathsChanged(this, &MProjectInfoDialog::LibPathsChanged)
	, mProject(nil)
{
	eTargetChanged.Connect(GetGladeXML(), "on_targ_changed");

	GtkWidget* wdgt = GetWidget(kSystemPathsListID);
	if (wdgt)
		eSysPathsChanged.Connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(wdgt))), "changed");

	wdgt = GetWidget(kUserPathsListID);
	if (wdgt)
		eUserPathsChanged.Connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(wdgt))), "changed");

	wdgt = GetWidget(kLibrariesListID);
	if (wdgt)
		eLibPathsChanged.Connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(wdgt))), "changed");

	wdgt = GetWidget('defs');
	if (wdgt)
		eDefinesChanged.Connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(wdgt))), "changed");
}

MProjectInfoDialog::~MProjectInfoDialog()
{
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

	SetText(kOutputDirControlID, mProjectInfo.mOutputDir.string());
	SetChecked(kResourcesControlID, mProjectInfo.mAddResources);
	SetText(kResourceDirControlID, mProjectInfo.mResourcesDir.string());

	TargetChanged();
	
	// set up the paths
	vector<string> paths;
	transform(mProjectInfo.mSysSearchPaths.begin(), mProjectInfo.mSysSearchPaths.end(),
		back_inserter(paths), boost::bind(&fs::path::string, _1));
	SetText(kSystemPathsListID, ba::join(paths, "\n"));

	paths.clear();
	transform(mProjectInfo.mUserSearchPaths.begin(), mProjectInfo.mUserSearchPaths.end(),
		back_inserter(paths), boost::bind(&fs::path::string, _1));
	SetText(kUserPathsListID, ba::join(paths, "\n"));

	paths.clear();
	transform(mProjectInfo.mLibSearchPaths.begin(), mProjectInfo.mLibSearchPaths.end(),
		back_inserter(paths), boost::bind(&fs::path::string, _1));
	SetText(kLibrariesListID, ba::join(paths, "\n"));
	
	// pkg-config
	
	vector<string> pkgs, desc;
	GetPkgConfigPackagesList(pkgs, desc);
	
//	copy(pkgs.begin(), pkgs.end(), ostream_iterator<string>(cout, "\n"));
	
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
		default:
			break;
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
		
		default:
			break;
	}
	
	SetChecked(kDebugInfoControlID, target.mBuildFlags & eBF_debug);
	SetChecked(kProfileControlID, target.mBuildFlags & eBF_profile);
	SetChecked(kPICControlID, target.mBuildFlags & eBF_pic);

	SetText(kCompilerControlID, target.mCompiler);
	
	SetText('defs', ba::join(target.mDefines, "\n"));
	SetText('warn', ba::join(target.mWarnings, "\n"));
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
	
	string s;
	
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
		
		case 'defs':
			GetText(inID, s);
			ba::split(target.mDefines, s, ba::is_any_of("\n\r\t "));
			break;

		case 'warn':
			GetText(inID, s);
			ba::split(target.mWarnings, s, ba::is_any_of("\n\r\t "));
			break;
	}	
}

void MProjectInfoDialog::SysPathsChanged()
{
	string s;
	GetText(kSystemPathsListID, s);
	
	vector<string> paths;
	ba::split(paths, s, ba::is_any_of("\n\r"));
	
	mProjectInfo.mSysSearchPaths.clear();
	copy(paths.begin(), paths.end(), back_inserter(mProjectInfo.mSysSearchPaths));
}

void MProjectInfoDialog::UserPathsChanged()
{
	string s;
	GetText(kUserPathsListID, s);
	
	vector<string> paths;
	ba::split(paths, s, ba::is_any_of("\n\r"));
	
	mProjectInfo.mUserSearchPaths.clear();
	copy(paths.begin(), paths.end(), back_inserter(mProjectInfo.mUserSearchPaths));
}

void MProjectInfoDialog::LibPathsChanged()
{
	string s;
	GetText(kLibrariesListID, s);
	
	vector<string> paths;
	ba::split(paths, s, ba::is_any_of("\n\r"));
	
	mProjectInfo.mLibSearchPaths.clear();
	copy(paths.begin(), paths.end(), back_inserter(mProjectInfo.mLibSearchPaths));
}

void MProjectInfoDialog::DefinesChanged()
{
	ValueChanged('defs');
}

void MProjectInfoDialog::WarningsChanged()
{
	ValueChanged('warn');
}
