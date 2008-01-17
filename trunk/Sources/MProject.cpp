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

#undef check
#ifndef BOOST_DISABLE_ASSERTS
#define BOOST_DISABLE_ASSERTS
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/filesystem/fstream.hpp>

#include "MProject.h"
#include "MListView.h"
#include "MDevice.h"
#include "MGlobals.h"
#include "MCommands.h"
#include "MMessageWindow.h"
#include "MUtils.h"
#include "MSound.h"
#include "MProjectItem.h"
#include "MProjectTarget.h"
#include "MProjectJob.h"
#include "MNewGroupDialog.h"
#include "MProjectInfoDialog.h"
#include "MFindAndOpenDialog.h"
#include "MPkgConfig.h"
#include "MStrings.h"
#include "MAlerts.h"
#include "MPreferences.h"

using namespace std;

namespace
{

const string
	kIQuote("-iquote"),
	kI("-I"),
	kF("-F"),
	kl("-l"),
	kL("-L");

const MColor
	kOutOfDateColor = MColor("#ff664c"),
	kCompilingColor = MColor("#5ea50c");

struct MProjectState
{
	uint16			mWindowPosition[2];
	uint16			mWindowSize[2];
	uint8			mSelectedTarget;
	uint8			mSelectedPanel;
	uint8			mFillers[2];
	int32			mScrollPosition[ePanelCount];
	uint32			mSelectedFile;
	
	void			Swap();
};

const char
	kJapieProjectState[] = "com.hekkelman.japi.ProjectState";

const uint32
	kMProjectStateSize = 7 * sizeof(uint32); // sizeof(MProjectState);

void MProjectState::Swap()
{
	net_swapper swap;
	
	mWindowPosition[0] = swap(mWindowPosition[0]);
	mWindowPosition[1] = swap(mWindowPosition[1]);
	mWindowSize[0] = swap(mWindowSize[0]);
	mWindowSize[1] = swap(mWindowSize[1]);
	mScrollPosition[ePanelFiles] = swap(mScrollPosition[ePanelFiles]);
	mScrollPosition[ePanelLinkOrder] = swap(mScrollPosition[ePanelLinkOrder]);
	mScrollPosition[ePanelPackage] = swap(mScrollPosition[ePanelPackage]);
	mSelectedFile = swap(mSelectedFile);
}

}

// ---------------------------------------------------------------------------
//	MProjectTarget

MTargetCPU MProjectTarget::GetNativeCPU()
{
	MTargetCPU arch;
#if defined(__amd64)
	arch = eCPU_x86_64;
#elif defined(__i386__)
	arch = eCPU_386;
#elif defined(__powerpc64__) or defined(__PPC64__) or defined(__ppc64__)
	arch = eCPU_PowerPC_64;
#elif defined(__powerpc__) or defined(__PPC__) or defined(__ppc__)
	arch = eCPU_PowerPC_32;
#else
#	error("Undefined processor")
#endif
	return arch;
}

#pragma mark -

// ---------------------------------------------------------------------------
//	MProject

MProject* MProject::sInstance;

MProject* MProject::Instance()
{
	return sInstance;
}

MProject::MProject(
	const MPath&		inProjectFile)
	: eMsgWindowClosed(this, &MProject::MsgWindowClosed)
	, ePoll(this, &MProject::Poll)
	, mProjectFile(inProjectFile)
	, mProjectDir(mProjectFile.branch_path())
	, mCurrentTarget(nil)
	, mProjectItems("", nil)
	, mPackageItems("", nil)
	, mCurrentJob(nil)
	, mNext(nil)
{
    LIBXML_TEST_VERSION

	AddRoute(gApp->eIdle, ePoll);

	mNext = sInstance;
	sInstance = this;
}

// ---------------------------------------------------------------------------
//	MProject::MProject

MProject::~MProject()
{
	for (vector<MProjectTarget*>::iterator target = mTargets.begin(); target != mTargets.end(); ++target)
		delete *target;
}

// ---------------------------------------------------------------------------
//	MProject::CloseAllProjects

void MProject::CloseAllProjects(
	MCloseReason	inAction)
{
	MProject* project = sInstance;
	while (project != nil)
	{
		MProject* next = project->mNext;
		
		if (project->mModified)
		{
			project->TryCloseDocument(inAction, project->mName, project);
			break;
		}
		
		project->Close();
		project = next;
	}
}

// ---------------------------------------------------------------------------
//	MProject::Read

void MProject::Read()
{
	xmlDocPtr			xmlDoc = nil;
	xmlXPathContextPtr	xPathContext = nil;
	
	if (fs::extension(mProjectFile) == ".prj")
		mName = fs::basename(mProjectFile);
	else
		mName = mProjectFile.leaf();
	
	xmlInitParser();

	try
	{
		xmlDoc = xmlParseFile(mProjectFile.string().c_str());
		if (xmlDoc == nil)
			THROW(("Failed to parse project file"));
		
		xPathContext = xmlXPathNewContext(xmlDoc);
		if (xPathContext == nil)
			THROW(("Failed to create xpath context for project file"));
		
		Read(xPathContext);

		xmlXPathFreeContext(xPathContext);
		xmlFreeDoc(xmlDoc);
	}
	catch (...)
	{
		if (xPathContext != nil)
			xmlXPathFreeContext(xPathContext);
		
		if (xmlDoc != nil)
			xmlFreeDoc(xmlDoc);
		
		xmlCleanupParser();
		throw;
	}
	
	xmlCleanupParser();
}

// ---------------------------------------------------------------------------
//	MProject::ReadState

bool MProject::ReadState(
	MRect&		outWindowPosition)
{
	bool result = false;
	
	if (Preferences::GetInteger("save state", 1))
	{
		MProjectState state = {};
		ssize_t r = read_attribute(mProjectFile, kJapieProjectState, &state, kMProjectStateSize);
		if (r > 0 and static_cast<uint32>(r) == kMProjectStateSize)
		{
			state.Swap();
			
//			mFileList->ScrollToPosition(0, state.mScrollPosition[ePanelFiles]);
			
			if (state.mWindowSize[0] > 50 and state.mWindowSize[1] > 50 and
				state.mWindowSize[0] < 2000 and state.mWindowSize[1] < 2000)
			{
				outWindowPosition = MRect(
					state.mWindowPosition[0], state.mWindowPosition[1],
					state.mWindowSize[0], state.mWindowSize[1]));
			}
			
	//		mLinkOrderList->ScrollToPosition(::CGPointMake(0, state.mScrollPosition[ePanelLinkOrder]));
	//		mPackageList->ScrollToPosition(::CGPointMake(0, state.mScrollPosition[ePanelPackage]));
			
	//		if (state.mSelectedPanel < ePanelCount)
	//		{
	//			mPanel = static_cast<MProjectListPanel>(state.mSelectedPanel);
	//			::SetControl32BitValue(mPanelSegmentRef, uint32(mPanel) + 1);
	//			SelectPanel(mPanel);
	//		}
	//		else
	//			mPanel = ePanelFiles;
	//		
	//		::MoveWindow(GetSysWindow(),
	//			state.mWindowPosition[0], state.mWindowPosition[1], true);
	//
	//		::SizeWindow(GetSysWindow(),
	//			state.mWindowSize[0], state.mWindowSize[1], true);
	//
	//		::ConstrainWindowToScreen(GetSysWindow(),
	//			kWindowStructureRgn, kWindowConstrainStandardOptions,
	//			NULL, NULL);
	
			SelectTarget(state.mSelectedTarget);
//			mFileList->SelectItem(state.mSelectedFile);
			
			result = true;
		}
	}
	
	return result;
}

