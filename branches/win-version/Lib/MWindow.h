//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINDOW_H
#define MWINDOW_H

#include <list>

#include "MView.h"
#include "MColor.h"
#include "MHandler.h"

#include "MP2PEvents.h"

#undef GetNextWindow

class MWindowImpl;

enum MWindowFlags
{
	kMFixedSize				= (1 << 0),
	kMAcceptFileDrops		= (1 << 1),
	kMPostionDefault		= (1 << 2),
	kMDialogBackground		= (1 << 3),
};

class MWindow : public MView, public MHandler
{
  public:
							MWindow(const std::string& inTitle,
								const MRect& inBounds, MWindowFlags inFlags,
								const std::string& inMenu);
	
	virtual					~MWindow();

	virtual MWindow*		GetWindow() const;

	MWindowFlags			GetFlags() const;

	virtual void			Show();
	virtual void			Select();
	
	virtual void			Activate();

	virtual void			UpdateNow();

	void					Close();
	
	void					Beep();
	
	static MWindow*			GetFirstWindow();
	MWindow*				GetNextWindow() const;
		
	void					SetTitle(
								const std::string&	inTitle);
	std::string				GetTitle() const;
	
	void					SetModifiedMarkInTitle(
								bool			inModified);

	// Focus is the handler that should receive commands and keyboard input,
	// by default, it is the window itself.
	virtual void			SetFocus(
								MHandler*		inHandler)	{ mFocus = inHandler; }
	MHandler*				GetFocus() const				{ return mFocus; }

	virtual bool			UpdateCommandStatus(
								uint32			inCommand,
								MMenu*			inMenu,
								uint32			inItemIndex,
								bool&			outEnabled,
								bool&			outChecked);

	virtual bool			ProcessCommand(
								uint32			inCommand,
								const MMenu*	inMenu,
								uint32			inItemIndex,
								uint32			inModifiers);

	MEventOut<void(MWindow*)>
							eWindowClosed;

	void					GetWindowPosition(
								MRect&			outPosition);

	void					SetWindowPosition(
								const MRect&	outPosition,
								bool			inTransition = false);

	void					GetMaxPosition(
								MRect&			outRect) const;
	
	MWindowImpl*			GetImpl() const			{ return mImpl; }

	// coordinate manipulations
	virtual void			ConvertToScreen(int32& ioX, int32& ioY) const;
	virtual void			ConvertFromScreen(int32& ioX, int32& ioY) const;

	virtual void			GetMouse(
								int32&			outX,
								int32&			outY,
								uint32&			outModifiers) const;

	virtual void			Invalidate(
								MRect			inRect);

	virtual void			ScrollRect(
								MRect			inRect,
								int32			inX,
								int32			inY);

	virtual void			SetCursor(
								MCursor			inCursor);
	virtual void			ObscureCursor();

  protected:

							MWindow(
								MWindowImpl*	inImpl);

	virtual bool			DoClose();

	void					SetImpl(
								MWindowImpl*	inImpl);

  private:

	void					TransitionTo(
								MRect			inPosition);

	virtual void			ShowSelf();
	virtual void			HideSelf();

	MWindowImpl*			mImpl;
	std::string				mTitle;
	bool					mModified;
	MHandler*				mFocus;

	static std::list<MWindow*>
							sWindowList;
};

#endif // MWINDOW_H
