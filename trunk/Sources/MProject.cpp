//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <sstream>
#include <limits>

#undef check
#ifndef BOOST_DISABLE_ASSERTS
#define BOOST_DISABLE_ASSERTS
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#define foreach BOOST_FOREACH

#include <zeep/xml/document.hpp>

#include "MProject.h"
#include "MDevice.h"
#include "MGlobals.h"
#include "MCommands.h"
#include "MMessageWindow.h"
#include "MUtils.h"
#include "MSound.h"
#include "MProjectItem.h"
#include "MProjectJob.h"
#include "MProjectInfoDialog.h"
#include "MFindAndOpenDialog.h"
#include "MPkgConfig.h"
#include "MStrings.h"
#include "MAlerts.h"
#include "MPreferences.h"
#include "MError.h"
#include "MTextDocument.h"
#include "MJapiApp.h"

using namespace std;
namespace xml = zeep::xml;
namespace ba = boost::algorithm;

namespace
{

const string
	kIQuote("-iquote"),
	kI("-I"),
	kF("-F"),
	kl("-l"),
	kL("-L");

MTargetCPU GetNativeCPU()
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

}

#pragma mark -

// ---------------------------------------------------------------------------
//	MProject

MProject* MProject::Instance()
{
	MDocument* doc = GetFirstDocument();
	
	while (doc != nil and dynamic_cast<MProject*>(doc) == nil)
		doc = doc->GetNextDocument();
	
	return dynamic_cast<MProject*>(doc);
}

MProject::MProject(
	const MFile&		inProjectFile)
	: MDocument(inProjectFile)
	, eProjectItemMoved(this, &MProject::ProjectItemMoved)
	, eMsgWindowClosed(this, &MProject::MsgWindowClosed)
	, ePoll(this, &MProject::Poll)
	, mProjectFile(inProjectFile.GetPath())
	, mProjectDir(mProjectFile.parent_path())
	, mProjectItems("", nil)
	, mPackageItems("", nil)
	, mStdErrWindow(nil)
	, mAllowWindows(true)
	, mCurrentTarget(numeric_limits<uint32>::max())	// force an update at first 
	, mCurrentJob(nil)
{
	if (gApp != nil)
		AddRoute(gApp->eIdle, ePoll);
}

// ---------------------------------------------------------------------------
//	MProject::~MProject

MProject::~MProject()
{
	if (mStdErrWindow != nil)
		mStdErrWindow->Close();
}

// ---------------------------------------------------------------------------
//	MProject::ReadFile

void MProject::ReadFile(
	istream&		inFile)
{
	if (fs::extension(mProjectFile) == ".prj")
		mName = fs::basename(mProjectFile);
	else
		mName = mProjectFile.filename();
	
	xml::document doc(inFile);
	Read(doc.child());
}

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
	const fs::path&		inPath) const
{
	return GetProjectFileForPath(inPath) != nil;
}

// ---------------------------------------------------------------------------
//	MProject::RecheckFiles

void MProject::RecheckFiles()
{
	MDocument* doc = GetFirstDocument();
	
	while (doc != nil)
	{
		if (dynamic_cast<MProject*>(doc) != nil)
			static_cast<MProject*>(doc)->CheckIsOutOfDate();
		
		doc = doc->GetNextDocument();
	}
}

// ---------------------------------------------------------------------------
//	MProject::ReadPaths

void MProject::ReadPaths(
	const xml::element_set&	inData,
	vector<fs::path>&		outPaths)
{
	foreach (const xml::element* node, inData)
	{
		string path(node->content());
		if (path.empty())
			continue;
//			THROW(("Invalid project file, missing path"));
		
		outPaths.push_back(path);
	}
}

// ---------------------------------------------------------------------------
//	MProject::ReadOptions

void MProject::ReadOptions(
	const xml::element_set&	inData,
	vector<string>&			outOptions)
{
	foreach (const xml::element* option, inData)
	{
		if (option->content().empty())
			continue;
		
		outOptions.push_back(option->content());
	}
}

// ---------------------------------------------------------------------------
//	MProject::ReadResources

void MProject::ReadResources(
	const xml::element*		inData,
	MProjectGroup*			inGroup)
{
	foreach (const xml::element* node, inData->children<xml::element>())
	{		
		if (node->qname() == "resource")
		{
			string fileName = node->content();
			if (fileName.length() == 0)
				THROW(("Invalid project file"));
			
			try
			{
				fs::path p(fileName);
				
				unique_ptr<MProjectResource> projectFile(
					new MProjectResource(p.filename(), inGroup,
						mProjectDir / mProjectInfo.mResourcesDir / p.parent_path()));

				inGroup->AddProjectItem(projectFile.release());
			}
			catch (std::exception& e)
			{
				DisplayError(e);
			}
		}
		else if (node->qname() == "group")
		{
			string name = node->get_attribute("name");
			
			unique_ptr<MProjectGroup> group(new MProjectGroup(name, inGroup));
			
			ReadResources(node, group.get());
			
			inGroup->AddProjectItem(group.release());
		}
	}
}

// ---------------------------------------------------------------------------
//	MProject::ReadFiles

void MProject::ReadFiles(
	const xml::element*	inData,
	MProjectGroup*		inGroup)
{
	foreach (const xml::element* node, inData->children<xml::element>())
	{
		if (node->qname() == "file")
		{
			string fileName = node->content();
			if (fileName.length() == 0)
				THROW(("Invalid project file"));
			
			if (mVersion == 1.0f and FileNameMatches("lib*.a;lib*.so;lib*.dylib", fileName))
			{
				bool shared = true;
				
				string linkName = fileName.substr(3);
				if (ba::ends_with(linkName, ".a"))
				{
					linkName.erase(linkName.end() - 2, linkName.end());
					shared = false;
				}
				else if (ba::ends_with(linkName, ".so"))
					linkName.erase(linkName.end() - 3, linkName.end());
				else
					linkName.erase(linkName.end() - 6, linkName.end());
				
				inGroup->AddProjectItem(new MProjectLib(linkName,
					shared,
					node->get_attribute("optional") == "true",
					inGroup));
			}
			else
			{
				fs::path filePath;
				try
				{
					unique_ptr<MProjectFile> projectFile;
					
					if (LocateFile(fileName, true, filePath))
						projectFile.reset(new MProjectFile(fileName, inGroup, filePath.parent_path()));
					else
						projectFile.reset(new MProjectFile(fileName, inGroup, fs::path()));
			
	//				AddRoute(projectFile->eStatusChanged, eProjectFileStatusChanged);
					inGroup->AddProjectItem(projectFile.release());
				}
				catch (std::exception& e)
				{
					DisplayError(e);
				}
			}
		}
		else if (node->qname() == "link")
		{
			string linkName = node->content();
			if (linkName.length() == 0)
				THROW(("Invalid project file"));
			
			inGroup->AddProjectItem(new MProjectLib(linkName,
				node->get_attribute("shared") == "true",
				node->get_attribute("optional") == "true",
				inGroup));
		}
		else if (node->qname() == "group")
		{
			string name = node->get_attribute("name");
			
			unique_ptr<MProjectGroup> group(new MProjectGroup(name, inGroup));
			
			ReadFiles(node, group.get());
			
			inGroup->AddProjectItem(group.release());
		}
	}
}