//// ---------------------------------------------------------------------------
////	MProject::DoClose
//
//bool MProject::DoClose()
//{
//	bool result = false;
//	
//	if (mModified)
//		TryCloseDocument(kSaveChangesClosingDocument, mName, this);
//	else
//	{
//		if (Preferences::GetInteger("save state", 1))
//		{
//			try
//			{
//				MProjectState state = { };
//		
//				(void)read_attribute(mProjectFile, kJapieProjectState, &state, kMProjectStateSize);
//				
//				state.Swap();
//	
//				state.mSelectedFile = mFileList->GetSelected();
//				state.mSelectedTarget =
//					find(mTargets.begin(), mTargets.end(), mCurrentTarget) - mTargets.begin();
//	
//				int32 x;
//				mFileList->GetScrollPosition(x, state.mScrollPosition[ePanelFiles]);
////				mLinkOrderList->GetScrollPosition(pt);
////				state.mScrollPosition[ePanelLinkOrder] = static_cast<uint32>(pt.y);
////				mPackageList->GetScrollPosition(pt);
////				state.mScrollPosition[ePanelPackage] = static_cast<uint32>(pt.y);
//				
//				state.mSelectedPanel = mPanel;
//	
//				MRect r;
//				GetWindowPosition(r);
//				state.mWindowPosition[0] = r.x;
//				state.mWindowPosition[1] = r.y;
//				state.mWindowSize[0] = r.width;
//				state.mWindowSize[1] = r.height;
//		
//				state.Swap();
//				
//				write_attribute(mProjectFile, kJapieProjectState, &state, kMProjectStateSize);
//			}
//			catch (...) {}
//		}
//		
//		result = MWindow::DoClose();
//	}
//	
//	if (result)
//	{
//		StopBuilding();
//		
//		if (sInstance == this)
//			sInstance = mNext;
//		else
//		{
//			MProject* p = sInstance;
//	
//			while (p != nil and p->mNext != this)
//				p = p->mNext;
//			
//			if (p != nil)
//				p->mNext = mNext;
//		}
//	}
//		
//	return result;
//}

// ---------------------------------------------------------------------------
//	MProject::DoRawKeyDown

//OSStatus MProject::DoRawKeyDown(EventRef inEvent)
//{
//	OSStatus err = eventNotHandledErr;
//	
//	if (::IsUserCancelEventRef(inEvent))
//	{
//		StopBuilding();
//		err = noErr;
//	}
//	
//	return err;
//}

// ---------------------------------------------------------------------------
//	MProject::StopBuilding

void MProject::StopBuilding()
{
	if (mCurrentJob.get() != nil)
	{
		mCurrentJob->Kill();
		mCurrentJob.reset(nil);
	}
	
	eStatus("", false);
}

// ---------------------------------------------------------------------------
//	MProject::IsFileInProject

bool MProject::IsFileInProject(
	const MPath&		inPath) const
{
	return GetProjectFileForPath(inPath) != nil;
}

// ---------------------------------------------------------------------------
//	MProject::RecheckFiles

void MProject::RecheckFiles()
{
	MProject* project = sInstance;
	while (project != nil)
	{
		project->CheckIsOutOfDate();
		project = project->mNext;
	}
}

// ---------------------------------------------------------------------------
//	MProject::CreateNewGroup

void MProject::CreateNewGroup(
	const string&	inGroupName)
{
	int32 selected = mFileList->GetSelected();
	
	MProjectGroup* parent = &mProjectItems;
	int32 position = kListItemLast;
	
	if (selected >= 0 and selected < static_cast<int32>(mFileList->GetCount()))
	{
		MProjectItem* item = GetItem(selected);
		parent = item->GetParent();
		position = item->GetPosition();
	}
	
	parent->AddProjectItem(new MProjectGroup(inGroupName, nil), position);
	
	UpdateList();
	SetModified(true);
}

// ---------------------------------------------------------------------------
//	MProject::TargetUpdated

void MProject::TargetUpdated()
{
//	ControlID id = { kJapieSignature, kTargetPopupViewID };
//	
//	THROW_IF_OSERROR(::HIViewFindByID(GetContentViewRef(), id, &mTargetPopupRef));
//	MenuRef targetMenuRef = ::GetControlPopupMenuHandle(mTargetPopupRef);
//	if (mTargetPopupRef == nil)
//		THROW(("Missing target menu"));
//	
//	THROW_IF_OSERROR(::DeleteMenuItems(targetMenuRef, 1, ::CountMenuItems(targetMenuRef)));
//	uint32 ix = 1;
//	
//	for (vector<MProjectTarget*>::iterator target = mTargets.begin();
//		target != mTargets.end(); ++target, ++ix)
//	{
//		MCFString targetTitle((*target)->GetName());
//		THROW_IF_OSERROR(::InsertMenuItemTextWithCFString(
//			targetMenuRef, targetTitle, ix, 0, cmd_SwitchTarget + ix - 1));
//	}

	while (mTargetPopupCount-- > 0)
		gtk_combo_box_remove_text(GTK_COMBO_BOX(mTargetPopup), 0);

	mTargetPopupCount = 0;
	for (vector<MProjectTarget*>::iterator target = mTargets.begin();
		target != mTargets.end(); ++target, ++mTargetPopupCount)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(mTargetPopup), (*target)->GetName().c_str());
	}

	SetModified(true);
}

// ---------------------------------------------------------------------------
//	MProject::SetProjectPaths

void MProject::SetProjectPaths(
	const vector<MPath>&	inUserPaths,
	const vector<MPath>&	inSysPaths,
	const vector<MPath>&	inLibPaths,
	const vector<MPath>&	inFrameworks)
{
	if (inUserPaths != mUserSearchPaths or
		inSysPaths != mSysSearchPaths or
		inLibPaths != mLibSearchPaths or
		inFrameworks != mFrameworkPaths)
	{
		mUserSearchPaths = inUserPaths;
		mSysSearchPaths = inSysPaths;
		mLibSearchPaths = inLibPaths;
		mFrameworkPaths = inFrameworks;
		
		SetModified(true);
		
		ResearchForFiles();
	}
}

// ---------------------------------------------------------------------------
//	MProject::ReadPaths

void MProject::ReadPaths(
	xmlXPathObjectPtr	inData,
	vector<MPath>&		outPaths)
{
	for (int i = 0; i < inData->nodesetval->nodeNr; ++i)
	{
		xmlNodePtr node = inData->nodesetval->nodeTab[i];
		
		if (node->children == nil)
			continue;
		
		const xmlChar* text = XML_GET_CONTENT(node->children);
		if (text == nil)
			THROW(("Invalid project file, missing path"));
		
		string path((const char*)text);

		if (path.length() > 0 and path[0] == '/')
			outPaths.push_back(MPath(path));
		else
			outPaths.push_back(mProjectDir / path);
	}
}

// ---------------------------------------------------------------------------
//	MProject::ReadOptions

void MProject::ReadOptions(
	xmlNodePtr			inData,
	const char*			inOptionName,
	MProjectTarget*		inTarget,
	AddOptionProc		inAddOptionProc)
{
	for (xmlNodePtr node = inData->children; node != nil; node = node->next)
	{
		if (xmlNodeIsText(node) or strcmp((const char*)node->name, inOptionName) != 0)
			continue;
		
		const char* option = (const char*)XML_GET_CONTENT(node->children);
		if (option == nil)
			THROW(("invalid option in project"));
		
		(inTarget->*inAddOptionProc)(option);
	}
}

// ---------------------------------------------------------------------------
//	MProject::ReadPackageAction

void MProject::ReadPackageAction(
	xmlNodePtr		inData,
	MPath			inDir,
	MProjectGroup*	inGroup)
{
	XMLNode data(inData);
	
	for (XMLNode::iterator node = data.begin(); node != data.end(); ++node)
	{		
		if (node->name() == "copy")
		{
			string fileName = node->text();
			if (fileName.length() == 0)
				THROW(("Invalid project file"));
			
			try
			{
				MPath filePath = inDir / fileName;

				auto_ptr<MProjectResource> projectFile(
					new MProjectResource(filePath.leaf(), inGroup, filePath.branch_path()));

				AddRoute(projectFile->eStatusChanged, eProjectFileStatusChanged);

				inGroup->AddProjectItem(projectFile.release());
			}
			catch (std::exception& e)
			{
				DisplayError(e);
			}
		}
		else if (node->name() == "mkdir")
		{
			string name = node->property("name");
			
			auto_ptr<MProjectMkDir> group(new MProjectMkDir(name, inGroup));
			
			if (node->children() != nil)
				ReadPackageAction(*node, inDir / name, group.get());
			
			inGroup->AddProjectItem(group.release());
		}
	}
}

// ---------------------------------------------------------------------------
//	MProject::ReadFiles

