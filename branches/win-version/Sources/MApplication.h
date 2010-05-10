//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MAPPLICATION_H
#define MAPPLICATION_H

#include <list>
#include <deque>

#include "MTypes.h"
#include "MHandler.h"
#include "MP2PEvents.h"
#include "MController.h"

extern const char kAppName[], kVersionString[];

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

	MDocument*			OpenOneDocument(
							const MFile&		inFileRef);

	MDocument*			AskOpenOneDocument();

	MDocWindow*			DisplayDocument(
							MDocument*			inDocument);

	const std::string&	GetCurrentFolder() const				{ return mCurrentFolder; }

	void				SetCurrentFolder(
							const std::string&	inFolder)		{ SetCurrentFolder(inFolder.c_str()); }

	void				SetCurrentFolder(
							const char*			inFolder);

	MEventOut<void(double)>						eIdle;

	//bool				IsServer();
	//bool				IsClient();

	void				RunEventLoop();

  private:
	typedef std::list<MWindow*>		MWindowList;

	void				DoQuit();
	void				DoNew();
	void				DoOpen();
	void				DoSaveAll();
	void				DoCloseAll(
							MCloseReason		inReason);
	void				UpdateWindowMenu(
							MMenu*				inMenu);
	void				DoSelectWindowFromWindowMenu(
							uint32				inIndex);
	void				Pulse();

	MApplicationImpl*	mImpl;

	bool				mQuit;
	bool				mQuitPending;
	bool				mInitialized;
	std::string			mCurrentFolder;
};

extern MApplication*	gApp;

#endif
