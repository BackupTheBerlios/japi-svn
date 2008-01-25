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

#undef check
#ifndef BOOST_DISABLE_ASSERTS
#define BOOST_DISABLE_ASSERTS
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>

#include "MProject.h"
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
#include "MError.h"
#include "MTextDocument.h"
#include "MJapiApp.h"

using namespace std;
namespace ba = boost::algorithm;

namespace
{

const string
	kIQuote("-iquote"),
	kI("-I"),
	kF("-F"),
	kl("-l"),
	kL("-L");

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

MProject* MProject::Instance()
{
	MDocument* doc = GetFirstDocument();
	
	while (doc != nil and dynamic_cast<MProject*>(doc) == nil)
		doc = doc->GetNextDocument();
	
	return dynamic_cast<MProject*>(doc);
}

MProject::MProject(
	const MUrl*		inProjectFile)
	: MDocument(inProjectFile)
	, eMsgWindowClosed(this, &MProject::MsgWindowClosed)
	, ePoll(this, &MProject::Poll)
	, mProjectFile(inProjectFile->GetPath())
	, mProjectDir(mProjectFile.branch_path())
	, mCurrentTarget(nil)
	, mProjectItems("", nil)
	, mPackageItems("", nil)
	, mCurrentJob(nil)
{
    LIBXML_TEST_VERSION

	AddRoute(gApp->eIdle, ePoll);
	
	fs::ifstream file(mProjectFile, ios::binary);
	if (not file.is_open())
		THROW(("Could not open project file %s", mProjectFile.string().c_str()));
	ReadFile(file);
}

// ---------------------------------------------------------------------------
//	MProject::MProject

MProject::~MProject()
{
	for (vector<MProjectTarget*>::iterator target = mTargets.begin(); target != mTargets.end(); ++target)
		delete *target;
}

// ---------------------------------------------------------------------------
//	MProject::ReadFile

void MProject::ReadFile(
	istream&		inFile)
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
		// First read the data into a buffer
		streambuf* b = inFile.rdbuf();
		
		int64 len = b->pubseekoff(0, ios::end);
		b->pubseekpos(0);
	
		if (len > numeric_limits<uint32>::max())
			THROW(("File too large to open"));
	
		auto_array<char> data(new char[len]);
		b->sgetn(data.get(), len);

		xmlDoc = xmlParseMemory(&data[0], len);
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
	MDocument* doc = GetFirstDocument();
	
	while (doc != nil)
	{
		if (dynamic_cast<MProject*>(doc) != nil)
			static_cast<MProject*>(doc)->CheckIsOutOfDate();
		
		doc = doc->GetNextDocument();
	}
}

// ---------------------------------------------------------------------------
//	MProject::CreateNewGroup

