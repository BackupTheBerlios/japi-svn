//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MJAPIAPP_H
#define MJAPIAPP_H

#include <list>
#include <deque>

#include "MTypes.h"
#include "MApplication.h"
#include "MP2PEvents.h"
#include "MController.h"
#include "MColor.h"

extern const char kAppName[], kVersionString[];

class MWindow;
class MDocument;
class MFile;

// ===========================================================================

class MJapiApp : public MApplication
{
  public:

	typedef std::vector<std::pair<uint32, MFile> > MFilesToOpenList;
	
						MJapiApp();
	
						~MJapiApp();
	
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

	MEventIn<void(const std::string&,MMenu*)>	eUpdateSpecialMenu;

	bool				LocateSystemIncludeFile(
							const std::string&	inFileName,
							MFile&				outFile);

	MDocument*			OpenOneDocument(
							const MFile&		inFileRef);

	MWindow*			DisplayDocument(
							MDocument*			inDocument);

	void				OpenProject(
							const MFile&		inPath);

	void				OpenEPub(
							const MFile&		inPath);

	void				ImportOEB();

	const std::string&	GetCurrentFolder() const				{ return mCurrentFolder; }

	void				SetCurrentFolder(
							const std::string&	inFolder)		{ SetCurrentFolder(inFolder.c_str()); }

	void				SetCurrentFolder(
							const char*			inFolder);

  private:
	typedef std::list<MWindow*>		MWindowList;

	virtual void		DoQuit();

	virtual void		DoNew();

	void				DoNewProject();

	void				DoNewEPub();

	void				DoSaveAll();

	void				DoCloseAll(
							MCloseReason		inReason);

	void				DoOpenTemplate(
							const std::string&	inTemplate);

	void				UpdateSpecialMenu(
							const std::string&	inName,
							MMenu*				inMenu);

	void				UpdateWindowMenu(
							MMenu*				inMenu);

	void				UpdateTemplateMenu(
							MMenu*				inMenu);

	void				UpdateScriptsMenu(
							MMenu*				inMenu);
	
	void				UpdateEPubMenu(
							MMenu*				inMenu);
	
	void				DoSelectWindowFromWindowMenu(
							uint32				inIndex);

	void				ShowWorksheet();
		
	//static gboolean		Timeout(
	//						gpointer			inData);

	//static gint			Snooper(
	//						GtkWidget*			inGrabWidget,
	//						GdkEventKey*		inEvent,
	//						gpointer			inFuncData); 

	void				ProcessSocketMessages();

	void				InitGlobals();
	void				SaveGlobals();

	int					mSocketFD;
	std::string			mCurrentFolder;
};

#endif
