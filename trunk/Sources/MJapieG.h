/*
	Copyright (c) 2007, Maarten L. Hekkelman
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

#ifndef MJG_H
#define MJG_H

#include <gtk/gtk.h>
#include <list>
#include <deque>

#include "MTypes.h"
#include "MHandler.h"
#include "MP2PEvents.h"
#include "MController.h"

#define nil NULL

// ---------------------------------------------------------------------------
//	Command ID numbers for our custom menu items

const uint32
	cmd_Preferences		= 'pref',

	cmd_CloseAll		= 'Clos',
	cmd_SaveAll			= 'Save',
	cmd_OpenRecent		= 'Recd',
	cmd_OpenTemplate	= 'Tmpd',
	cmd_ClearRecent		= 'ClrR',
	cmd_Worksheet		= 'Wrks';

class MWindow;
class MDocument;

// ===========================================================================

class MJapieApp : public MHandler
{
  public:
					MJapieApp(
						int					argc,
						char*				argv[]);

					~MJapieApp();
	
	void			RecycleWindow(
						MWindow*			inWindow);
	
	virtual bool	UpdateCommandStatus(
						uint32				inCommand,
						bool&				outEnabled,
						bool&				outChecked);

	virtual bool	ProcessCommand(
						uint32				inCommand);

	void			RebuildRecentMenu();

	bool			LocateSystemIncludeFile(
						const std::string&	inFileName,
						MPath&				outFile);

	MDocument*		OpenOneDocument(
						const MPath&			inFileRef);

	MDocument*		AskOpenOneDocument();

	void			OpenProject(
						const MPath&			inPath);

	void			AddToRecentMenu(
						const MPath&			inFileRef);

	void			StoreRecentMenu();
	
	GtkRecentManager*
					GetRecentMgr() const					{ return mRecentMgr; }
	
	MEventOut<void(double)>					eIdle;

	void			RunEventLoop();

  private:
	typedef std::list<MWindow*>	MWindowList;

	void			DoQuit();

	void			DoNew();

	void			DoOpen();

	void			DoSaveAll();

	void			DoCloseAll(
						MCloseReason		inReason);

	void			DoClearRecent();

	void			DoOpenRecent(
						uint32				inCommand);

	void			DoOpenTemplate(
						uint32				inCommand);

//	void			DoNavUserAction(NavCBRecPtr inParams);
//	void			DoNavTerminate(NavCBRecPtr inParams);

//	void			DoNavOpenDocument(NavCBRecPtr inParams);
//	void			OpenListOfDocuments(const AEDesc& inDocList, const AEDesc& inSelectionList);

	void			ShowWorksheet();
	
	void			Pulse();
	
	static gboolean	Timeout(
						gpointer			inData);

	static gint		Snooper(
						GtkWidget*			inGrabWidget,
						GdkEventKey*		inEvent,
						gpointer			inFuncData); 

	MWindowList			mTrashCan;
	GtkRecentManager*	mRecentMgr;
	bool				mQuit;
	bool				mQuitPending;
};

extern MJapieApp*	gApp;

#endif
