//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <sstream>
#include <iterator>
#include <vector>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>

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
#include "MStrings.h"

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
	kCxx0xControlID				= 'c+0x',
	
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
	, ePkgToggled(this, &MProjectInfoDialog::PkgToggled)
	, mProject(nil)
{
	eTargetChanged.Connect(this, "on_targ_changed");

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

	wdgt = GetWidget('warn');
	if (wdgt)
		eWarningsChanged.Connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(wdgt))), "changed");
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
	UpdateTargetPopup(mProject->GetSelectedTarget());

	TargetChanged();

	SetText(kOutputDirControlID, mProjectInfo.mOutputDir.string());
	SetChecked(kResourcesControlID, mProjectInfo.mAddResources);
	SetText(kResourceDirControlID, mProjectInfo.mResourcesDir.string());
	
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
	
	vector<pair<string,string> > pkgs;
	GetPkgConfigPackagesList(pkgs);
	
	GtkWidget* list = GetWidget('pkgc');
	if (list == nil)
		THROW(("Missing list"));
	
	mPkgList = gtk_tree_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(mPkgList));

	GtkCellRenderer* renderer = gtk_cell_renderer_toggle_new();
	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
		"", renderer, "active", 0, nil);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	ePkgToggled.Connect(G_OBJECT(renderer), "toggled");

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		"", renderer, "text", 1, nil);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		"", renderer, "text", 2, nil);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	// fill the list, starting with the perl option
	bool isChecked = find(
		mProjectInfo.mPkgConfigPkgs.begin(), mProjectInfo.mPkgConfigPkgs.end(),
		"perl:default") != mProjectInfo.mPkgConfigPkgs.end();
	
	GtkTreeIter iter;
	gtk_tree_store_append(mPkgList, &iter, nil);
	gtk_tree_store_set(mPkgList, &iter,
		0, isChecked, 1, "perl", 2, "Create code that uses an embedded Perl interpreter", -1);
	
	for (vector<pair<string,string> >::iterator pkg = pkgs.begin(); pkg != pkgs.end(); ++pkg)
	{
		string test = "pkg-config:";
		test += pkg->first;
		
		isChecked = find(
			mProjectInfo.mPkgConfigPkgs.begin(), mProjectInfo.mPkgConfigPkgs.end(),
			test) != mProjectInfo.mPkgConfigPkgs.end();
		
		gtk_tree_store_append(mPkgList, &iter, nil);
		gtk_tree_store_set(mPkgList, &iter,
			0, isChecked, 1, pkg->first.c_str(), 2, pkg->second.c_str(), -1);
	}
	
	SetEnabled('delt', mProjectInfo.mTargets.size() > 1);
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
	
	// find out the optimiser level
	int optimiser = 1;
	const vector<string>& cflags = target.mCFlags;
	if (find(cflags.begin(), cflags.end(), "-Os") != cflags.end())
		optimiser = 2;
	else if (find(cflags.begin(), cflags.end(), "-O1") != cflags.end())
		optimiser = 3;
	else if (find(cflags.begin(), cflags.end(), "-O2") != cflags.end())
		optimiser = 4;
	else if (find(cflags.begin(), cflags.end(), "-O3") != cflags.end())
		optimiser = 5;
	SetValue('opti', optimiser);
	
	// pthread
	SetChecked('thre', find(cflags.begin(), cflags.end(), "-pthread") != cflags.end());
	
	SetChecked(kDebugInfoControlID, find(cflags.begin(), cflags.end(), "-gdwarf-2") != cflags.end());
	SetChecked(kProfileControlID, find(cflags.begin(), cflags.end(), "-pg") != cflags.end());
	SetChecked(kPICControlID, find(cflags.begin(), cflags.end(), "-fPIC") != cflags.end());
	SetChecked(kCxx0xControlID, find(cflags.begin(), cflags.end(), "-std=c++0x") != cflags.end());

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

	vector<string>& cflags = target.mCFlags;
