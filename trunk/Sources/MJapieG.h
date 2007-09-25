#ifndef MJG_H
#define MJG_H

#include <gtk/gtk.h>
#include <list>

#define nil NULL

class MWindow;

class MJapieApp
{
  public:
				MJapieApp(
					int				argc,
					char*			argv[]);
				~MJapieApp();
	
	void		RecycleWindow(
					MWindow*		inWindow);

	void		DoQuit();
	void		DoNew();

	void		RunEventLoop();

  private:
	typedef std::list<MWindow*>	MWindowList;

	void		Pulse();

	MWindowList	mTrashCan;
	bool		mQuit;
	bool		mQuitPending;
};

extern MJapieApp*	gApp;

#endif
