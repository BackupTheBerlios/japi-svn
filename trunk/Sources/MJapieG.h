#ifndef MJG_H
#define MJG_H

#include <gtk/gtk.h>
#include <list>

#include "MTypes.h"
#include "MHandler.h"

#define nil NULL

class MWindow;

class MJapieApp : public MHandler
{
  public:
					MJapieApp(
						int				argc,
						char*			argv[]);
					~MJapieApp();
	
	void			RecycleWindow(
						MWindow*		inWindow);

	void			DoQuit();
	void			DoNew();
	void			DoOpen();

	void			RunEventLoop();
	
	virtual bool	UpdateCommandStatus(
						uint32			inCommand,
						bool&			outEnabled,
						bool&			outChecked);

	virtual bool	ProcessCommand(
						uint32			inCommand);

  private:
	typedef std::list<MWindow*>	MWindowList;

	void			Pulse();
	
	static gboolean	Timeout(
						gpointer		inData);

	MWindowList		mTrashCan;
	bool			mQuit;
	bool			mQuitPending;
};

extern MJapieApp*	gApp;

#endif
