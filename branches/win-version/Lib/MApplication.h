//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MAPPLICATION_H
#define MAPPLICATION_H

#include <list>
#include <vector>
#include <deque>

#include "MTypes.h"
#include "MHandler.h"
#include "MP2PEvents.h"
#include "MController.h"
#include "MFile.h"

extern const char kAppName[], kVersionString[];

extern fs::path gExecutablePath;

class MWindow;
class MDocument;
class MFile;
class MApplicationImpl;

// ===========================================================================

class MApplication : public MHandler
{
  public:

	typedef std::vector<std::pair<uint32, MFile> > MFilesToOpenList;
	
						MApplication();
	
						~MApplication();

	static int			Main(
							const std::vector<std::string>&
												inArgs);

	virtual bool		UpdateCommandStatus(
							uint32				inCommand,
							MMenu*				inMenu,
							uint32				inItemIndex,
							bool&				outEnabled,
							bool&				outChecked);

	virtual bool		ProcessCommand(
							uint32				inCommand,
							const MMenu*		inMenu,
							uint32				inItemIndex,
							uint32				inModifiers);

	virtual void		UpdateSpecialMenu(
							const std::string&	inMenuKind,
							MMenu*				inMenu);

	void				AddToRecentMenu(
							const fs::path&		inFile);

	virtual MDocument*	OpenOneDocument(
							const MFile&		inFileRef);

	virtual MDocument*	CreateNewDocument();

	virtual MWindow*	DisplayDocument(
							MDocument*			inDocument);

	const std::string&	GetCurrentFolder() const				{ return mCurrentFolder; }

	virtual void		SetCurrentFolder(
							const std::string&	inFolder)		{ SetCurrentFolder(inFolder.c_str()); }

	virtual void		SetCurrentFolder(
							const char*			inFolder);

	MEventOut<void(double)>						eIdle;

	//bool				IsServer();
	//bool				IsClient();

	int					RunEventLoop();

  protected:
	  friend class MApplicationImpl;
	  
	  typedef std::list<MWindow*>		MWindowList;

	virtual void		DoQuit();
	virtual void		DoNew();
	virtual void		DoOpen();
	virtual void		DoSaveAll();
	virtual void		DoCloseAll(
							MCloseReason		inReason);
	virtual void		UpdateWindowMenu(
							MMenu*				inMenu);
	void				UpdateRecentMenu(
							MMenu*				inMenu);
	virtual void		DoSelectWindowFromWindowMenu(
							uint32				inIndex);
	virtual void		Pulse();

	virtual void		InitGlobals();
	virtual void		SaveGlobals();

	MApplicationImpl*	mImpl;

	bool				mQuit;
	bool				mQuitPending;
	bool				mInitialized;
	std::string			mCurrentFolder;
	std::deque<fs::path>
						mRecentFiles;
};

extern MApplication*	gApp;

#endif