void MProject::ReadFiles(
	xmlNodePtr		inData,
	MProjectGroup*	inGroup)
{
	XMLNode data(inData);
	
	for (XMLNode::iterator node = data.begin(); node != data.end(); ++node)
	{
		if (node->name() == "file")
		{
			string fileName = node->text();
			if (fileName.length() == 0)
				THROW(("Invalid project file"));
			
			MPath filePath;
			try
			{
				auto_ptr<MProjectFile> projectFile;
				
				if (LocateFile(fileName, true, filePath))
					projectFile.reset(new MProjectFile(fileName, inGroup, filePath.branch_path()));
				else
					projectFile.reset(new MProjectFile(fileName, inGroup, MPath()));
				
				if (node->property("optional") == "true")
					projectFile->SetOptional(true);

				AddRoute(projectFile->eStatusChanged, eProjectFileStatusChanged);

				inGroup->AddProjectItem(projectFile.release());
			}
			catch (std::exception& e)
			{
				DisplayError(e);
			}
		}
		else if (node->name() == "link")
		{
			string linkName = node->text();
			if (linkName.length() == 0)
				THROW(("Invalid project file"));
			
			inGroup->AddProjectItem(new MProjectLib(linkName, inGroup));
		}
		else if (node->name() == "group")
		{
			string name = node->property("name");
			
			auto_ptr<MProjectGroup> group(new MProjectGroup(name, inGroup));
			
			if (node->children() != nil)
				ReadFiles(*node, group.get());
			
			inGroup->AddProjectItem(group.release());
		}
	}
}

// ---------------------------------------------------------------------------
//	MProject::Read

void MProject::Read(
	xmlXPathContextPtr	inContext)
{
	xmlXPathObjectPtr data;
	
	// read the pkg-config data
	
	data = xmlXPathEvalExpression(BAD_CAST "/project/pkg-config/pkg", inContext);
	
	if (data != nil)
	{
		if (data->nodesetval != nil)
		{
			for (int i = 0; i < data->nodesetval->nodeNr; ++i)
			{
				xmlNodePtr node = data->nodesetval->nodeTab[i];
				
				if (node->children == nil)
					continue;
				
				const xmlChar* text = XML_GET_CONTENT(node->children);
				if (text == nil)
					THROW(("Invalid project file, missing pkg"));
				
				string pkg((const char*)text);
				mPkgConfigPkgs.push_back(pkg);
			}
		}
			
		xmlXPathFreeObject(data);
	}
	
	// read the system search paths
	
	data = xmlXPathEvalExpression(BAD_CAST "/project/syspaths/path", inContext);
	
	if (data != nil)
	{
		if (data->nodesetval != nil)
			ReadPaths(data, mSysSearchPaths);

		xmlXPathFreeObject(data);
	}
	
	// next the user search paths
	data = xmlXPathEvalExpression(BAD_CAST "/project/userpaths/path", inContext);
	
	if (data != nil)
	{
		if (data->nodesetval != nil)
			ReadPaths(data, mUserSearchPaths);

		xmlXPathFreeObject(data);
	}
	
	// then the lib search paths
	data = xmlXPathEvalExpression(BAD_CAST "/project/libpaths/path", inContext);
	
	if (data != nil)
	{
		if (data->nodesetval != nil)
			ReadPaths(data, mLibSearchPaths);

		xmlXPathFreeObject(data);
	}
	
	// then the framework paths
	data = xmlXPathEvalExpression(BAD_CAST "/project/frameworks/path", inContext);
	
	if (data != nil)
	{
		if (data->nodesetval != nil)
			ReadPaths(data, mFrameworkPaths);

		xmlXPathFreeObject(data);
	}
	
	// now we're ready to read the files
	
	data = xmlXPathEvalExpression(BAD_CAST "/project/files", inContext);
	
	if (data != nil and data->nodesetval != nil)
	{
		for (int i = 0; i < data->nodesetval->nodeNr; ++i)
			ReadFiles(data->nodesetval->nodeTab[i], &mProjectItems);
	}
	
	if (data != nil)
		xmlXPathFreeObject(data);

	// and the package actions, if any
	
	data = xmlXPathEvalExpression(BAD_CAST "/project/package", inContext);
	
	if (data != nil and data->nodesetval != nil)
	{
		for (int i = 0; i < data->nodesetval->nodeNr; ++i)
		{
			XMLNode node(data->nodesetval->nodeTab[i]);

			string rd = node.property("resource_dir");
			if (rd.length() > 0)
				mResourcesDir = mProjectDir / rd;
	
			MPath dir;		// empty
			ReadPackageAction(node, dir, &mPackageItems);
		}
	}
	
	if (data != nil)
		xmlXPathFreeObject(data);

	// And finally we add the targets
	
	data = xmlXPathEvalExpression(BAD_CAST "/project/targets/target", inContext);
	
	if (data != nil and data->nodesetval != nil)
	{
		for (int i = 0; i < data->nodesetval->nodeNr; ++i)
		{
			XMLNode targetNode(data->nodesetval->nodeTab[i]);
			
			string linkTarget = targetNode.property("linkTarget");
			if (linkTarget.length() == 0)
				THROW(("Invalid target, missing linkTarget"));
			
			string name = targetNode.property("name");
			if (name.length() == 0)
				THROW(("Invalid target, missing name"));
			
			string p = targetNode.property("kind");
			if (p.length() == 0)
				THROW(("Invalid target, missing kind"));
			
			MTargetKind kind;
			if (p == "Application Package")
				kind = eTargetApplicationPackage;
			else if (p == "Shared Library")
				kind = eTargetSharedLibrary;
			else if (p == "Static Library")
				kind = eTargetStaticLibrary;
			else if (p == "Executable")
				kind = eTargetExecutable;
			else
				THROW(("Unsupported target kind"));
			
//			if (kind == eTargetExecutable or kind == eTargetStaticLibrary or kind == eTargetSharedLibrary)
//				::DisableControl(mPackagePanelRef);
			
			MTargetCPU arch = eCPU_native;

			p = targetNode.property("arch");
			if (p.length() > 0)
			{
				if (p == "ppc")
					arch = eCPU_PowerPC_32;
				else if (p == "ppc64")
					arch = eCPU_PowerPC_64;
				else if (p == "i386")
					arch = eCPU_386;
				else if (p == "amd64")
					arch = eCPU_x86_64;
				else if (p != "native")
					THROW(("Unsupported target arch"));
			}
			
			auto_ptr<MProjectTarget> target(new MProjectTarget(linkTarget, name, kind, arch));
			
			p = targetNode.property("debug");
			if (p == "true")
				target->SetDebugFlag(true);
			
			for (XMLNode::iterator node = targetNode.begin(); node != targetNode.end(); ++node)
			{
				if (node->name() == "bundle")
				{
					for (XMLNode::iterator d = node->begin(); d != node->end(); ++d)
					{
						if (d->name() == "name")
							target->SetBundleName(d->text());
						else if (d->name() == "creator")
							target->SetCreator(d->text());
						else if (d->name() == "type")
							target->SetType(d->text());
					}
				}
				else if (node->name() == "defines")
					ReadOptions(*node, "define", target.get(), &MProjectTarget::AddDefine);
				else if (node->name() == "cflags")
					ReadOptions(*node, "cflag", target.get(), &MProjectTarget::AddCFlag);
				else if (node->name() == "ldflags")
					ReadOptions(*node, "ldflag", target.get(), &MProjectTarget::AddLDFlag);
				else if (node->name() == "frameworks")
					ReadOptions(*node, "framework", target.get(), &MProjectTarget::AddFramework);
				else if (node->name() == "warnings")
					ReadOptions(*node, "warning", target.get(), &MProjectTarget::AddWarning);
			}
			
			mTargets.push_back(target.release());
		}
	}
	
	if (data != nil)
		xmlXPathFreeObject(data);

	mModified = false;
}

// ---------------------------------------------------------------------------
//	MProject::Write routines

#define THROW_IF_XML_ERR(x)		do { int __err = (x); if (__err < 0) THROW((#x " failed")); } while (false)

// ---------------------------------------------------------------------------
//	MProject::WritePaths