// ---------------------------------------------------------------------------
//	MProject::Read

void MProject::Read(
	const xml::element*	inRoot)
{
	mVersion = 1.0;

	try
	{
		mVersion = boost::lexical_cast<float>(inRoot->get_attribute("version"));
		if (mVersion < 1)
			mVersion = 1.0f;
	}
	catch (...) {}
	
	// read the pkg-config data
	
	xml::element_set data = inRoot->find("pkg-config/pkg");

	foreach (xml::element* node, data)
	{
		string tool = node->get_attribute("tool");
		if (tool.length() == 0)
			tool = "pkg-config";
				
		string pkg = node->content();
		mProjectInfo.mPkgConfigPkgs.push_back(tool + ':' + pkg);
	}
	
	// read the system search paths
	
	data = inRoot->find("syspaths/path");
	if (not data.empty())
		ReadPaths(data, mProjectInfo.mSysSearchPaths);
	
	// next the user search paths
	data = inRoot->find("userpaths/path");
	if (not data.empty())
		ReadPaths(data, mProjectInfo.mUserSearchPaths);
	
	// then the lib search paths
	data = inRoot->find("libpaths/path");
	if (not data.empty())
		ReadPaths(data, mProjectInfo.mLibSearchPaths);
	
	// now we're ready to read the files
	xml::element* files = inRoot->find_first("files");
	if (files != nil)
		ReadFiles(files, &mProjectItems);

	// and the package actions, if any
	xml::element* resources = inRoot->find_first("resources");
	if (resources != nil)
	{
		mProjectInfo.mAddResources = true;
		
		string rd = resources->get_attribute("resource-dir");
		if (rd.length() > 0)
			mProjectInfo.mResourcesDir = rd;
		else
			mProjectInfo.mResourcesDir = "Resources";

		ReadResources(resources, &mPackageItems);
	}
	else
		mProjectInfo.mAddResources = false;
	
	// And finally we add the targets
	data = inRoot->find("targets/target");
	foreach (xml::element* targetnode, data)
	{
		string linkTarget = targetnode->get_attribute("linkTarget");
		string name = targetnode->get_attribute("name");
		
		string p = targetnode->get_attribute("kind");
		if (p.length() == 0)
			THROW(("Invalid target, missing kind"));
		
		MTargetKind kind;
		if (p == "Shared Library")
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

		p = targetnode->get_attribute("arch");
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
		
		MProjectTarget target;
		target.mLinkTarget = linkTarget;
		target.mName = name;
		target.mKind = kind;
		target.mTargetCPU = arch;
		target.mCompiler = Preferences::GetString("c++", "/usr/bin/c++");
		
		p = targetnode->get_attribute("debug");
		if (p == "true")
			target.mCFlags.push_back("-gdwarf-2");
		
		p = targetnode->get_attribute("pic");
		if (p == "true")
			target.mCFlags.push_back("-fPIC");
		
		p = targetnode->get_attribute("profile");
		if (p == "true")
			target.mCFlags.push_back("-pg");
		
		ReadOptions(targetnode->find("defines/define"), target.mDefines);
		ReadOptions(targetnode->find("cflags/cflag"), target.mCFlags);
		ReadOptions(targetnode->find("ldflags/ldflag"), target.mLDFlags);
		ReadOptions(targetnode->find("warnings/warning"), target.mWarnings);
		
		foreach (xml::element* node, targetnode->children<xml::element>())
		{
			if (node->qname() == "name")
				target.mName = node->content();
//			else if (node->name() == "defines")
//				ReadOptions(node, "define", target.mDefines);
//			else if (node->name() == "cflags")
//				ReadOptions(node, "cflag", target.mCFlags);
//			else if (node->name() == "ldflags")
//				ReadOptions(node, "ldflag", target.mLDFlags);
//			else if (node->name() == "warnings")
//				ReadOptions(node, "warning", target.mWarnings);
			else if (node->qname() == "compiler")
				target.mCompiler = node->content();
			else if (node->qname() == "linkTarget")
				target.mLinkTarget = node->content();
		}
		
		mProjectInfo.mTargets.push_back(target);
	}

	SetModified(false);
}

// ---------------------------------------------------------------------------
//	MProject::Write routines
//
//	MProject::WritePaths

void MProject::WritePaths(
	xml::element*		inNode,
	const char*			inTag,
	vector<fs::path>&	inPaths,
	bool				inFullPath)
{
	xml::element* root(new xml::element(inTag));
	
	for (vector<fs::path>::iterator p = inPaths.begin(); p != inPaths.end(); ++p)
	{
		xml::element* path(new xml::element("path"));

		if (p->is_complete())
			path->content(p->string());
		else if (inFullPath)
			path->content(fs::system_complete(*p).string());
		else
			path->content(p->string());

		root->append(path);
	}
	
	inNode->append(root);
}

// ---------------------------------------------------------------------------
//	MProject::WriteFiles

