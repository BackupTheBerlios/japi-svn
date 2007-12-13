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
#include "MWindow.h"
#include "MCallbacks.h"
#include "MSaverMixin.h"
#include "MProjectItem.h"
#include "MMenu.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlwriter.h>

class MListView;
class MMessageWindow;
class MProjectJob;
class MProjectTarget;
class MDevice;

enum MProjectListPanel
{
	ePanelFiles,
	ePanelLinkOrder,
	ePanelPackage,
	
	ePanelCount
};

class MProject : public MWindow, public MSaverMixin
{
  public:
						MProject(
							const MPath&		inPath);

	virtual				~MProject();

	const MPath&		GetPath() const			{ return mProjectFile; }

	static MProject*	Instance();

	static void			CloseAllProjects(
							MCloseReason		inAction);

	void				Initialize();

	static void			RecheckFiles();

	virtual bool		ProcessCommand(
							uint32				inCommand,
							const MMenu*		inMenu,
							uint32				inItemIndex);

	virtual bool		UpdateCommandStatus(
							uint32				inCommand,
							MMenu*				inMenu,
							uint32				inItemIndex	,
							bool&				outEnabled,
							bool&				outChecked);

	void				SetStatus(
							const std::string&	inMessage,
							bool				inActive);

	MMessageWindow*		GetMessageWindow();

	void				StopBuilding();

	void				Preprocess(
							const MPath&		inFile);

	void				CheckSyntax(
							const MPath&		inFile);

	void				Compile(
							const MPath&		inFile);

	void				Disassemble(
							const MPath&		inFile);

	bool				IsFileInProject(
							const MPath&		inFile) const;

	bool				LocateFile(
							const std::string&	inFile,
							bool				inSearchUserPaths,
							MPath&				outPath) const;

	bool				LocateInFramework(
							const MPath&		inFrameworksPath,
							const std::string&	inFramework,
							const std::string&	inFile,
							MPath&				outPath) const;

	void				GetIncludePaths(
							std::vector<MPath>&	outPaths) const;

	void				TargetUpdated();

	void				SetProjectPaths(
							const std::vector<MPath>&	inUserPaths,
							const std::vector<MPath>&	inSysPaths,
							const std::vector<MPath>&	inLibPaths,
							const std::vector<MPath>&	inFrameworks);

	void				CreateNewGroup(
							const std::string&	inGroupName);

	MEventIn<void()>	eProjectFileStatusChanged;

  protected:

	virtual bool		SaveDocument();
	virtual void		RevertDocument();
	virtual bool		DoSaveAs(
							const MUrl&			inPath);
	virtual void		CloseAfterNavigationDialog();

	virtual void		Close();

	virtual bool		DoClose();

	void				DrawProjectItem(
							MDevice&			inDevice,
							MRect				inFrame,
							uint32				inRow,
							bool				inSelected,
							const void*			inData,
							uint32				inDataLength);

	void				ClickProjectItem(
							MRect				inFrame,
							uint32				inRow,
							int32				inX,
							int32				inY);

	void				FileItemDragged(
							uint32				inTargetRow,
							uint32				inNewRow,
							bool				inDropUnder);

	void				LinkOrderItemDragged(
							uint32				inTargetRow,
							uint32				inNewRow,
							bool				inDropUnder);

	void				PackageItemDragged(
							uint32				inTargetRow,
							uint32				inNewRow,
							bool				inDropUnder);

	void				FilesDropped(
							uint32				inTargetRow,
							std::vector<MPath>	inFiles);

	void				PackageFilesDropped(
							uint32				inTargetRow,
							std::vector<MPath>	inFiles);

	void				InvokeProjectItem(
							uint32				inItemNr);
	
	void				IsContainerItem(
							uint32				inItemNr,
							bool&				outIsContainer);

	void				DeleteProjectItem(
							uint32				inItemNr);

	void				ProjectFileStatusChanged();
	
//						// to check for cancel events
//	OSStatus			DoRawKeyDown(
//							EventRef			ioEvent);

	void				Poll(
							double				inSystemTime);

	MEventIn<void(MWindow*)>					eMsgWindowClosed;