void MProject::WritePaths(
	xmlTextWriterPtr	inWriter,
	const char*			inTag,
	vector<MPath>&		inPaths,
	bool				inFullPath)
{
	THROW_IF_XML_ERR(xmlTextWriterStartElement(inWriter, BAD_CAST inTag));
	
	for (vector<MPath>::iterator p = inPaths.begin(); p != inPaths.end(); ++p)
	{
		string path;
		if (inFullPath)
			path = fs::system_complete(*p).string();
		else
			path = relative_path(mProjectDir, *p).string();

		THROW_IF_XML_ERR(xmlTextWriterWriteElement(inWriter, BAD_CAST "path", BAD_CAST path.c_str()));
	}
	
	THROW_IF_XML_ERR(xmlTextWriterEndElement(inWriter));
}

// ---------------------------------------------------------------------------
//	MProject::WriteFiles

void MProject::WriteFiles(
	xmlTextWriterPtr		inWriter,
	vector<MProjectItem*>&	inItems)
{
	for (vector<MProjectItem*>::iterator item = inItems.begin(); item != inItems.end(); ++item)
	{
		if (dynamic_cast<MProjectGroup*>(*item) != nil)
		{
			THROW_IF_XML_ERR(xmlTextWriterStartElement(inWriter, BAD_CAST "group"));
			
			MProjectGroup* group = static_cast<MProjectGroup*>(*item);
			
			THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "name",
				BAD_CAST group->GetName().c_str()));
			
			WriteFiles(inWriter, group->GetItems());
			
			THROW_IF_XML_ERR(xmlTextWriterEndElement(inWriter));
		}
		else if (dynamic_cast<MProjectLib*>(*item) != nil)
		{
			THROW_IF_XML_ERR(xmlTextWriterWriteElement(inWriter,
				BAD_CAST "link", BAD_CAST (*item)->GetName().c_str()));
		}
		else
		{
			THROW_IF_XML_ERR(xmlTextWriterStartElement(inWriter, BAD_CAST "file"));
			
			MProjectFile* file = dynamic_cast<MProjectFile*>(*item);
		
			if (file != nil and file->IsOptional())
			{
				THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "optional",
					BAD_CAST "true"));
			}
			
			THROW_IF_XML_ERR(xmlTextWriterWriteString(inWriter,
				BAD_CAST (*item)->GetName().c_str()));
			
			THROW_IF_XML_ERR(xmlTextWriterEndElement(inWriter));
		}
	}
}

// ---------------------------------------------------------------------------
//	MProject::WritePackage

void MProject::WritePackage(
	xmlTextWriterPtr		inWriter,
	const MPath&			inDir,
	vector<MProjectItem*>&	inItems)
{
	for (vector<MProjectItem*>::iterator item = inItems.begin(); item != inItems.end(); ++item)
	{
		if (dynamic_cast<MProjectGroup*>(*item) != nil)
		{
			THROW_IF_XML_ERR(xmlTextWriterStartElement(inWriter, BAD_CAST "mkdir"));
			
			MProjectGroup* group = static_cast<MProjectGroup*>(*item);
			
			THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "name",
				BAD_CAST group->GetName().c_str()));
			
			WritePackage(inWriter, inDir / group->GetName(), group->GetItems());
			
			THROW_IF_XML_ERR(xmlTextWriterEndElement(inWriter));
		}
		else if (dynamic_cast<MProjectResource*>(*item) != nil)
		{
			MPath path = mResourcesDir / static_cast<MProjectResource*>(*item)->GetPath();

			THROW_IF_XML_ERR(xmlTextWriterWriteElement(inWriter, BAD_CAST "copy",
				BAD_CAST relative_path(inDir, path).string().c_str()));

//			THROW_IF_XML_ERR(xmlTextWriterWriteElement(inWriter, BAD_CAST "copy",
//				BAD_CAST path.string().c_str()));
		}
	}
}

// ---------------------------------------------------------------------------
//	MProject::WriteTarget

void MProject::WriteTarget(
	xmlTextWriterPtr	inWriter,
	MProjectTarget&		inTarget)
{
	// <target>
	THROW_IF_XML_ERR(xmlTextWriterStartElement(inWriter, BAD_CAST "target"));
	
	switch (inTarget.GetKind())
	{
		case eTargetExecutable:
			THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "kind",
				BAD_CAST "Executable"));
			break;
		
		case eTargetStaticLibrary:
			THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "kind",
				BAD_CAST "Static Library"));
			break;
		
		case eTargetSharedLibrary:
			THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "kind",
				BAD_CAST "Shared Library"));
			break;
		
		case eTargetApplicationPackage:
			THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "kind",
				BAD_CAST "Application Package"));
			break;
		
		default:
			break;
	}
		
	switch (inTarget.GetTargetCPU())
	{
		case eCPU_PowerPC_32:
			THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "arch", BAD_CAST "ppc"));
			break;
		
		case eCPU_PowerPC_64:
			THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "arch", BAD_CAST "ppc64"));
			break;
		
		case eCPU_386:
			THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "arch", BAD_CAST "i386"));
			break;
		
		case eCPU_x86_64:
			THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "arch", BAD_CAST "amd64"));
			break;
		
		default:
			break;
	}
	
	THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "name",
		BAD_CAST inTarget.GetName().c_str()));
	
	THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "linkTarget",
		BAD_CAST inTarget.GetLinkTarget().c_str()));

	if (inTarget.GetDebugFlag())
		THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(inWriter, BAD_CAST "debug", BAD_CAST "true"));
	
	if (inTarget.GetBundleName().length() > 0)
	{
		// <bundle>
		
		THROW_IF_XML_ERR(xmlTextWriterStartElement(inWriter, BAD_CAST "bundle"));
		
		THROW_IF_XML_ERR(xmlTextWriterWriteElement(inWriter, BAD_CAST "name",
			BAD_CAST inTarget.GetBundleName().c_str()));

		if (inTarget.GetType().length() == 4)
		{
			THROW_IF_XML_ERR(xmlTextWriterWriteElement(inWriter, BAD_CAST "type",
				BAD_CAST inTarget.GetType().c_str()));
		}

		if (inTarget.GetCreator().length() == 4)
		{
			THROW_IF_XML_ERR(xmlTextWriterWriteElement(inWriter, BAD_CAST "creator",
				BAD_CAST inTarget.GetCreator().c_str()));
		}
		
		// </bundle>
		THROW_IF_XML_ERR(xmlTextWriterEndElement(inWriter));
	}
	
	WriteOptions(inWriter, "defines", "define", inTarget.GetDefines());
	WriteOptions(inWriter, "cflags", "cflag", inTarget.GetCFlags());
	WriteOptions(inWriter, "warnings", "warning", inTarget.GetWarnings());
	WriteOptions(inWriter, "frameworks", "framework", inTarget.GetFrameworks());
	
	// </target>
	THROW_IF_XML_ERR(xmlTextWriterEndElement(inWriter));
}

// ---------------------------------------------------------------------------
//	MProject::WriteOptions

void MProject::WriteOptions(
	xmlTextWriterPtr		inWriter,
	const char*				inOptionGroupName,
	const char*				inOptionName,
	const vector<string>&	inOptions)
{
	if (inOptions.size() > 0)
	{
		// <optiongroup>
		
		THROW_IF_XML_ERR(xmlTextWriterStartElement(inWriter, BAD_CAST inOptionGroupName));
	
		for (vector<string>::const_iterator o = inOptions.begin(); o != inOptions.end(); ++o)
		{
			THROW_IF_XML_ERR(xmlTextWriterWriteElement(inWriter, BAD_CAST inOptionName,
				BAD_CAST o->c_str()));
		}
	
		// </optiongroup>
		THROW_IF_XML_ERR(xmlTextWriterEndElement(inWriter));
	}
}

// ---------------------------------------------------------------------------
//	MProject::Write