void MProject::WriteFiles(
	xml::element*			inNode,
	vector<MProjectItem*>&	inItems)
{
	for (vector<MProjectItem*>::iterator item = inItems.begin(); item != inItems.end(); ++item)
	{
		if (dynamic_cast<MProjectGroup*>(*item) != nil)
		{
			MProjectGroup* group = static_cast<MProjectGroup*>(*item);

			xml::element* groupNode(new xml::element("group"));
			groupNode->set_attribute("name", group->GetName());
			WriteFiles(groupNode, group->GetItems());
			inNode->append(groupNode);
		}
		else if (dynamic_cast<MProjectLib*>(*item) != nil)
		{
			MProjectLib* link = dynamic_cast<MProjectLib*>(*item);

			xml::element* linkNode(new xml::element("link"));
			linkNode->content(link->GetName());

			if (link->IsOptional())
				linkNode->set_attribute("optional", "true");

			if (link->IsShared())
				linkNode->set_attribute("shared", "true");

			inNode->append(linkNode);
		}
		else
		{
			xml::element* fileNode(new xml::element("file"));
			fileNode->content((*item)->GetName());
			inNode->append(fileNode);
		}
	}
}

// ---------------------------------------------------------------------------
//	MProject::WriteResources

void MProject::WriteResources(
	xml::element*			inNode,
	vector<MProjectItem*>&	inItems)
{
	fs::path rsrcDir = mProjectDir / mProjectInfo.mResourcesDir;
	
	for (vector<MProjectItem*>::iterator item = inItems.begin(); item != inItems.end(); ++item)
	{
		if (dynamic_cast<MProjectGroup*>(*item) != nil)
		{
			MProjectGroup* group = static_cast<MProjectGroup*>(*item);
			
			xml::element* groupNode(new xml::element("group"));
			groupNode->set_attribute("name", group->GetName());
			
			WriteResources(groupNode, group->GetItems());
			
			inNode->append(groupNode);
		}
		else if (dynamic_cast<MProjectResource*>(*item) != nil)
		{
			fs::path path = static_cast<MProjectResource*>(*item)->GetPath();
			
			xml::element* resourceNode(new xml::element("resource"));
			resourceNode->content(relative_path(rsrcDir, path).string());

			inNode->append(resourceNode);
		}
	}
}

// ---------------------------------------------------------------------------
//	MProject::WriteTarget

void MProject::WriteTarget(
	xml::element*		inNode,
	MProjectTarget&		inTarget)
{
	// <target>
	xml::element* targetNode(new xml::element("target"));
	
	switch (inTarget.mKind)
	{
		case eTargetExecutable:
			targetNode->set_attribute("kind", "Executable");
			break;
		
		case eTargetStaticLibrary:
			targetNode->set_attribute("kind", "Static Library");
			break;
		
		case eTargetSharedLibrary:
			targetNode->set_attribute("kind", "Shared Library");
			break;
		
		default:
			break;
	}
		
	switch (inTarget.mTargetCPU)
	{
		case eCPU_PowerPC_32:
			targetNode->set_attribute("arch", "ppc");
			break;
		
		case eCPU_PowerPC_64:
			targetNode->set_attribute("arch", "ppc64");
			break;
		
		case eCPU_386:
			targetNode->set_attribute("arch", "i386");
			break;
		
		case eCPU_x86_64:
			targetNode->set_attribute("arch", "amd64");
			break;
		
		default:
			break;
	}
	
	xml::element* node(new xml::element("name"));
	node->content(inTarget.mName);
	targetNode->append(node);
	
	node = new xml::element("linkTarget");
	node->content(inTarget.mLinkTarget);
	targetNode->append(node);

	node = new xml::element("compiler");
	node->content(inTarget.mCompiler);
	targetNode->append(node);
	
	WriteOptions(targetNode, "defines", "define", inTarget.mDefines);
	WriteOptions(targetNode, "cflags", "cflag", inTarget.mCFlags);
	WriteOptions(targetNode, "ldflags", "ldflag", inTarget.mLDFlags);
	WriteOptions(targetNode, "warnings", "warning", inTarget.mWarnings);
	
	inNode->append(targetNode);
}

// ---------------------------------------------------------------------------
//	MProject::WriteOptions

void MProject::WriteOptions(
	xml::element*			inNode,
	const char*				inOptionGroupName,
	const char*				inOptionName,
	const vector<string>&	inOptions)
{
	if (not inOptions.empty())
	{
		// <optiongroup>
		xml::element* group(new xml::element(inOptionGroupName));
		
		for (vector<string>::const_iterator o = inOptions.begin(); o != inOptions.end(); ++o)
		{
			xml::element* option(new xml::element(inOptionName));
			option->content(*o);
			group->append(option);
		}
	
		// </optiongroup>
		inNode->append(group);
	}
}

// ---------------------------------------------------------------------------
//	MProject::WriteFile

void MProject::WriteFile(
	ostream&			inFile)
{
	// <project>
	xml::element* projectNode(new xml::element("project"));
	
	projectNode->set_attribute("version", "2.0");
		
	// pkg-config
	if (mProjectInfo.mPkgConfigPkgs.size() > 0)
	{
		xml::element* pkgConfigNode(new xml::element("pkg-config"));
		
		for (vector<string>::iterator p = mProjectInfo.mPkgConfigPkgs.begin(); p != mProjectInfo.mPkgConfigPkgs.end(); ++p)
		{
			xml::element* pkgNode(new xml::element("pkg"));
			
			vector<string> f;
			ba::split(f, *p, ba::is_any_of(":"));
			
			if (f.size() == 2)
			{
				pkgNode->set_attribute("tool", f[0]);
				pkgNode->content(f[1]);
			}
			else if (f[0] == "perl")
				pkgNode->set_attribute("tool", "perl");
			else
				pkgNode->content(f[0]);
			
			pkgConfigNode->append(pkgNode);
		}
		
		projectNode->append(pkgConfigNode);
	}
	
	WritePaths(projectNode, "syspaths", mProjectInfo.mSysSearchPaths, false);
	WritePaths(projectNode, "userpaths", mProjectInfo.mUserSearchPaths, false);
	WritePaths(projectNode, "libpaths", mProjectInfo.mLibSearchPaths, false);

	// <files>
	xml::element* filesNode(new xml::element("files"));
	WriteFiles(filesNode, mProjectItems.GetItems());
	projectNode->append(filesNode);
	
	// <resources>
	if (mProjectInfo.mAddResources)
	{
		xml::element* resourcesNode(new xml::element("resources"));

		if (fs::exists(mProjectDir / mProjectInfo.mResourcesDir))
			resourcesNode->set_attribute("resource-dir", mProjectInfo.mResourcesDir.string());

		WriteResources(resourcesNode, mPackageItems.GetItems());
		
		projectNode->append(resourcesNode);
	}

	// <targets>
	xml::element* targetsNode(new xml::element("targets"));
	for (vector<MProjectTarget>::iterator target = mProjectInfo.mTargets.begin(); target != mProjectInfo.mTargets.end(); ++target)
		WriteTarget(targetsNode, *target);
	projectNode->append(targetsNode);
	
	xml::document doc;
	doc.child(projectNode);
	inFile << doc;
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
	unique_ptr<MProjectJob> job(inJob);
	
	if (mCurrentJob.get() != nil)
		THROW(("Cannot start a job when another already runs"));
	
	swap(mCurrentJob, job);
	
	if (mStdErrWindow != nil)
	{
		mStdErrWindow->ClearList();
		mStdErrWindow->Hide();
	}

	mCurrentJob->Execute();
}

