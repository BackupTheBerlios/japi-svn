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

#ifndef MPROJECT_H
#define MPROJECT_H

#include "MFile.h"
#include "MProjectItem.h"
#include "MDocument.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlwriter.h>

class MWindow;
class MMessageWindow;
class MProjectJob;

enum MProjectListPanel
{
	ePanelFiles,
	ePanelLinkOrder,
	ePanelPackage,
	
	ePanelCount
};

// ---------------------------------------------------------------------------
//	MProjectTarget

enum MTargetCPU {
	eCPU_Unknown,
	eCPU_native,
	eCPU_386,
	eCPU_x86_64,
	eCPU_PowerPC_32,
	eCPU_PowerPC_64
};

enum MTargetKind
{
	eTargetNone,
	eTargetExecutable,
	eTargetSharedLibrary,
	eTargetStaticLibrary
};

enum MBuildFlags
{
	eBF_debug		= 1 << 0,
	eBF_profile		= 1 << 1,
	eBF_pic			= 1 << 2
};

struct MProjectTarget
{
	std::string					mLinkTarget;
	std::string					mName;
	MTargetKind					mKind;
	MTargetCPU					mTargetCPU;
	std::string					mCompiler;
	std::vector<std::string>	mCFlags;
	std::vector<std::string>	mLDFlags;
	std::vector<std::string>	mWarnings;
	std::vector<std::string>	mDefines;
	uint32						mBuildFlags;
};

// These are the settable fields (using info dialog)
struct MProjectInfo
{
	std::vector<MProjectTarget>	mTargets;
	fs::path					mOutputDir;
	bool						mAddResources;
	fs::path					mResourcesDir;
	std::vector<std::string>	mPkgConfigPkgs;
	std::vector<fs::path>		mSysSearchPaths;
	std::vector<fs::path>		mUserSearchPaths;
	std::vector<fs::path>		mLibSearchPaths;
};

// --------------------------------------------------------------------
// MProject

class MProject : public MDocument
{
  public:

	explicit			MProject(
							const fs::path&		inProjectFile);

						MProject(
							const fs::path&		inParentDir,
							const std::string&	inName,
							const std::string&	inTemplate);

	virtual				~MProject();

	virtual bool		UpdateCommandStatus(
							uint32			inCommand,
							MMenu*			inMenu,
							uint32			inItemIndex,
							bool&			outEnabled,
							bool&			outChecked);

	virtual bool		ProcessCommand(
							uint32			inCommand,
							const MMenu*	inMenu,
							uint32			inItemIndex);

	std::string			GetName() const			{ return mName; }

	const fs::path&		GetPath() const			{ return mProjectFile; }

	static MProject*	Instance();

	const std::vector<MProjectTarget>&
						GetTargets() const		{ return mProjectInfo.mTargets; }
	MProjectGroup*		GetFiles() const		{ return const_cast<MProjectGroup*>(&mProjectItems); }
	MProjectGroup*		GetResources() const	{ return const_cast<MProjectGroup*>(&mPackageItems); }

	void				AddFiles(
							std::vector<std::string>&
												inFiles,
							MProjectGroup*		inGroup,
							uint32				inIndex);

	void				RemoveItem(
							MProjectItem*		inItem);

	void				RemoveItems(
							std::vector<MProjectItem*>&
												inItems);

	bool				IsValidItem(
							MProjectItem*		inItem);

	void				MoveItem(
							MProjectItem*		inItem,
							MProjectGroup*		inGroup,
							uint32				inIndex);

	static void			RecheckFiles();

	MMessageWindow*		GetMessageWindow();

	void				StopBuilding();

	void				Preprocess(
							const fs::path&		inFile);

	void				CheckSyntax(
							const fs::path&		inFile);

	void				Compile(
							const fs::path&		inFile);

	void				Disassemble(
							const fs::path&		inFile);

	bool				IsFileInProject(
							const fs::path&		inFile) const;

	bool				LocateFile(
							const std::string&	inFile,
							bool				inSearchUserPaths,
							fs::path&			outPath) const;

	bool				LocateLibrary(
							const std::string&	inFile,
							fs::path&			outPath) const;

	void				GetIncludePaths(
							std::vector<fs::path>&
												outPaths) const;

	void				CreateNewGroup(
							const std::string&	inGroupName,
							MProjectGroup*		inGroup,
							int32				inIndex);

	void				SetStatus(
							const std::string&	inStatus,
							bool				inBusy);