void MProject::CreateNewGroup(
	const string&		inGroupName,
	MProjectGroup*		inGroup,
	int32				inIndex)
{
	MProjectGroup* root = inGroup;
	THROW_IF_NIL(root);
	
	while (root->GetParent() != nil)
		root = root->GetParent();

	MProjectGroup* newGroup = new MProjectGroup(inGroupName, nil);
	inGroup->AddProjectItem(newGroup, inIndex);
	
	SetModified(true);
	if (root == &mProjectItems)
		eInsertedFile(newGroup);
	else
		eInsertedResource(newGroup);
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
//	MProject::ReadResources

void MProject::ReadResources(
	xmlNodePtr		inData,
	MProjectGroup*	inGroup)
{
	XMLNode data(inData);
	
	for (XMLNode::iterator node = data.begin(); node != data.end(); ++node)
	{		
		if (node->name() == "resource")
		{
			string fileName = node->text();
			if (fileName.length() == 0)
				THROW(("Invalid project file"));
			
			try
			{
				MPath filePath = mResourcesDir / fileName;

				auto_ptr<MProjectResource> projectFile(
					new MProjectResource(filePath.leaf(), inGroup, filePath.branch_path()));

				inGroup->AddProjectItem(projectFile.release());
			}
			catch (std::exception& e)
			{
				DisplayError(e);
			}
		}
		else if (node->name() == "group")
		{
			string name = node->property("name");
			
			auto_ptr<MProjectGroup> group(new MProjectGroup(name, inGroup));
			
			if (node->children() != nil)
				ReadResources(*node, group.get());
			
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

//				AddRoute(projectFile->eStatusChanged, eProjectFileStatusChanged);

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
	
	data = xmlXPathEvalExpression(BAD_CAST "/project/resources", inContext);
	
	if (data != nil and data->nodesetval != nil)
	{
		for (int i = 0; i < data->nodesetval->nodeNr; ++i)
		{
			XMLNode node(data->nodesetval->nodeTab[i]);

			string rd = node.property("resource_dir");
			if (rd.length() > 0)
				mResourcesDir = mProjectDir / rd;
			else
				mResourcesDir = mProjectDir / "Resources";
	
			ReadResources(node, &mPackageItems);
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

	SetModified(false);
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
//	MProject::WriteResources

void MProject::WriteResources(
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
			
			WriteResources(inWriter, group->GetItems());
			
			THROW_IF_XML_ERR(xmlTextWriterEndElement(inWriter));
		}
		else if (dynamic_cast<MProjectResource*>(*item) != nil)
		{
			MPath path = static_cast<MProjectResource*>(*item)->GetPath();

			THROW_IF_XML_ERR(xmlTextWriterWriteElement(inWriter, BAD_CAST "resource",
				BAD_CAST relative_path(mResourcesDir, path).string().c_str()));
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
//	MProject::WriteFile

void MProject::WriteFile(
	ostream&			inFile)
{
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
		THROW_IF_XML_ERR(xmlTextWriterStartElement(writer, BAD_CAST "resources"));

		THROW_IF_XML_ERR(xmlTextWriterWriteAttribute(writer,
			BAD_CAST "resource_dir",
			BAD_CAST relative_path(mProjectDir, mResourcesDir).string().c_str()));

		WriteResources(writer, mPackageItems.GetItems());
		
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
		
		inFile.write(reinterpret_cast<const char*>(buf->content), buf->use);
	}
	catch (exception& e)
	{
		if (buf != nil)
			xmlBufferFree(buf);
		
		throw;
	}
	
	if (buf != nil)
		xmlBufferFree(buf);
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
	
	argv.push_back(Preferences::GetString("c++", "/usr/bin/c++"));

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

	if (files.size() > 0)
	{
		MTargetCPU arch = mCurrentTarget->GetTargetCPU();
		if (arch == eCPU_native)
			arch = MProjectTarget::GetNativeCPU();
		
		job->AddJob(new MProjectCreateResourceJob(
			"Creating resources", this, files, mObjectDir / "__rsrc__.o", arch));
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
	
	if (mCurrentTarget->GetKind() == eTargetStaticLibrary)
		argv.push_back(Preferences::GetString("linker", "/usr/bin/ld"));
	else
		argv.push_back(Preferences::GetString("c++", "/usr/bin/c++"));

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
	
	argv.push_back(Preferences::GetString("c++", "/usr/bin/c++"));

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
	
	MTextDocument* output = new MTextDocument(nil);
	output->SetFileNameHint(inFile.leaf() + " # preprocessed");
	
	SetCallBack(job->eStdOut, output, &MTextDocument::StdOut);
	gApp->DisplayDocument(output);

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
	
	argv.push_back(Preferences::GetString("c++", "/usr/bin/c++"));

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
	
	MTextDocument* output = new MTextDocument(nil);
	output->SetFileNameHint(inFile.leaf() + " # disassembled");
	
	SetCallBack(job->eStdOut, output, &MTextDocument::StdOut);
	gApp->DisplayDocument(output);

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
	
	argv.push_back(Preferences::GetString("c++", "/usr/bin/c++"));

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
//	MProject::GetSelectedTarget

uint32 MProject::GetSelectedTarget() const
{
	vector<MProjectTarget*>::const_iterator t = find(mTargets.begin(), mTargets.end(), mCurrentTarget);

	if (t == mTargets.end())
		t = mTargets.begin();

	return t - mTargets.begin();
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
	
//	if (gtk_combo_box_get_active(GTK_COMBO_BOX(mTargetPopup)) != int32(inTarget))
//		gtk_combo_box_set_active(GTK_COMBO_BOX(mTargetPopup), inTarget);

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
//	MProject::SetStatus

void MProject::SetStatus(
	const string&	inStatus,
	bool			inBusy)
{
	eStatus(inStatus, inBusy);
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
	bool result = true, isCompilable = false;
	
	outEnabled = false;
	
	switch (inCommand)
	{
		case cmd_Preprocess:
		case cmd_CheckSyntax:
		case cmd_Compile:
		case cmd_Disassemble:
			outEnabled = isCompilable;
			break;

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
	uint32			inItemIndex)
{
	bool result = true;

	MProjectItem* item = nil;
	MProjectFile* file = dynamic_cast<MProjectFile*>(item);
	
	switch (inCommand)
	{
		case cmd_RecheckFiles:
			CheckIsOutOfDate();
			break;
		
		case cmd_BringUpToDate:
			BringUpToDate();
			break;
		
		case cmd_Preprocess:
			if (file != nil)
				Preprocess(file->GetPath());
			break;
		
		case cmd_CheckSyntax:
			if (file != nil)
				CheckSyntax(file->GetPath());
			break;
		
		case cmd_Compile:
			if (file != nil)
				Compile(file->GetPath());
			break;

		case cmd_Disassemble:
			if (file != nil)
				Disassemble(file->GetPath());
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

			result = MDocument::ProcessCommand(inCommand, inMenu, inItemIndex);
			break;
	}
	
	return result;
}

void MProject::AddFiles(
	vector<string>&		inFiles,
	MProjectGroup*		inGroup,
	uint32				inIndex)
{
	MProjectGroup* root = inGroup;
	THROW_IF_NIL(root);
	
	while (root->GetParent() != nil)
		root = root->GetParent();
	
	try
	{
		for (vector<string>::iterator file = inFiles.begin(); file != inFiles.end(); ++file)
		{
			ba::trim(*file);
			if (file->length() == 0)
				continue;
			
			MUrl url(*file);
			if (not url.IsLocal())
				THROW(("You can only add local files to a project"));
			
			MPath p = url.GetPath();
			string name = p.leaf();
			
			if (fs::is_directory(p))
			{
				SetModified(true);

				MProjectGroup* group = new MProjectGroup(name, inGroup);
				inGroup->AddProjectItem(group, inIndex);
				
				if (root == &mProjectItems)
					eInsertedFile(group);
				else
					eInsertedResource(group);
				
				vector<string> files;
				MFileIterator iter(p, kFileIter_ReturnDirectories);
				while (iter.Next(p))
					files.push_back(MUrl(p).str());
				
				AddFiles(files, group, 0);
			}
			else if (root == &mProjectItems)
			{
				auto_ptr<MProjectFile> projectFile;
				MPath filePath;
				
				if (LocateFile(name, true, filePath))
				{
					if (p == filePath)
						projectFile.reset(new MProjectFile(name, inGroup, p.branch_path()));
					else
						THROW(("Cannot add file %s since another file with that name but in another location is already present.",
							name.c_str()));
				}
				else
				{
					MPath dir = relative_path(mProjectDir, p.branch_path());
					
					if (DisplayAlert("ask-add-include-path-alert", dir.string()) == 1)
					{
						mUserSearchPaths.push_back(dir);

						if (not LocateFile(name, true, filePath))
							THROW(("Cannot find file, something is wrong, sorry..."));
						
						projectFile.reset(new MProjectFile(name, inGroup, p.branch_path()));
					}
				}
				
				MProjectItem* item = projectFile.release();
				if (item != nil)
				{
					SetModified(true);
	
					inGroup->AddProjectItem(item, inIndex);

					if (root == &mProjectItems)
						eInsertedFile(item);
					else
						eInsertedResource(item);
				}
			}
		}
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

void MProject::RemoveItem(
	MProjectItem*		inItem)
{
	THROW_IF_NIL(inItem);
	
	MProjectGroup* group = inItem->GetParent();
	int32 index = inItem->GetPosition();
	
	group->RemoveProjectItem(inItem);
	SetModified(true);
	
	MProjectGroup* root = group;
	THROW_IF_NIL(root);
	
	while (root->GetParent() != nil)
		root = root->GetParent();
	
	if (root == &mProjectItems)
		eRemovedFile(group, index);
	else
		eRemovedResource(group, index);
}

void MProject::RemoveItems(
	vector<MProjectItem*>&	inItems)
{
	for (vector<MProjectItem*>::iterator item = inItems.begin(); item != inItems.end(); ++item)
	{
		if (IsValidItem(*item))
			RemoveItem(*item);
	}
}

bool MProject::IsValidItem(
	MProjectItem*		inItem)
{
	return mProjectItems.Contains(inItem) or mPackageItems.Contains(inItem);
}

void MProject::MoveItem(
	MProjectItem*		inItem,
	MProjectGroup*		inGroup,
	uint32				inIndex)
{
	THROW_IF_NIL(inItem);
	
	MProjectGroup* group = inItem->GetParent();
	int32 index = inItem->GetPosition();
	
	MProjectGroup* root = group;
	THROW_IF_NIL(root);
	
	while (root->GetParent() != nil)
		root = root->GetParent();
	
	if (root == &mProjectItems)
		EmitRemovedRecursive(eRemovedFile, inItem, group, index);
	else
		EmitRemovedRecursive(eRemovedResource, inItem, group, index);

	group->RemoveProjectItem(inItem);
	SetModified(true);
	
	inGroup->AddProjectItem(inItem, inIndex);
			
	if (root == &mProjectItems)
		EmitInsertedRecursive(eInsertedFile, inItem);
	else
		EmitInsertedRecursive(eInsertedResource, inItem);
	
	ResearchForFiles();
	
	MModDateCache modDateCache;
	
	if (root == &mProjectItems)
	{
		mProjectItems.UpdatePaths(mObjectDir);
		mProjectItems.CheckCompilationResult();
		mProjectItems.CheckIsOutOfDate(modDateCache);
	}
	else
	{
		mPackageItems.UpdatePaths(mObjectDir);
		mPackageItems.CheckCompilationResult();
		mPackageItems.CheckIsOutOfDate(modDateCache);
	}
}

void MProject::EmitRemovedRecursive(
	MEventOut<void(MProjectGroup*,int32)>&	inEvent,
	MProjectItem*							inItem,
	MProjectGroup*							inParent,
	int32									inIndex)
{
	if (dynamic_cast<MProjectGroup*>(inItem) != nil)
	{
		MProjectGroup* group = static_cast<MProjectGroup*>(inItem);
		vector<MProjectItem*>& items = group->GetItems();
		
		for (int32 ix = items.size() - 1; ix >= 0; --ix)
			EmitRemovedRecursive(inEvent, items[ix], group, ix);
	}

	inEvent(inParent, inIndex);
}

void MProject::EmitInsertedRecursive(
	MEventOut<void(MProjectItem*)>&			inEvent,
	MProjectItem*							inItem)
{
	inEvent(inItem);

	if (dynamic_cast<MProjectGroup*>(inItem) != nil)
	{
		MProjectGroup* group = static_cast<MProjectGroup*>(inItem);
		vector<MProjectItem*>& items = group->GetItems();
		
		for (int32 ix = items.size() - 1; ix >= 0; --ix)
			EmitInsertedRecursive(inEvent, items[ix]);
	}
}