// ---------------------------------------------------------------------------
//	MProject::GetProjectFileForPath

MProjectFile* MProject::GetProjectFileForPath(
	const fs::path&		inPath) const
{
	return mProjectItems.GetProjectFileForPath(inPath);
}

// ---------------------------------------------------------------------------
//	MProject::GetProjectRsrcForPath

MProjectResource* MProject::GetProjectRsrcForPath(
	const fs::path&		inPath) const
{
	return mPackageItems.GetProjectResourceForPath(inPath);
}

// ---------------------------------------------------------------------------
//	MProject::LocateFile

bool MProject::LocateFile(
	const string&		inFile,
	bool				inSearchUserPaths,
	fs::path&			outPath) const
{
	bool found = false;

	// search library files in libpath	
	if (FileNameMatches("*.a;*.so;*.dylib", inFile))
	{
		for (vector<fs::path>::const_iterator p = mProjectInfo.mLibSearchPaths.begin();
			 not found and p != mProjectInfo.mLibSearchPaths.end();
			 ++p)
		{
			if (p->is_complete())
				outPath = *p / inFile;
			else
				outPath = mProjectDir / *p / inFile;

			found = exists(outPath);
		}
		
		if (not found)
		{
			for (vector<fs::path>::const_iterator p = mCLibSearchPaths.begin();
				 not found and p != mCLibSearchPaths.end();
				 ++p)
			{
				outPath = *p / inFile;
				found = exists(outPath);
			}
		}

		// special case for boost file names
		if (not found and FileNameMatches("libboost*.a;libboost*.so", inFile) and
			not Preferences::GetString("boost-ext", "-mt").empty())
		{
			string ext = Preferences::GetString("boost-ext", "-mt");
			
			string::size_type p = inFile.rfind('.');
			assert(p != string::npos);
			
			string file = inFile;
			file.insert(p, ext);

			for (vector<fs::path>::const_iterator p = mCLibSearchPaths.begin();
				 not found and p != mCLibSearchPaths.end();
				 ++p)
			{
				outPath = *p / file;
				found = exists(outPath);
			}
		}
	}
	else
	{
		if (inSearchUserPaths)
		{
			for (vector<fs::path>::const_iterator p = mProjectInfo.mUserSearchPaths.begin();
				 not found and p != mProjectInfo.mUserSearchPaths.end();
				 ++p)
			{
				if (p->is_complete())
					outPath = *p / inFile;
				else
					outPath = mProjectDir / *p / inFile;

				found = exists(outPath);
			}
		}

		for (vector<fs::path>::const_iterator p = mProjectInfo.mSysSearchPaths.begin();
			 not found and p != mProjectInfo.mSysSearchPaths.end();
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
		
		if (not found and fs::exists(mCppIncludeDir))
		{
			outPath = fs::path(mCppIncludeDir) / inFile;
			found = fs::exists(outPath);
		}
	}
	
	if (found)
		NormalizePath(outPath);
	
	return found;
}

// ---------------------------------------------------------------------------
//	MProject::GetIncludePaths

void MProject::GetIncludePaths(
	vector<fs::path>&	outPaths) const
{
	copy(mProjectInfo.mSysSearchPaths.begin(), mProjectInfo.mSysSearchPaths.end(), back_inserter(outPaths));

	for (vector<string>::const_iterator f = mPkgConfigCFlags.begin();
		 f != mPkgConfigCFlags.end();
		 ++f)
	{
		if (f->length() > 2 and f->substr(0, 2) == "-I")
			outPaths.push_back(fs::path(f->substr(2)));
	}

	for (vector<fs::path>::const_iterator p = mProjectInfo.mUserSearchPaths.begin();
		 p != mProjectInfo.mUserSearchPaths.end();
		 ++p)
	{
		if (p->is_complete())
			outPaths.push_back(*p);
		else
			outPaths.push_back(mProjectDir / *p);
	}
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
	
	mObjectDir = mProjectDataDir / (mProjectInfo.mTargets[mCurrentTarget].mName + "_Object");
	if (not exists(mObjectDir))
		fs::create_directory(mObjectDir);
	else if (not is_directory(mObjectDir))
		THROW(("Project data dir is not valid"));
	
	if (not mProjectInfo.mOutputDir.empty() and not fs::exists(mProjectInfo.mOutputDir))
	{
		fs::path outputDir;
		if (mProjectInfo.mOutputDir.is_complete())
			outputDir = mProjectInfo.mOutputDir;
		else
			outputDir = mProjectDir / mProjectInfo.mOutputDir;
		
		fs::create_directory(outputDir);
	}
}

// ---------------------------------------------------------------------------
//	MProject::GetObjectPathForFile

fs::path MProject::GetObjectPathForFile(
	const fs::path&	inFile) const
{
	MProjectFile* file = GetProjectFileForPath(inFile);
	
	if (file == nil)
		THROW(("File %s is not in project", inFile.string().c_str()));
	
	return file->GetObjectPath();
}

// ---------------------------------------------------------------------------
//	MProject::GenerateCFlags

void MProject::GenerateCFlags(
	vector<string>&	outArgv) const
{
	const MProjectTarget& target(mProjectInfo.mTargets[mCurrentTarget]);
	
	switch (target.mTargetCPU)
	{
		case eCPU_native:	break;
		case eCPU_386:		outArgv.push_back("-m32"); break;
		case eCPU_x86_64:	outArgv.push_back("-m64"); break;
		default:			THROW(("Unsupported CPU"));
	}

	transform(target.mDefines.begin(), target.mDefines.end(),
		back_inserter(outArgv), bind1st(plus<string>(), "-D"));

	transform(target.mWarnings.begin(), target.mWarnings.end(),
		back_inserter(outArgv), bind1st(plus<string>(), "-W"));

	copy(mPkgConfigCFlags.begin(), mPkgConfigCFlags.end(),
		back_inserter(outArgv));

	outArgv.insert(outArgv.end(), target.mCFlags.begin(), target.mCFlags.end());

	outArgv.push_back("-pipe");

//	if (target.IsAnsiStrict())
//		outArgv.push_back("-ansi");
//	
//	if (target.IsPedantic())
//		outArgv.push_back("-pedantic");
		
//	outArgv.push_back("-fmessage-length=132");
	
	for (vector<fs::path>::const_iterator p = mProjectInfo.mUserSearchPaths.begin(); p != mProjectInfo.mUserSearchPaths.end(); ++p)
	{
		fs::path path;
		if (p->is_complete())
			path = *p;
		else
			path = mProjectDir / *p;
		
		if (fs::exists(path) and fs::is_directory(path))
			outArgv.push_back(kIQuote + path.string());
	}

	for (vector<fs::path>::const_iterator p = mProjectInfo.mSysSearchPaths.begin(); p != mProjectInfo.mSysSearchPaths.end(); ++p)
	{
		fs::path path;
		if (p->is_complete())
			path = *p;
		else
			path = mProjectDir / *p;
		
		if (fs::exists(path) and fs::is_directory(path))
			outArgv.push_back(kI + path.string());
	}
}

// ---------------------------------------------------------------------------
//	MProject::CreateCompileJob

MProjectJob* MProject::CreateCompileJob(
	MProjectFile*		inFile)
{
	CheckDataDir();

	if (inFile->GetObjectPath().empty())
		ResearchForFiles();
	
	vector<string> argv;
	
	argv.push_back(mProjectInfo.mTargets[mCurrentTarget].mCompiler);

	GenerateCFlags(argv);

	argv.push_back("-c");
	argv.push_back("-o");
	argv.push_back(inFile->GetObjectPath().string());
	argv.push_back("-MD");
	
	argv.push_back(inFile->GetPath().string());

	MProjectExecJob* result = new MProjectCompileJob(
		string("Compiling ") + inFile->GetPath().filename(), this, argv, inFile);
	result->eStdErr.SetProc(this, &MProject::StdErrIn);
	return result;
}

// ---------------------------------------------------------------------------
//	MProject::CreateCompileAllJob

MProjectJob* MProject::CreateCompileAllJob()
{
	unique_ptr<MProjectDoAllJob> job(new MProjectDoAllJob("Compiling",  this));
	
	vector<MProjectItem*> files;
	mProjectItems.Flatten(files);
	
	for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
	{
		if ((*file)->IsCompilable() and (*file)->IsOutOfDate())
			job->AddJob(CreateCompileJob(static_cast<MProjectFile*>(*file)));
	}
	
	if (mProjectInfo.mAddResources)
	{
		files.clear();
		mPackageItems.Flatten(files);
	
		if (files.size() > 0)
		{
			MTargetCPU arch = mProjectInfo.mTargets[mCurrentTarget].mTargetCPU;
			if (arch == eCPU_native)
				arch = GetNativeCPU();
			
			job->AddJob(new MProjectCreateResourceJob(
				"Creating resources", this, files, mObjectDir / "__rsrc__.o", arch));
		}
	}

	return job.release();
}

// ---------------------------------------------------------------------------
//	MProject::CreateLinkJob

MProjectJob* MProject::CreateLinkJob(
	const fs::path&		inLinkerOutput)
{
	CheckDataDir();
	
	MProjectTarget& target = mProjectInfo.mTargets[mCurrentTarget];
	
	vector<string> argv;
	
	if (target.mKind == eTargetStaticLibrary)
		argv.push_back(Preferences::GetString("linker", "/usr/bin/ld"));
	else
		argv.push_back(mProjectInfo.mTargets[mCurrentTarget].mCompiler);

	argv.push_back("-o");
	argv.push_back(inLinkerOutput.string());

	switch (target.mTargetCPU)
	{
		case eCPU_native:	break;
		case eCPU_386:		argv.push_back("-m32"); break;
		case eCPU_x86_64:	argv.push_back("-m64"); break;
		default:			THROW(("Unsupported CPU"));
	}

	switch (target.mKind)
	{
		case eTargetSharedLibrary:
			argv.push_back("-shared");
//			argv.push_back("-static-libgcc");
			break;
		
		case eTargetStaticLibrary:
			argv.push_back("-r");
			break;
		
		default:
			break;
	}

	for (vector<fs::path>::const_iterator p = mProjectInfo.mLibSearchPaths.begin(); p != mProjectInfo.mLibSearchPaths.end(); ++p)
	{
		fs::path path;
		
		if (p->is_complete())
			path = *p;
		else
			path = mProjectDir / *p;
		
		if (fs::exists(path) and fs::is_directory(path))
			argv.push_back(kL + path.string());
	}
	
	argv.insert(argv.end(), target.mLDFlags.begin(), target.mLDFlags.end());
	// I think this is OK:
	if (target.mKind != eTargetStaticLibrary)
		argv.insert(argv.end(), target.mCFlags.begin(), target.mCFlags.end());

	vector<MProjectItem*> files;
	mProjectItems.Flatten(files);

	for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
	{
		MProjectFile* f = dynamic_cast<MProjectFile*>(*file);
		
		if (f != nil)
		{
			if (f->IsCompilable())
				argv.push_back(f->GetObjectPath().string());
			
			continue;
		}
		
		MProjectLib* l = dynamic_cast<MProjectLib*>(*file);

		if (l != nil)
		{
			fs::path path;
			if (LocateFile(l->GetLibraryName(), true, path))
			{
				if (l->IsShared())
					argv.push_back(kl + l->GetName());
				else
					argv.push_back(path.string());
			}
			else if (not l->IsOptional())
				THROW(("Could not locate library '%s'", l->GetLibraryName().c_str()));
		}
	}

	files.clear();
	mPackageItems.Flatten(files);

	for (vector<MProjectItem*>::iterator file = files.begin(); file != files.end(); ++file)
	{
		MProjectFile* f = dynamic_cast<MProjectFile*>(*file);
		if (f != nil and f->IsCompilable())
		{
			fs::path p(mObjectDir / "__rsrc__.o");
			argv.push_back(p.string());
			break;
		}
	}

	copy(mPkgConfigLibs.begin(), mPkgConfigLibs.end(), back_inserter(argv));
	
	MProjectExecJob* result = new MProjectExecJob("Linking", this, argv);
	result->eStdErr.SetProc(this, &MProject::StdErrIn);
	return result;
}

// ---------------------------------------------------------------------------
//	MProject::Preprocess

void MProject::Preprocess(
	const vector<MProjectFile*>&	inFiles)
{
	CheckDataDir();

	unique_ptr<MProjectDoAllJob> allJobs(new MProjectDoAllJob("Preprocessing",  this));
	
	for (auto file = inFiles.begin(); file != inFiles.end(); ++file)
		allJobs->AddJob(CreatePreprocessJob(*file));
	
	StartJob(allJobs.release());
}

MProjectJob* MProject::CreatePreprocessJob(
	MProjectFile*		inFile)
{
	vector<string> argv;
	
	fs::path path = inFile->GetPath();
	
	argv.push_back(mProjectInfo.mTargets[mCurrentTarget].mCompiler);

	GenerateCFlags(argv);

	argv.push_back("-E");
	argv.push_back(path.string());

	MProjectExecJob* job = new MProjectCompileJob(
		string("Preprocessing ") + path.filename(), this, argv, inFile);

	job->eStdErr.SetProc(this, &MProject::StdErrIn);
	
	MTextDocument* output = MDocument::Create<MTextDocument>(MFile());
	output->SetFileNameHint(path.filename() + " # preprocessed");
	
	SetCallback(job->eStdOut, output, &MTextDocument::StdOut);
	gApp->DisplayDocument(output);

	return job;
}

// ---------------------------------------------------------------------------
//	MProject::Disassemble

void MProject::Disassemble(
	const vector<MProjectFile*>&	inFiles)
{
	CheckDataDir();

	unique_ptr<MProjectDoAllJob> allJobs(new MProjectDoAllJob("Disassembling",  this));
	
	for (auto file = inFiles.begin(); file != inFiles.end(); ++file)
		allJobs->AddJob(CreateDisassembleJob(*file));

	StartJob(allJobs.release());
}

MProjectJob* MProject::CreateDisassembleJob(
	MProjectFile*		inFile)
{
	vector<string> argv;
	
	fs::path path = inFile->GetPath();
	
	argv.push_back(mProjectInfo.mTargets[mCurrentTarget].mCompiler);

	GenerateCFlags(argv);
	
	argv.push_back("-S");
	argv.push_back("-o");
	argv.push_back("-");
	
	argv.push_back(path.string());

	MProjectExecJob* job = new MProjectCompileJob(
		string("Disassembling ") + path.filename(), this, argv, inFile);

	job->eStdErr.SetProc(this, &MProject::StdErrIn);
	
	MTextDocument* output = MDocument::Create<MTextDocument>(MFile());
	output->SetFileNameHint(path.filename() + " # disassembled");
	
	SetCallback(job->eStdOut, output, &MTextDocument::StdOut);
	gApp->DisplayDocument(output);
	
	return job;
}

// ---------------------------------------------------------------------------
//	MProject::CheckSyntax

void MProject::CheckSyntax(
	const vector<MProjectFile*>&	inFiles)
{
	unique_ptr<MProjectDoAllJob> allJobs(new MProjectDoAllJob("Checking Syntax",  this));
	
	for (auto file = inFiles.begin(); file != inFiles.end(); ++file)
		allJobs->AddJob(CreateCheckSyntaxJob(*file));

	StartJob(allJobs.release());
}

MProjectJob* MProject::CreateCheckSyntaxJob(
	MProjectFile*	inFile)
{
	CheckDataDir();
	
	vector<string> argv;
	
	argv.push_back(mProjectInfo.mTargets[mCurrentTarget].mCompiler);

	GenerateCFlags(argv);

	argv.push_back("-o");
	argv.push_back("/dev/null");
	argv.push_back("-c");
	argv.push_back(inFile->GetPath().string());

	MProjectExecJob* job = new MProjectCompileJob(
		string("Checking syntax of ") + inFile->GetPath().filename(), this, argv, inFile);
	job->eStdErr.SetProc(this, &MProject::StdErrIn);
	
	return job;
}

// ---------------------------------------------------------------------------
//	MProject::Compile

void MProject::Compile(
	const vector<MProjectFile*>&	inFiles)
{
	unique_ptr<MProjectDoAllJob> allJobs(new MProjectDoAllJob("Compiling",  this));
	
	for (auto file = inFiles.begin(); file != inFiles.end(); ++file)
		allJobs->AddJob(CreateCompileJob(*file));

	StartJob(allJobs.release());
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

bool MProject::Make(
	bool		inUsePolling)
{
	unique_ptr<MProjectJob> job(CreateCompileAllJob());

	fs::path targetPath;
	fs::path outputDir;
	if (mProjectInfo.mOutputDir.empty())
		outputDir = mProjectDir;
	else if (mProjectInfo.mOutputDir.is_complete())
		outputDir = mProjectInfo.mOutputDir;
	else
		outputDir = mProjectDir / mProjectInfo.mOutputDir;

	switch (mProjectInfo.mTargets[mCurrentTarget].mKind)
	{
		case eTargetSharedLibrary:
			targetPath = outputDir / (mProjectInfo.mTargets[mCurrentTarget].mLinkTarget + ".so");
			break;
		
		case eTargetStaticLibrary:
			targetPath = outputDir / (mProjectInfo.mTargets[mCurrentTarget].mLinkTarget + ".a");
			break;
		
		case eTargetExecutable:
			targetPath = outputDir / mProjectInfo.mTargets[mCurrentTarget].mLinkTarget;
			break;

		default:
			THROW(("Unsupported target kind"));
	}

	// combine with link job
	job.reset(new MProjectIfJob("", this, job.release(), CreateLinkJob(targetPath)));

	// and that's it for now
	StartJob(job.release());
	
	bool result = true;
	if (inUsePolling == false)
	{
		mAllowWindows = false;
		while (mCurrentJob.get() != nil)
		{
			usleep(50000);

			if (mCurrentJob->IsDone())
			{
				result = mCurrentJob->mStatus == 0;
				mCurrentJob.reset(nil);
			}
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProject::MsgWindowClosed

void MProject::MsgWindowClosed(
	MWindow*		inWindow)
{
	assert(inWindow == mStdErrWindow);
	mStdErrWindow = nil;
}

// ---------------------------------------------------------------------------
//	MProject::GetMessageWindow

MMessageWindow* MProject::GetMessageWindow()
{
	if (mAllowWindows)
	{
		if (mStdErrWindow == nil)
		{
			mStdErrWindow = new MMessageWindow(FormatString("Build messages for ^0", mName));
			AddRoute(mStdErrWindow->eWindowClosed, eMsgWindowClosed);
		}
		else
			mStdErrWindow->Show();
	}
	
	return mStdErrWindow;
}

// ---------------------------------------------------------------------------
//	MProject::GetMessageWindow

void MProject::StdErrIn(
	const char*			inText,
	uint32				inLength)
{
	MMessageWindow* errWindow = GetMessageWindow();
	if (errWindow != nil)
		errWindow->AddStdErr(inText, inLength);
	else
		cerr.write(inText, inLength);
}

// ---------------------------------------------------------------------------
//	MProject::SelectTarget

void MProject::SelectTarget(
	const string&		inTarget)
{
	vector<MProjectTarget>::iterator target;

	for (target = mProjectInfo.mTargets.begin(); target != mProjectInfo.mTargets.end(); ++target)
	{
		if (inTarget == target->mName)
		{
			SelectTarget(target - mProjectInfo.mTargets.begin());
			break;
		}
	}
	
	if (target == mProjectInfo.mTargets.end())
		THROW(("Target not found: %s", inTarget.c_str()));
}

void MProject::SelectTarget(
	uint32				inTarget)
{
	if (inTarget >= mProjectInfo.mTargets.size())
		inTarget = 0;
	
	assert(inTarget < mProjectInfo.mTargets.size());

	if (mCurrentTarget == inTarget)
		return;

	mCurrentTarget = inTarget;

	CheckDataDir();	// set up all directory paths
	
	MModDateCache modDateCache;
	
	SetStatus("Checking modification dates", true);
	
	GetCompilerPaths(mProjectInfo.mTargets[mCurrentTarget].mCompiler, mCppIncludeDir, mCLibSearchPaths);

	mPkgConfigCFlags.clear();
	mPkgConfigLibs.clear();
	
	for (vector<string>::iterator pkg = mProjectInfo.mPkgConfigPkgs.begin(); pkg != mProjectInfo.mPkgConfigPkgs.end(); ++pkg)
	{
		vector<string> p;
		ba::split(p, *pkg, ba::is_any_of(":"));
		
		if (p[0] == "pkg-config")
		{
			GetPkgConfigResult(p[1], "--cflags", mPkgConfigCFlags);
			GetPkgConfigResult(p[1], "--libs", mPkgConfigLibs);
		}
		else if (p[0] == "perl")
		{
			const char* kCFlagsArgs[] = {
				"-MExtUtils::Embed",
				"-e",
				"perl_inc",
				nil
			};

			GetToolConfigResult("perl", kCFlagsArgs, mPkgConfigCFlags);

			const char* kLDFlagsArgs[] = {
				"-MExtUtils::Embed",
				"-e",
				"ldopts",
				nil
			};

			GetToolConfigResult("perl", kLDFlagsArgs, mPkgConfigLibs);
		}
	}
	
	ResearchForFiles();
	
	SetStatus("", false);
}

// ---------------------------------------------------------------------------
//	MProject::CheckIsOutOfDate

void MProject::CheckIsOutOfDate()
{
	try
	{
		MModDateCache modDateCache;

		mProjectItems.CheckCompilationResult();
		mProjectItems.CheckIsOutOfDate(modDateCache);

		mPackageItems.CheckCompilationResult();
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
		fs::path filePath;
		
		if (LocateFile(f->GetName(), true, filePath))
			f->SetParentDir(filePath.parent_path());
		else
			f->SetParentDir(fs::path());
	}
	
	mProjectItems.UpdatePaths(mObjectDir);
	
	CheckIsOutOfDate();
}

// ---------------------------------------------------------------------------
//	MProject::SetStatus

void MProject::SetStatus(
	const string&	inStatus,
	bool			inBusy)
{
	if (mAllowWindows)
		eStatus(inStatus, inBusy);
	else
		cout << inStatus << endl;
}

// ---------------------------------------------------------------------------
//	MProject::UpdateCommandStatus

bool MProject::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = true;
	
	outEnabled = false;
	
	switch (inCommand)
	{
		case cmd_RecheckFiles:
		case cmd_BringUpToDate:
		case cmd_MakeClean:
		case cmd_Make:
		case cmd_OpenIncludeFile:
			outEnabled = true;
			break;

		case cmd_Run:
			break;
		
		case cmd_Stop:
			outEnabled = mCurrentJob.get() != nil;
			break;
		
		default:
			result = MDocument::UpdateCommandStatus(
				inCommand, inMenu, inItemIndex, outEnabled, outChecked);
			break;
	}
	
	return result;
}

bool MProject::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;

	switch (inCommand)
	{
		case cmd_RecheckFiles:
			ResearchForFiles();
			CheckIsOutOfDate();
			break;
		
		case cmd_BringUpToDate:
			BringUpToDate();
			break;
		
		case cmd_MakeClean:
			MakeClean();
			break;
		
		case cmd_Make:
			Make();
			break;
		
		case cmd_Stop:
			StopBuilding();
			break;
		
		default:
			result = MDocument::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
			break;
	}
	
	return result;
}

MProjectItem* MProject::CreateFileItem(
	const fs::path&		inFile)
{
	if (GetProjectFileForPath(inFile))
		THROW(("File is already part of this project"));
	
	string name = inFile.filename();
	
	assert (not fs::is_directory(inFile));

	unique_ptr<MProjectItem> projectFile;
	fs::path filePath;
	
	if (LocateFile(name, true, filePath))
	{
		if (inFile == filePath)
		{
			if (FileNameMatches("*.a;*.so;*.dylib", inFile))
			{
				int r = DisplayAlert("ask-add-lib-as-link", inFile.filename());
				
				if (r == 1)
					projectFile.reset(new MProjectFile(name, nil, inFile.parent_path()));
				else if (r == 2)
				{
					// user chose 'Link' so we create a link statement
					
					bool shared = true;
					
					string linkName = inFile.filename();
					if (ba::starts_with(linkName, "lib"))
						linkName.erase(0, 3);

					if (ba::ends_with(linkName, ".a"))
					{
						linkName.erase(linkName.end() - 2, linkName.end());
						shared = false;
					}
					else if (ba::ends_with(linkName, ".so"))
						linkName.erase(linkName.end() - 3, linkName.end());
					else
						linkName.erase(linkName.end() - 6, linkName.end());
					
					projectFile.reset(new MProjectLib(linkName, shared, false, nil));
				}
			}
			else
				projectFile.reset(new MProjectFile(name, nil, inFile.parent_path()));
		}
		else
			THROW(("Cannot add file %s since another file with that name but in another location is already present.",
				name.c_str()));
	}
	else
	{
		fs::path dir = relative_path(mProjectDir, inFile.parent_path());
		
		if (DisplayAlert("ask-add-include-path-alert", dir.string()) == 1)
		{
			if (FileNameMatches("*.a;*.so;*.dylib", inFile))
				mProjectInfo.mLibSearchPaths.push_back(dir);
			else
				mProjectInfo.mUserSearchPaths.push_back(dir);

			if (not LocateFile(name, true, filePath))
				THROW(("Cannot find file, something is wrong, sorry..."));
			
			projectFile.reset(new MProjectFile(name, nil, inFile.parent_path()));
		}
	}
	
	return projectFile.release();
}

void MProject::CreateFileItem(
	const string&		inFile,
	MProjectGroup*		inGroup,
	MProjectItem*&		outItem)
{
	MFile url(inFile);
	if (not url.IsLocal())
		THROW(("You can only add local files to a project"));
	
	fs::path p = url.GetPath();
	string name = p.filename();
	
	if (fs::is_directory(p))
	{
		SetModified(true);

		MProjectGroup* group = new MProjectGroup(name, inGroup);
		int32 index = 0;
		
		vector<string> files;
		MFileIterator iter(p, kFileIter_ReturnDirectories);
		while (iter.Next(p))
		{
			MProjectItem* item = nil;
			CreateFileItem(p.string(), group, item);
			if (item != nil)
			{
				group->AddProjectItem(item, index);
				++index;
			}
		}

		outItem = group;
	}
	else
	{
		unique_ptr<MProjectItem> projectFile;
		fs::path filePath;
		
		if (LocateFile(name, true, filePath))
		{
			if (p == filePath)
			{
				if (FileNameMatches("*.a;*.so;*.dylib", p))
				{
					int r = DisplayAlert("ask-add-lib-as-link", p.filename());
					
					if (r == 1)
						projectFile.reset(new MProjectFile(name, inGroup, p.parent_path()));
					else if (r == 2)
					{
						// user chose 'Link' so we create a link statement
						
						bool shared = true;
						
						string linkName = p.filename();
						if (ba::starts_with(linkName, "lib"))
							linkName.erase(0, 3);

						if (ba::ends_with(linkName, ".a"))
						{
							linkName.erase(linkName.end() - 2, linkName.end());
							shared = false;
						}
						else if (ba::ends_with(linkName, ".so"))
							linkName.erase(linkName.end() - 3, linkName.end());
						else
							linkName.erase(linkName.end() - 6, linkName.end());
						
						projectFile.reset(new MProjectLib(linkName, shared, false, inGroup));
					}
				}
				else
					projectFile.reset(new MProjectFile(name, inGroup, p.parent_path()));
			}
			else
				THROW(("Cannot add file %s since another file with that name but in another location is already present.",
					name.c_str()));
		}
		else
		{
			fs::path dir = relative_path(mProjectDir, p.parent_path());
			
			if (DisplayAlert("ask-add-include-path-alert", dir.string()) == 1)
			{
				mProjectInfo.mUserSearchPaths.push_back(dir);

				if (not LocateFile(name, true, filePath))
					THROW(("Cannot find file, something is wrong, sorry..."));
				
				projectFile.reset(new MProjectFile(name, inGroup, p.parent_path()));
			}
		}
		
		outItem = projectFile.release();
	}
}

void MProject::CreateResourceItem(
	const string&		inFile,
	MProjectGroup*		inGroup,
	MProjectItem*&		outItem)
{
	MFile url(inFile);
	if (not url.IsLocal())
		THROW(("You can only add local files to a project"));
	
	fs::path p = url.GetPath();
	string name = p.filename();
	
	if (fs::is_directory(p))
	{
		SetModified(true);

		MProjectGroup* group = new MProjectGroup(name, inGroup);
		int32 index = 0;
		
		vector<string> files;
		MFileIterator iter(p, kFileIter_ReturnDirectories);
		while (iter.Next(p))
		{
			MProjectItem* item = nil;
			CreateResourceItem(p.string(), group, item);
			if (item != nil)
			{
				group->AddProjectItem(item, index);
				++index;
			}
		}

		outItem = group;
	}
	else
	{
		unique_ptr<MProjectItem> projectFile;
		fs::path filePath;
		
		fs::path rsrcDir = mProjectDir / mProjectInfo.mResourcesDir;
		filePath = relative_path(rsrcDir, p);
		outItem = new MProjectResource(filePath.filename(), inGroup, rsrcDir / filePath.parent_path());
	}
}

MProjectItem* MProject::CreateRsrcItem(
	const fs::path&		inFile)
{
	string name = inFile.filename();
	
	unique_ptr<MProjectItem> projectFile;
	fs::path filePath;
	
	fs::path rsrcDir = mProjectDir / mProjectInfo.mResourcesDir;
	filePath = relative_path(rsrcDir, inFile);
	return new MProjectResource(filePath.filename(), nil, rsrcDir / filePath.parent_path());
}

bool MProject::IsValidItem(
	MProjectItem*		inItem)
{
	return mProjectItems.Contains(inItem) or mPackageItems.Contains(inItem);
}

void MProject::ProjectItemMoved()
{
	SetModified(true);
}

void MProject::GetInfo(
	MProjectInfo&							outInfo) const
{
	outInfo = mProjectInfo;
}

void MProject::SetInfo(
	const MProjectInfo&						inInfo)
{
	mProjectInfo = inInfo;

	SetModified(true);
	
	uint32 currentTarget = mCurrentTarget;
	mCurrentTarget = mProjectInfo.mTargets.size();	// force update
	SelectTarget(currentTarget);
	
	ResearchForFiles();
	
	eTargetsChanged();
}