//	vector<string>& ldflags = target.mLDFlags;
	
	string s;
	
	switch (inID)
	{
		case kTargetNameControlID:
			GetText(inID, target.mName);
			UpdateTargetPopup(targetPopup.GetActive());
			break;

		case kLinkerOutputControlID:
			GetText(inID, target.mLinkTarget);
			break;

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
		
		case kOutputDirControlID:
			GetText(inID, s);
			mProjectInfo.mOutputDir = s;
			break;

		case kResourcesControlID:
			mProjectInfo.mAddResources = IsChecked(kResourcesControlID);
			break;
		
		case kResourceDirControlID:
			GetText(inID, s);
			mProjectInfo.mResourcesDir = s;
			break;
		
		case kCompilerControlID:
			GetText(inID, target.mCompiler);
			break;
		
		case kDebugInfoControlID:
			cflags.erase(remove(cflags.begin(), cflags.end(), "-gdwarf-2"), cflags.end());
			if (IsChecked(inID))
				cflags.push_back("-gdwarf-2");
			break;
		
		case kProfileControlID:
			cflags.erase(remove(cflags.begin(), cflags.end(), "-pg"), cflags.end());
			if (IsChecked(inID))
				cflags.push_back("-pg");
			break;
		
		case kPICControlID:
			cflags.erase(remove(cflags.begin(), cflags.end(), "-fPIC"), cflags.end());
			if (IsChecked(inID))
				cflags.push_back("-fPIC");
			break;
		
		case kCxx0xControlID:
			cflags.erase(remove(cflags.begin(), cflags.end(), "-std=c++0x"), cflags.end());
			if (IsChecked(inID))
				cflags.push_back("-std=c++0x");
			break;
		
		case 'thre':
			cflags.erase(remove(cflags.begin(), cflags.end(), "-pthread"), cflags.end());
			if (IsChecked(inID))
				cflags.push_back("-pthread");
			break;
		
		case 'opti':
			cflags.erase(remove(cflags.begin(), cflags.end(), "-O0"), cflags.end());
			cflags.erase(remove(cflags.begin(), cflags.end(), "-Os"), cflags.end());
			cflags.erase(remove(cflags.begin(), cflags.end(), "-O1"), cflags.end());
			cflags.erase(remove(cflags.begin(), cflags.end(), "-O2"), cflags.end());
			cflags.erase(remove(cflags.begin(), cflags.end(), "-O3"), cflags.end());
		
			switch (GetValue(inID))
			{
				case 1:	cflags.push_back("-O0"); break;
				case 2:	cflags.push_back("-Os"); break;
				case 3:	cflags.push_back("-O1"); break;
				case 4:	cflags.push_back("-O2"); break;
				case 5:	cflags.push_back("-O3"); break;
			}
			break;
		
		case 'defs':
			GetText(inID, s);
			ba::split(target.mDefines, s, ba::is_any_of("\n\r\t "));
			break;

		case 'warn':
			GetText(inID, s);
			ba::split(target.mWarnings, s, ba::is_any_of("\n\r\t "));
			break;
		
		case 'addt':
			AddTarget();
			break;

		case 'delt':
			DeleteTarget();
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

void MProjectInfoDialog::PkgToggled(
	gchar*			inPath)
{
	GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
	
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(mPkgList), &iter, path);
	gtk_tree_path_free(path);

	bool checked;
	gchar* pkg;
	
	gtk_tree_model_get(GTK_TREE_MODEL(mPkgList), &iter, 0, &checked, 1, &pkg, -1);
	
	if (pkg == nil)
		THROW(("Missing string"));
	
	string test = "pkg-config:";
	
	if (strcmp(pkg, "perl") == 0)
		test = "perl:default";
	else
		test += pkg;
	
	if (checked)
	{
		vector<string>::iterator p = find(
			mProjectInfo.mPkgConfigPkgs.begin(), mProjectInfo.mPkgConfigPkgs.end(),
			test);
		
		if (p != mProjectInfo.mPkgConfigPkgs.end())
			mProjectInfo.mPkgConfigPkgs.erase(p);
	}
	else
		mProjectInfo.mPkgConfigPkgs.push_back(test);
	
	g_free(pkg);

	gtk_tree_store_set(mPkgList, &iter, 0, not checked, -1);
}

void MProjectInfoDialog::AddTarget()
{
	MGtkComboBox targetPopup(GetWidget(kTargetPopupID));
	MProjectTarget newTarget = mProjectInfo.mTargets[targetPopup.GetActive()];
	
	newTarget.mName += _(" (copy)");
	
	mProjectInfo.mTargets.push_back(newTarget);

	UpdateTargetPopup(mProjectInfo.mTargets.size() - 1);

	TargetChanged();

	SetEnabled('delt', true);
}

void MProjectInfoDialog::DeleteTarget()
{
	MGtkComboBox targetPopup(GetWidget(kTargetPopupID));
	
	mProjectInfo.mTargets.erase(mProjectInfo.mTargets.begin() + targetPopup.GetActive());

	UpdateTargetPopup(0);

	TargetChanged();

	SetEnabled('delt', mProjectInfo.mTargets.size() > 1);
}

void MProjectInfoDialog::UpdateTargetPopup(
	uint32			inActive)
{
	MGtkComboBox targetPopup(GetWidget(kTargetPopupID));
	
	eTargetChanged.Block(targetPopup, "on_targ_changed");
	
	targetPopup.RemoveAll();
	
	for (vector<MProjectTarget>::iterator t = mProjectInfo.mTargets.begin(); t != mProjectInfo.mTargets.end(); ++t)
		targetPopup.Append(t->mName);
	
	targetPopup.SetActive(inActive);

	eTargetChanged.Unblock(targetPopup, "on_targ_changed");
}