	void				MsgWindowClosed(
							MWindow*			inWindow);

	MProjectFile*		GetProjectFileForPath(
							const MPath&		inPath) const;

	void				CheckIsOutOfDate();

	MPath				GetObjectPathForFile(
							const MPath&		inFile) const;

	MProjectJob*		CreateCompileJob(
							const MPath&		inFile);

	MProjectJob*		CreateCompileAllJob();

	MProjectJob*		CreateLinkJob(
							const MPath&		inLinkerOutput);

	MProjectJob*		CreateGenerateDSymJob(
							const MPath&		inLinkerOutput,
							const MPath&		inDSymFile);

	void				StartJob(
							MProjectJob*		inJob);

	void				BringUpToDate();

	void				Make();

	void				MakeClean();

	void				ReadPaths(
							xmlXPathObjectPtr	inData,
							std::vector<MPath>&	outPaths);

	typedef void (MProjectTarget::*AddOptionProc)(const char* inOption);

	void		 		ReadOptions(
							xmlNodePtr			inData,
							const char*			inOptionName,
							MProjectTarget*		inTarget,
							AddOptionProc		inAddOptionProc);

	void				ReadFiles(
							xmlNodePtr			inData,
							MProjectGroup*		inGroup);

	void				ReadPackageAction(
							xmlNodePtr			inData,
							MPath				inDir,
							MProjectGroup*		inGroup);

	void				Read(
							xmlXPathContextPtr	inContext);

	void				WritePaths(
							xmlTextWriterPtr	inWriter,
							const char*			inTag,
							std::vector<MPath>&	inPaths,
							bool				inFullPath);

	void				WriteFiles(
							xmlTextWriterPtr	inWriter,
							std::vector<MProjectItem*>&
												inItems);

	void				WritePackage(
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

	bool				Write(
							MFile*				inFile);

	void				CheckDataDir();

	void				UpdateList();

	void				AddItem(
							MProjectItem*		inItem,
							int32				inPosition,
							bool				inAddUnder);

	void				SelectTarget(
							uint32				inTarget);
	
	void				SelectPanel(
							MProjectListPanel	inPanel);

	void				ResearchForFiles();

	void				SetModified(
							bool				inModified);

	void				Read();

	bool				ReadState();
	
						// convenience routine, get the ProjectItem from the current list
	MProjectItem*		GetItem(
							int32				inItemNr);

	void				TargetSelected();

	MSlot<void()>			mTargetSelected;
	MEventIn<void(double)>	ePoll;

	bool				mModified;
	std::string			mName;
	MProjectTarget*		mCurrentTarget;
	MPath				mProjectFile;
	MPath				mProjectDir;
	MPath				mOutputDir;
	MPath				mProjectDataDir;
	MPath				mObjectDir;
	MPath				mResourcesDir;
	MProjectGroup		mProjectItems;
	MProjectGroup		mPackageItems;
	MProjectListPanel	mPanel;
	std::vector<MProjectTarget*>
						mTargets;
	std::vector<std::string>
						mPkgConfigPkgs;
	std::vector<std::string>
						mPkgConfigCFlags;
	std::vector<std::string>
						mPkgConfigLibs;
	std::vector<MPath>	mSysSearchPaths;
	std::vector<MPath>	mUserSearchPaths;
	std::vector<MPath>	mLibSearchPaths;
	std::vector<MPath>	mFrameworkPaths;
	std::auto_ptr<MMessageWindow>
						mStdErrWindow;
	
	std::auto_ptr<MProjectJob>
						mCurrentJob;

	MListView*			mFileList;
	MListView*			mLinkOrderList;
	MListView*			mPackageList;

	GtkWidget*			mVBox;
	MMenubar			mMenubar;
	GtkWidget*			mTargetPopup;
	uint32				mTargetPopupCount;
	GtkWidget*			mPanelSegment;
	GtkWidget*			mFilePanel;
	GtkWidget*			mLinkOrderPanel;
	GtkWidget*			mPackagePanel;
	GtkWidget*			mStatusPanel;

	static MProject*	sInstance;
	MProject*			mNext;
};

#endif