bool MProject::Write(
	ostream*			inFile)
{
	bool result = false;
	xmlBufferPtr buf = nil;
	
	try
	{
		buf = xmlBufferCreate();
		if (buf == nil)
			THROW(("Failed to create xml buffer"));
		
		xmlTextWriterPtr writer = xmlNewTextWriterMemory(buf, 0);
		if (writer == nil)
			THROW(("Failed to create xml writer"));

		THROW_IF_XML_ERR(xmlTextWriterSetIndent(writer, 1));
		THROW_IF_XML_ERR(xmlTextWriterSetIndentString(writer, BAD_CAST "\t"));
		
		THROW_IF_XML_ERR(xmlTextWriterStartDocument(writer, nil, nil, nil));
		
		// <project>
		THROW_IF_XML_ERR(xmlTextWriterStartElement(writer, BAD_CAST "project"));
		
		// pkg-config
		if (mPkgConfigPkgs.size() > 0)
		{
			THROW_IF_XML_ERR(xmlTextWriterStartElement(writer, BAD_CAST "pkg-config"));
			
			for (vector<string>::iterator p = mPkgConfigPkgs.begin(); p != mPkgConfigPkgs.end(); ++p)
				THROW_IF_XML_ERR(xmlTextWriterWriteElement(writer, BAD_CAST "pkg", BAD_CAST p->c_str()));
			
			THROW_IF_XML_ERR(xmlTextWriterEndElement(writer));
		}
		
		WritePaths(writer, "syspaths", mSysSearchPaths, true);
		WritePaths(writer, "userpaths", mUserSearchPaths, false);
		WritePaths(writer, "libpaths", mLibSearchPaths, true);
		WritePaths(writer, "frameworks", mFrameworkPaths, true);

		// <files>
		THROW_IF_XML_ERR(xmlTextWriterStartElement(writer, BAD_CAST "files"));

		WriteFiles(writer, mProjectItems.GetItems());
		
		// </files>
		THROW_IF_XML_ERR(xmlTextWriterEndElement(writer));

		// <package>
		THROW_IF_XML_ERR(xmlTextWriterStartElement(writer, BAD_CAST "package"));

		THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(writer,
			BAD_CAST "resource_dir",
			BAD_CAST relative_path(mProjectDir, mResourcesDir).string().c_str()));

		WritePackage(writer, mResourcesDir, mPackageItems.GetItems());
		
		// </package>
		THROW_IF_XML_ERR(xmlTextWriterEndElement(writer));

		// <targets>
		THROW_IF_XML_ERR(xmlTextWriterStartElement(writer, BAD_CAST "targets"));

		for (vector<MProjectTarget*>::iterator target = mTargets.begin(); target != mTargets.end(); ++target)
			WriteTarget(writer, **target);
		
		// </targets>
		THROW_IF_XML_ERR(xmlTextWriterEndElement(writer));
		
		// </project>
		THROW_IF_XML_ERR(xmlTextWriterEndElement(writer));

		THROW_IF_XML_ERR(xmlTextWriterEndDocument(writer));
		
		xmlFreeTextWriter(writer);
		
		inFile->write(reinterpret_cast<const char*>(buf->content), buf->use);
		
		mModified = false;
		result = true;
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	if (buf != nil)
		xmlBufferFree(buf);
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProject::Poll

void MProject::Poll(
	double		inSystemTime)
{
	if (mCurrentJob.get() != nil)
	{
		try
		{
			if (mCurrentJob->IsDone())
			{
				if (mCurrentJob->mStatus == 0)
					PlaySound("success");
				else
					PlaySound("failure");
	
				mCurrentJob.reset(nil);
				eStatus("", false);
			}
		}
		catch (exception& e)
		{
			DisplayError(e);
			StopBuilding();
		}
	}
}

// ---------------------------------------------------------------------------
//	MProject::StartJob

void MProject::StartJob(
	MProjectJob*	inJob)
{
	auto_ptr<MProjectJob> job(inJob);
	
	if (mCurrentJob.get() != nil)
		THROW(("Cannot start a job when another already runs"));
	
	mCurrentJob = job;
	
	if (mStdErrWindow.get() != nil)
		mStdErrWindow->Close();

	mCurrentJob->Execute();
}

// ---------------------------------------------------------------------------
//	MProject::GetProjectFileForPath

MProjectFile* MProject::GetProjectFileForPath(
	const MPath&		inPath) const
{
	return mProjectItems.GetProjectFileForPath(inPath);
}

// ---------------------------------------------------------------------------
//	MProject::LocateFile

bool MProject::LocateFile(
	const string&		inFile,
	bool				inSearchUserPaths,
	MPath&				outPath) const
{
	bool found = false;
	
	if (FileNameMatches("*.a;*.dylib", inFile))
	{
		for (vector<MPath>::const_iterator p = mLibSearchPaths.begin();
			 not found and p != mLibSearchPaths.end();
			 ++p)
		{
			outPath = *p / inFile;
			found = exists(outPath);
		}
		
		if (not found)
		{
			for (vector<MPath>::const_iterator p = mCLibSearchPaths.begin();
				 not found and p != mCLibSearchPaths.end();
				 ++p)
			{
				outPath = *p / inFile;
				found = exists(outPath);
			}
		}
	}
	else
	{
		if (inSearchUserPaths)
		{
			for (vector<MPath>::const_iterator p = mUserSearchPaths.begin();
				 not found and p != mUserSearchPaths.end();
				 ++p)
			{
				outPath = *p / inFile;
				found = exists(outPath);
			}
		}

		for (vector<MPath>::const_iterator p = mSysSearchPaths.begin();
			 not found and p != mSysSearchPaths.end();
			 ++p)
		{
			outPath = *p / inFile;
			found = exists(outPath);
		}
		
		for (vector<string>::const_iterator f = mPkgConfigCFlags.begin();
			 not found and f != mPkgConfigCFlags.end();
			 ++f)
		{
			if (f->length() > 2 and f->substr(0, 2) == "-I")
			{
				outPath = fs::path(f->substr(2)) / inFile;
				found = exists(outPath);
			}
		}
		
		string::size_type s;
		if (not found and (s = inFile.find('/')) != string::npos)
		{
			string framework = inFile.substr(0, s);
			string filename = inFile.substr(s + 1);
			
			for (vector<MPath>::const_iterator p = mFrameworkPaths.begin();
				 not found and p != mFrameworkPaths.end();
				 ++p)
			{
				found = LocateInFramework(*p, framework, filename, outPath);
			}
		}
		
		if (not found and fs::exists(mCppIncludeDir))
		{
			outPath = MPath(mCppIncludeDir) / inFile;
			found = fs::exists(outPath);
		}
	}
	
	return found;
}

// ---------------------------------------------------------------------------
//	MProject::LocateInFramework

bool MProject::LocateInFramework(
	const MPath&	inFrameworksPath,
	const string&	inFramework,
	const string&	inFile,
	MPath&			outPath) const
{
	outPath = inFrameworksPath / (inFramework + ".framework") / "Headers" / inFile;
	
	bool found = exists(outPath);

	if (not found)
	{
		MFileIterator iter(inFrameworksPath, kFileIter_ReturnDirectories);
		iter.SetFilter("*.framework");

		MPath fw;
		while (not found and iter.Next(fw))
		{
			MPath subFramework = fw / "Frameworks";
			
			if (exists(subFramework) and is_directory(subFramework))
				found = LocateInFramework(subFramework, inFramework, inFile, outPath);
		}
	}
	
	return found;
}

// ---------------------------------------------------------------------------
//	MProject::GetIncludePaths

void MProject::GetIncludePaths(
	vector<MPath>&	outPaths) const
{
	copy(mSysSearchPaths.begin(), mSysSearchPaths.end(), back_inserter(outPaths));
	
	// duh... ••• TODO, fix? We only search at most one sub framework level deep
	
//	foreach (const MPath& p, mFrameworkPaths)
	for (vector<MPath>::const_iterator p = mFrameworkPaths.begin(); p != mFrameworkPaths.end(); ++p)
	{
		fs::directory_iterator end;
		for (fs::directory_iterator fw(*p); fw != end; ++fw)
		{
			if (not fs::is_directory(*fw))
				continue;
			
			MPath headers = *fw;
			headers /= "Headers";
			
			if (exists(headers) and is_directory(headers))
				outPaths.push_back(headers);
			
			MPath sub = *fw;
			sub /= "Frameworks";
			
			if (fs::exists(sub) and fs::is_directory(sub))
			{
				for (fs::directory_iterator fw(sub); fw != end; ++fw)
				{
					if (not fs::is_directory(*fw))
						continue;
					
					MPath headers = *fw;
					headers /= "Headers";
					
					if (fs::exists(headers) and fs::is_directory(headers))
						outPaths.push_back(headers);
				}
			}
		}
	}

	for (vector<string>::const_iterator f = mPkgConfigCFlags.begin();
		 f != mPkgConfigCFlags.end();
		 ++f)
	{
		if (f->length() > 2 and f->substr(0, 2) == "-I")
			outPaths.push_back(MPath(f->substr(2)));
	}

	copy(mUserSearchPaths.begin(), mUserSearchPaths.end(), back_inserter(outPaths));
}

// ---------------------------------------------------------------------------
//	MProject::CheckDataDir

void MProject::CheckDataDir()
{
	if (not exists(mProjectDir) or not is_directory(mProjectDir))
		THROW(("Project dir does not seem to exist"));
	
	mProjectDataDir = mProjectDir / (mName + "_Data");
	if (not exists(mProjectDataDir))
		fs::create_directory(mProjectDataDir);
	else if (not is_directory(mProjectDataDir))
		THROW(("Project data dir is not valid"));
	
	mObjectDir = mProjectDataDir / (mCurrentTarget->GetName() + "_Object");
	if (not exists(mObjectDir))
		fs::create_directory(mObjectDir);
	else if (not is_directory(mObjectDir))
		THROW(("Project data dir is not valid"));
}

// ---------------------------------------------------------------------------
//	MProject::GetObjectPathForFile

MPath MProject::GetObjectPathForFile(
	const MPath&	inFile) const
{
	MProjectFile* file = GetProjectFileForPath(inFile);
	
	if (file == nil)
		THROW(("File is not in project"));
	
	return file->GetObjectPath();
}

// ---------------------------------------------------------------------------
//	MProject::CreateCompileJob

MProjectJob* MProject::CreateCompileJob(
	const MPath&	inFile)
{
	CheckDataDir();
	
	MProjectFile* file = GetProjectFileForPath(inFile);
	if (file == nil)
		THROW(("File is not part of the target project"));

	assert(mCurrentTarget != nil);
	MProjectTarget& target = *mCurrentTarget;
	
	vector<string> argv;
	
	argv.push_back(Preferences::GetString("c++", "c++"));

//	argv.push_back("-arch");
//	if (target.GetArch() == eTargetArchPPC_32)
//		argv.push_back("ppc");
//	else if (target.GetArch() == eTargetArchx86_32)
//		argv.push_back("i386");
//	else
//		assert(false);

	transform(target.GetDefines().begin(), target.GetDefines().end(),
		back_inserter(argv), bind1st(plus<string>(), "-D"));

	transform(target.GetWarnings().begin(), target.GetWarnings().end(),
		back_inserter(argv), bind1st(plus<string>(), "-W"));

	copy(mPkgConfigCFlags.begin(), mPkgConfigCFlags.end(),
		back_inserter(argv));

	argv.insert(argv.end(), target.GetCFlags().begin(), target.GetCFlags().end());

	argv.push_back("-pipe");
	argv.push_back("-c");
	argv.push_back("-o");
	argv.push_back(GetObjectPathForFile(inFile).string());
	argv.push_back("-MD");
	
	if (target.IsAnsiStrict())
		argv.push_back("-ansi");
	
	if (target.IsPedantic())
		argv.push_back("-pedantic");
		
	if (target.GetDebugFlag())
		argv.push_back("-gdwarf-2");
	
//	argv.push_back("-fmessage-length=132");
	
	for (vector<MPath>::const_iterator p = mUserSearchPaths.begin(); p != mUserSearchPaths.end(); ++p)
	{
		if (fs::exists(*p) and fs::is_directory(*p))
			argv.push_back(kIQuote + p->string());
	}

	for (vector<MPath>::const_iterator p = mSysSearchPaths.begin(); p != mSysSearchPaths.end(); ++p)
	{
		if (fs::exists(*p) and fs::is_directory(*p))
			argv.push_back(kI + p->string());
	}
	
	for (vector<MPath>::const_iterator p = mFrameworkPaths.begin(); p != mFrameworkPaths.end(); ++p)
	{
		if (fs::exists(*p) and fs::is_directory(*p))
			argv.push_back(kF + p->string());
	}
	
	argv.push_back(inFile.string());

	return new MProjectCompileJob(
		string("Compiling ") + inFile.leaf(), this, argv, file);
}

// ---------------------------------------------------------------------------
//	MProject::CreateCompileAllJob

MProjectJob* MProject::CreateCompileAllJob()
{
	auto_ptr<MProjectCompileAllJob> job(new MProjectCompileAllJob("Compiling",  this));
	
	vector<MProjectItem*> files;
	mProjectItems.Flatten(files);
	
	for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
	{
		if ((*file)->IsCompilable() and (*file)->IsOutOfDate())
			job->AddJob(CreateCompileJob(static_cast<MProjectFile*>(*file)->GetPath()));
	}

	files.clear();
	mPackageItems.Flatten(files);

	vector<MPath> rsrcFiles;

	for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
	{		
		if ((*file)->IsCompilable() and (*file)->IsOutOfDate())
		{
			MProjectFile& f(dynamic_cast<MProjectFile&>(**file));
			rsrcFiles.push_back(f.GetPath());
		}
	}
	
	if (rsrcFiles.size() > 0)
	{
		MTargetCPU arch = mCurrentTarget->GetTargetCPU();
		if (arch == eCPU_native)
			arch = MProjectTarget::GetNativeCPU();
		
		job->AddJob(new MProjectCreateResourceJob(
			"Creating resources",
			this, mResourcesDir, rsrcFiles,
			mObjectDir / "__rsrc__.o",
			arch));
	}

	return job.release();
}

// ---------------------------------------------------------------------------
//	MProject::CreateLinkJob

MProjectJob* MProject::CreateLinkJob(
	const MPath&		inLinkerOutput)
{
	CheckDataDir();
	
	assert(mCurrentTarget != nil);
	MProjectTarget& target = *mCurrentTarget;
	
	vector<string> argv;
	
	argv.push_back(Preferences::GetString("c++", "c++"));

//	argv.push_back("-arch");
//	if (target.GetArch() == eTargetArchPPC_32)
//		argv.push_back("ppc");
//	else if (target.GetArch() == eTargetArchx86_32)
//		argv.push_back("i386");
//	else
//		assert(false);

	argv.insert(argv.end(), target.GetLDFlags().begin(), target.GetLDFlags().end());

	switch (mCurrentTarget->GetKind())
	{
		case eTargetSharedLibrary:
			argv.push_back("-bundle");
			argv.push_back("-undefined");
			argv.push_back("suppress");
			argv.push_back("-flat_namespace");
			break;
		
		case eTargetStaticLibrary:
			argv.push_back("-r");
			break;
		
		default:
			break;
	}

	for (vector<string>::const_iterator f = target.GetFrameworks().begin(); f != target.GetFrameworks().end(); ++f)
	{
		argv.push_back("-framework");
		argv.push_back(*f);
	}

	argv.push_back("-o");
	argv.push_back(inLinkerOutput.string());

#if not defined(__APPLE__)
	argv.push_back("-pthread");
#endif

	if (target.GetDebugFlag())
		argv.push_back("-gdwarf-2");

	for (vector<MPath>::const_iterator p = mLibSearchPaths.begin(); p != mLibSearchPaths.end(); ++p)
	{
		if (fs::exists(*p) and fs::is_directory(*p))
			argv.push_back(kL + p->string());
	}
	
	vector<MProjectItem*> files;
	mProjectItems.Flatten(files);

	for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
	{
		MProjectFile* f = dynamic_cast<MProjectFile*>(*file);
		
		if (f != nil)
		{
			if (f->IsCompilable())
				argv.push_back(f->GetObjectPath().string());
			else if (FileNameMatches("*.a;*.o;*.dylib", f->GetName()))
			{
				if (fs::exists(f->GetPath()) )
					argv.push_back(f->GetPath().string());
				else if (not f->IsOptional())
					THROW(("Missing library file %s", f->GetName().c_str()));
			}
			
			continue;
		}
		
		MProjectLib* l = dynamic_cast<MProjectLib*>(*file);

		if (l != nil)
			argv.push_back(kl + l->GetName());
	}

	files.clear();
	mPackageItems.Flatten(files);

	for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
	{
		MProjectFile* f = dynamic_cast<MProjectFile*>(*file);
		if (f != nil and f->IsCompilable())
		{
			MPath p(mObjectDir / "__rsrc__.o");
			argv.push_back(p.string());
			break;
		}
	}

	copy(mPkgConfigLibs.begin(), mPkgConfigLibs.end(), back_inserter(argv));
	
	return new MProjectExecJob("Linking", this, argv);
}

// ---------------------------------------------------------------------------
//	MProject::Preprocess

void MProject::Preprocess(
	const MPath&	inFile)
{
	CheckDataDir();
	
	MProjectFile* file = GetProjectFileForPath(inFile);
	if (file == nil)
		THROW(("File is not part of the target project"));

	assert(mCurrentTarget != nil);
	MProjectTarget& target = *mCurrentTarget;
	
	vector<string> argv;
	
	argv.push_back(Preferences::GetString("c++", "c++"));

	transform(target.GetDefines().begin(), target.GetDefines().end(),
		back_inserter(argv), bind1st(plus<string>(), "-D"));

	transform(target.GetWarnings().begin(), target.GetWarnings().end(),
		back_inserter(argv), bind1st(plus<string>(), "-W"));

	copy(mPkgConfigCFlags.begin(), mPkgConfigCFlags.end(),
		back_inserter(argv));

	argv.insert(argv.end(), target.GetCFlags().begin(), target.GetCFlags().end());

	argv.push_back("-E");
	
	if (target.IsAnsiStrict())
		argv.push_back("-ansi");
	
	if (target.IsPedantic())
		argv.push_back("-pedantic");
	
	argv.push_back("-fmessage-length=132");
	
	for (vector<MPath>::const_iterator p = mUserSearchPaths.begin(); p != mUserSearchPaths.end(); ++p)
		argv.push_back(kIQuote + p->string());

	for (vector<MPath>::const_iterator p = mSysSearchPaths.begin(); p != mSysSearchPaths.end(); ++p)
		argv.push_back(kI + p->string());
	
	for (vector<MPath>::const_iterator p = mFrameworkPaths.begin(); p != mFrameworkPaths.end(); ++p)
		argv.push_back(kF + p->string());
	
	argv.push_back(inFile.string());

	MProjectExecJob* job = new MProjectCompileJob(
		string("Preprocessing ") + inFile.leaf(), this, argv, file);
	
	MDocument* output = new MDocument(nil);
	output->SetFileNameHint(inFile.leaf() + " # preprocessed");
	
	SetCallBack(job->eStdOut, output, &MDocument::StdOut);
	MDocWindow::DisplayDocument(output);

	StartJob(job);
}

// ---------------------------------------------------------------------------
//	MProject::Disassemble

void MProject::Disassemble(
	const MPath&	inFile)
{
	CheckDataDir();
	
	MProjectFile* file = GetProjectFileForPath(inFile);
	if (file == nil)
		THROW(("File is not part of the target project"));

	assert(mCurrentTarget != nil);
	MProjectTarget& target = *mCurrentTarget;
	
	vector<string> argv;
	
	argv.push_back(Preferences::GetString("c++", "c++"));

	transform(target.GetDefines().begin(), target.GetDefines().end(),
		back_inserter(argv), bind1st(plus<string>(), "-D"));

	transform(target.GetWarnings().begin(), target.GetWarnings().end(),
		back_inserter(argv), bind1st(plus<string>(), "-W"));

	copy(mPkgConfigCFlags.begin(), mPkgConfigCFlags.end(),
		back_inserter(argv));

	argv.insert(argv.end(), target.GetCFlags().begin(), target.GetCFlags().end());

	argv.push_back("-S");
	argv.push_back("-o");
	argv.push_back("-");
	
	if (target.IsAnsiStrict())
		argv.push_back("-ansi");
	
	if (target.IsPedantic())
		argv.push_back("-pedantic");
	
	argv.push_back("-fmessage-length=132");
	
	for (vector<MPath>::const_iterator p = mUserSearchPaths.begin(); p != mUserSearchPaths.end(); ++p)
		argv.push_back(kIQuote + p->string());

	for (vector<MPath>::const_iterator p = mSysSearchPaths.begin(); p != mSysSearchPaths.end(); ++p)
		argv.push_back(kI + p->string());
	
	for (vector<MPath>::const_iterator p = mFrameworkPaths.begin(); p != mFrameworkPaths.end(); ++p)
		argv.push_back(kF + p->string());
	
	argv.push_back(inFile.string());

	MProjectExecJob* job = new MProjectCompileJob(
		string("Disassembling ") + inFile.leaf(), this, argv, file);
	
	MDocument* output = new MDocument(nil);
	output->SetFileNameHint(inFile.leaf() + " # disassembled");
	
	SetCallBack(job->eStdOut, output, &MDocument::StdOut);
	MDocWindow::DisplayDocument(output);

	StartJob(job);
}

// ---------------------------------------------------------------------------
//	MProject::CheckSyntax

void MProject::CheckSyntax(
	const MPath&	inFile)
{
	CheckDataDir();
	
	MProjectFile* file = GetProjectFileForPath(inFile);
	if (file == nil)
		THROW(("File is not part of the target project"));

	assert(mCurrentTarget != nil);
	MProjectTarget& target = *mCurrentTarget;
	
	vector<string> argv;
	
	argv.push_back(Preferences::GetString("c++", "c++"));

	transform(target.GetDefines().begin(), target.GetDefines().end(),
		back_inserter(argv), bind1st(plus<string>(), "-D"));

	transform(target.GetWarnings().begin(), target.GetWarnings().end(),
		back_inserter(argv), bind1st(plus<string>(), "-W"));

	argv.insert(argv.end(), target.GetCFlags().begin(), target.GetCFlags().end());

	argv.push_back("-c");
	argv.push_back("-o");
	argv.push_back("/dev/null");
	
	if (target.IsAnsiStrict())
		argv.push_back("-ansi");
	
	if (target.IsPedantic())
		argv.push_back("-pedantic");
	
	argv.push_back("-fmessage-length=132");
	
	for (vector<MPath>::const_iterator p = mUserSearchPaths.begin(); p != mUserSearchPaths.end(); ++p)
		argv.push_back(kIQuote + p->string());

	for (vector<MPath>::const_iterator p = mSysSearchPaths.begin(); p != mSysSearchPaths.end(); ++p)
		argv.push_back(kI + p->string());
	
	for (vector<MPath>::const_iterator p = mFrameworkPaths.begin(); p != mFrameworkPaths.end(); ++p)
		argv.push_back(kF + p->string());
	
	argv.push_back(inFile.string());

	StartJob(new MProjectCompileJob(
		string("Checking syntax of ") + inFile.leaf(), this, argv, file));
}

// ---------------------------------------------------------------------------
//	MProject::Compile

void MProject::Compile(
	const MPath&	inFile)
{
	StartJob(CreateCompileJob(inFile));
}

// ---------------------------------------------------------------------------
//	MProject::MakeClean

void MProject::MakeClean()
{
	switch (DisplayAlert("make-clean-alert"))
	{
		case 1:
			break;
		
		case 2:
			mProjectItems.SetOutOfDate(true);
			fs::remove_all(mProjectDataDir);
			break;
		
		case 3:
			mProjectItems.SetOutOfDate(true);
			fs::remove_all(mObjectDir);
			break;
	}
	
	CheckDataDir();
}

// ---------------------------------------------------------------------------
//	MProject::BringUpToDate

void MProject::BringUpToDate()
{
	StartJob(CreateCompileAllJob());
}

// ---------------------------------------------------------------------------
//	MProject::Make

void MProject::Make()
{
	auto_ptr<MProjectJob> job(CreateCompileAllJob());

	MPath targetPath;

	switch (mCurrentTarget->GetKind())
	{
		case eTargetApplicationPackage:
		{
			MPath macOsDir = mProjectDir / mCurrentTarget->GetBundleName() / "Contents" / "MacOS";
			
			if (not exists(macOsDir))
				fs::create_directories(macOsDir);
			
			targetPath = macOsDir / mCurrentTarget->GetLinkTarget();
						
			string pkginfo = mCurrentTarget->GetType() + mCurrentTarget->GetCreator();
			if (pkginfo.length() == 8)
			{
				fs::ofstream pkgInfoFile(macOsDir / ".." / "PkgInfo", ios_base::trunc);
				if (pkgInfoFile.is_open())
					pkgInfoFile.write(pkginfo.c_str(), 8);
			}
			break;
		}
		
		case eTargetSharedLibrary:
			targetPath = mProjectDir / mCurrentTarget->GetBundleName();
			break;
		
		case eTargetStaticLibrary:
			targetPath = mProjectDir / (mCurrentTarget->GetLinkTarget() + ".a");
			break;
		
		case eTargetExecutable:
			targetPath = mProjectDir / mCurrentTarget->GetLinkTarget();
			break;
		
		default:
			THROW(("Unsupported target kind"));
	}

	// combine with link job
	job.reset(new MProjectIfJob("", this, job.release(), CreateLinkJob(targetPath)));

//	if (mCurrentTarget->GetKind() == eTargetApplicationPackage and mPackageItems.Count() > 0)
//	{
//		// and a copy job for the Package contents, if needed
//		
//		auto_ptr<MProjectCompileAllJob> copyAllJob(new MProjectCompileAllJob("Copying", this));
//		MPath appPath = mProjectDir / mCurrentTarget->GetBundleName();
//
//		vector<MProjectItem*> files;
//		mPackageItems.Flatten(files);
//		
//		for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
//		{
//			MProjectResource* f = dynamic_cast<MProjectResource*>(*file);
//			
//			if (f == nil)
//				continue;
//
//			MPath dstDir = f->GetDestPath(appPath).branch_path();
//			
//			if (not exists(dstDir))
//				fs::create_directories(dstDir);
//			
//			copyAllJob->AddJob(new MProjectCopyFileJob("Copying files", this,
//				f->GetSourcePath(), f->GetDestPath(appPath), true));
//		}
//
//		job.reset(new MProjectIfJob("", this, job.release(), copyAllJob.release()));
//	}
	
	// and that's it for now
	
	StartJob(job.release());
}

// ---------------------------------------------------------------------------
//	MProject::MsgWindowClosed

void MProject::MsgWindowClosed(
	MWindow*		inWindow)
{
	assert(inWindow == mStdErrWindow.get());
	mStdErrWindow.release();
}

// ---------------------------------------------------------------------------
//	MProject::GetMessageWindow

MMessageWindow* MProject::GetMessageWindow()
{
	if (mStdErrWindow.get() == nil)
	{
		mStdErrWindow.reset(new MMessageWindow(FormatString("Build messages for ^0", mName)));
		AddRoute(mStdErrWindow->eWindowClosed, eMsgWindowClosed);
	}
				
	return mStdErrWindow.get();
}

// ---------------------------------------------------------------------------
//	MProject::UpdateList

void MProject::UpdateList()
{
	switch (mPanel)
	{
		case ePanelFiles:
		{
			vector<MProjectItem*> files;
			mProjectItems.Flatten(files);
		
			mFileList->RemoveAll();
		
			for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
			{
				MProjectItem* item = *file;
				mFileList->InsertItem(kListItemLast, &item, sizeof(item));
			}
			break;
		}
		
		case ePanelLinkOrder:
		{
			vector<MProjectItem*> files;
			mProjectItems.Flatten(files);
			
			mLinkOrderList->RemoveAll();
		
			for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
			{
				MProjectFile* f = dynamic_cast<MProjectFile*>(*file);
				if (f != nil)
					mLinkOrderList->InsertItem(kListItemLast, &f, sizeof(f));
			}
			break;
		}

		case ePanelPackage:
		{
			vector<MProjectItem*> files;
			mPackageItems.Flatten(files);
		
			mPackageList->RemoveAll();
			
			for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
			{
				MProjectItem* item = *file;
				mPackageList->InsertItem(kListItemLast, &item, sizeof(item));
			}
			break;
		}
		
		default:
			break;
	}
}

// ---------------------------------------------------------------------------
//	MProject::SelectTarget

void MProject::SelectTarget(
	uint32	inTarget)
{
	if (inTarget >= mTargets.size())
		inTarget = 0;
	
	assert(inTarget < mTargets.size());

	if (mCurrentTarget == mTargets[inTarget])
		return;
	
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(mTargetPopup)) != int32(inTarget))
		gtk_combo_box_set_active(GTK_COMBO_BOX(mTargetPopup), inTarget);

	mCurrentTarget = mTargets[inTarget];

	CheckDataDir();	// set up all directory paths
	
	MModDateCache modDateCache;
	
	SetStatus("Checking modification dates", true);
	
	GetCompilerPaths("c++", mCppIncludeDir, mCLibSearchPaths);

	mPkgConfigCFlags.clear();
	mPkgConfigLibs.clear();
	
	for (vector<string>::iterator pkg = mPkgConfigPkgs.begin(); pkg != mPkgConfigPkgs.end(); ++pkg)
	{
		GetPkgConfigResult(*pkg, "--cflags", mPkgConfigCFlags);
		GetPkgConfigResult(*pkg, "--libs", mPkgConfigLibs);
	}
	
	ResearchForFiles();
	
	mProjectItems.UpdatePaths(mObjectDir);
	mProjectItems.CheckCompilationResult();
	mProjectItems.CheckIsOutOfDate(modDateCache);
	
	mPackageItems.UpdatePaths(mObjectDir);
	mPackageItems.CheckCompilationResult();
	mPackageItems.CheckIsOutOfDate(modDateCache);
	
	SetStatus("", false);
}

