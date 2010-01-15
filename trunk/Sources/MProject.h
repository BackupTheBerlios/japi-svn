//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPROJECT_H
#define MPROJECT_H

#include "MFile.h"
#include "MProjectItem.h"
#include "MDocument.h"

namespace xml { class node; class node_list; }

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
							const MFile&		inProjectFile);

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
							uint32			inItemIndex,
							uint32			inModifiers);

	std::string			GetName() const			{ return mName; }

	const fs::path&		GetPath() const			{ return mProjectFile; }

	static MProject*	Instance();

	const std::vector<MProjectTarget>&
						GetTargets() const		{ return mProjectInfo.mTargets; }
	MProjectGroup*		GetFiles() const		{ return const_cast<MProjectGroup*>(&mProjectItems); }
	MProjectGroup*		GetResources() const	{ return const_cast<MProjectGroup*>(&mPackageItems); }

	MEventIn<void(const std::string&, MProjectGroup*, MProjectItem*&)>
											eProjectCreateFileItem;
	MEventIn<void(const std::string&, MProjectGroup*, MProjectItem*&)>
											eProjectCreateResourceItem;
	
	MEventIn<void()>						eProjectItemMoved;
	MEventIn<void()>						eProjectItemRemoved;

	void				CreateFileItem(
							const std::string&	inFile,
							MProjectGroup*		inGroup,
							MProjectItem*&		outItem);

	void				CreateResourceItem(
							const std::string&	inFile,
							MProjectGroup*		inGroup,
							MProjectItem*&		outItem);

	bool				IsValidItem(
							MProjectItem*		inItem);

	void				ProjectItemMoved();

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
							const xml::node_list&
												inData,
							std::vector<fs::path>&
												outPaths);

	void		 		ReadOptions(
							const xml::node&	inData,
							const char*			inOptionName,
							std::vector<std::string>&
												outOptions);

	void				ReadFiles(
							const xml::node&	inData,
							MProjectGroup*		inGroup);

	void				ReadResources(
							const xml::node&	inData,
							MProjectGroup*		inGroup);

	void				Read(
							const xml::node&	inContext);

	void				WritePaths(
							xml::node&			inNode,
							const char*			inTag,
							std::vector<fs::path>&
												inPaths,
							bool				inFullPath);

	void				WriteFiles(
							xml::node&			inNode,
							std::vector<MProjectItem*>&
												inItems);

	void				WriteResources(
							xml::node&			inNode,
							std::vector<MProjectItem*>&
												inItems);

	void				WriteTarget(
							xml::node&			inNode,
							MProjectTarget&		inTarget);

	void				WriteOptions(
							xml::node&			inNode,
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

	MEventOut<void()>						eTargetsChanged;	// notify window

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