	MEventOut<void(std::string,bool)>
						eStatus;

	virtual void		ReadFile(
							std::istream&		inFile);

	virtual void		WriteFile(
							std::ostream&		inFile);

	void				Poll(
							double				inSystemTime);

	MEventIn<void(MWindow*)>					eMsgWindowClosed;

	void				MsgWindowClosed(
							MWindow*			inWindow);

	void				StdErrIn(
							const char*			inText,
							uint32				inLength);

	MProjectFile*		GetProjectFileForPath(
							const fs::path&		inPath) const;

	void				CheckIsOutOfDate();

	fs::path			GetObjectPathForFile(
							const fs::path&		inFile) const;

	void				GenerateCFlags(
							std::vector<std::string>&
												outArgv) const;

	MProjectJob*		CreateCompileJob(
							const fs::path&		inFile);

	MProjectJob*		CreateCompileAllJob();

	MProjectJob*		CreateLinkJob(
							const fs::path&		inLinkerOutput);

	void				StartJob(
							MProjectJob*		inJob);

	void				BringUpToDate();

	bool				Make(
							bool				inUsePolling = true);

	void				MakeClean();

	void				ReadPaths(
							xmlXPathObjectPtr	inData,
							std::vector<fs::path>&
												outPaths);

	void		 		ReadOptions(
							xmlNodePtr			inData,
							const char*			inOptionName,
							std::vector<std::string>&
												outOptions);

	void				ReadFiles(
							xmlNodePtr			inData,
							MProjectGroup*		inGroup);

	void				ReadResources(
							xmlNodePtr			inData,
							MProjectGroup*		inGroup);

	void				Read(
							xmlXPathContextPtr	inContext);

	void				WritePaths(
							xmlTextWriterPtr	inWriter,
							const char*			inTag,
							std::vector<fs::path>&
												inPaths,
							bool				inFullPath);

	void				WriteFiles(
							xmlTextWriterPtr	inWriter,
							std::vector<MProjectItem*>&
												inItems);

	void				WriteResources(
							xmlTextWriterPtr	inWriter,
							std::vector<MProjectItem*>&
												inItems);

	void				WriteTarget(
							xmlTextWriterPtr	inWriter,
							MProjectTarget&		inTarget);

	void				WriteOptions(
							xmlTextWriterPtr	inWriter,
							const char*			inOptionGroupName,
							const char*			inOptionName,
							const std::vector<std::string>&
												inOptions);

	// interface for Project Info Dialog
	void				GetInfo(
							MProjectInfo&		outInfo) const;

	void				SetInfo(
							const MProjectInfo&	inInfo);

	void				CheckDataDir();

	void				SelectTarget(
							uint32				inTarget);
	
	void				SelectTarget(
							const std::string&	inTarget);
	
	uint32				GetSelectedTarget() const		{ return mCurrentTarget; }
	
	void				ResearchForFiles();

	MEventIn<void(double)>					ePoll;

	MEventOut<void(MProjectItem*)>			eInsertedFile;
	MEventOut<void(MProjectItem*)>			eInsertedResource;
	MEventOut<void(MProjectGroup*,int32)>	eRemovedFile;
	MEventOut<void(MProjectGroup*,int32)>	eRemovedResource;

	void				EmitRemovedRecursive(
							MEventOut<void(MProjectGroup*,int32)>&
												inEvent,
							MProjectItem*		inItem,
							MProjectGroup*		inParent,
							int32				inIndex);

	void				EmitInsertedRecursive(
							MEventOut<void(MProjectItem*)>&
												inEvent,
							MProjectItem*		inItem);

	std::string					mName;
	fs::path					mProjectFile;
	fs::path					mProjectDir;
	fs::path					mProjectDataDir;
	fs::path					mObjectDir;

	MProjectInfo				mProjectInfo;
	MProjectGroup				mProjectItems;
	MProjectGroup				mPackageItems;

	std::vector<std::string>	mPkgConfigCFlags;
	std::vector<std::string>	mPkgConfigLibs;
	std::string					mCppIncludeDir;		// compiler's c++ include dir
	std::vector<fs::path>		mCLibSearchPaths;	// compiler default search dirs
	MMessageWindow*				mStdErrWindow;
	bool						mAllowWindows;
	uint32						mCurrentTarget;
	std::auto_ptr<MProjectJob>	mCurrentJob;
};

#endif