// ---------------------------------------------------------------------------
//	MProject::TargetSelected

void MProject::TargetSelected()
{
	int32 target = gtk_combo_box_get_active(GTK_COMBO_BOX(mTargetPopup));
	
	if (target >= 0)
		SelectTarget(target);
}

// ---------------------------------------------------------------------------
//	MProject::SelectPanel

void MProject::SelectPanel(
	MProjectListPanel	inPanel)
{
	mPanel = inPanel;

//	switch (mPanel)
//	{
//		case ePanelFiles:
//            ::SetControlVisibility(mFilePanelRef, true, true);
//            ::SetControlVisibility(mLinkOrderPanelRef, false, false);
//            ::SetControlVisibility(mPackagePanelRef, false, false);
//			break;
//
//		case ePanelLinkOrder:
//            ::SetControlVisibility(mFilePanelRef, false, false);
//            ::SetControlVisibility(mLinkOrderPanelRef, true, true);
//            ::SetControlVisibility(mPackagePanelRef, false, false);
//			break;
//
//		case ePanelPackage:
//            ::SetControlVisibility(mFilePanelRef, false, false);
//            ::SetControlVisibility(mLinkOrderPanelRef, false, false);
//            ::SetControlVisibility(mPackagePanelRef, true, true);
//			break;
//
//		default:
//			break;
//	}
	
	UpdateList();
}

// ---------------------------------------------------------------------------
//	MProject::CheckIsOutOfDate

void MProject::CheckIsOutOfDate()
{
	try
	{
		MModDateCache modDateCache;
		mProjectItems.CheckIsOutOfDate(modDateCache);
		mPackageItems.CheckIsOutOfDate(modDateCache);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

// ---------------------------------------------------------------------------
//	MProject::ResearchForFiles

void MProject::ResearchForFiles()
{
	vector<MProjectItem*> files;
	mProjectItems.Flatten(files);
	
	for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
	{
		if (dynamic_cast<MProjectFile*>(*file) == nil)
			continue;
		
		MProjectFile* f = static_cast<MProjectFile*>(*file);
		MPath filePath;
		
		if (LocateFile(f->GetName(), true, filePath))
			f->SetParentDir(filePath.branch_path());
		else
			f->SetParentDir(MPath());
	}
	
	CheckIsOutOfDate();
}

// ---------------------------------------------------------------------------
//	MProject::ResearchForFiles

MProjectItem* MProject::GetItem(
	int32				inItemNr)
{
	MListView* listView = nil;
	switch (mPanel)
	{
		case ePanelFiles:		listView = mFileList;		break;
		case ePanelLinkOrder:	listView = mLinkOrderList;	break;
		case ePanelPackage:		listView = mPackageList;	break;
		default:											break;
	}
	
	if (inItemNr < 0 or inItemNr >= static_cast<int32>(listView->GetCount()))
		THROW(("Item number out of range"));
	
	MProjectItem* item = nil;
	listView->GetItem(inItemNr, &item, sizeof(item));
	
	if (item == nil)
		THROW(("Item is nil"));
	
	return item;
}
