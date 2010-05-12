//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINDOWIMPL_H
#define MWINDOWIMPL_H

#include "MWindow.h"

class MWindowImpl
{
  public:
	static MWindowImpl*	Create(std::string inTitle, MRect inBounds, MWindowFlags inFlags, MWindow* inWindow);
	  
	virtual				~MWindowImpl() {}

	virtual void		SetTitle(std::string inTitle) = 0;
	virtual std::string	GetTitle() const = 0;

	virtual void		Show() = 0;
	virtual void		Hide() = 0;

	virtual bool		Visible() const	= 0;

	virtual void		Select() = 0;
	virtual void		Close() = 0;

	//virtual void		ActivateSelf();
	//virtual void		DeactivateSelf();
	//virtual void		BeFocus();
	//virtual void		SubFocusChanged();
	
	virtual void		SetGlobalBounds(MRect inBounds) = 0;
	virtual void		GetGlobalBounds(MRect& outBounds) const = 0;
	
//	virtual void		Invalidate(const HRegion& inRegion) = 0;
//	virtual void		Validate(const HRegion& inRegion) = 0;
	virtual void		UpdateIfNeeded(bool inFlush) = 0;

	virtual void		ScrollBits(MRect inRect, int32 inDeltaH, int32 inDeltaV) = 0;
	
	virtual bool		GetMouse(int32& outX, int32& outY, unsigned long& outModifiers) = 0;
	virtual bool		WaitMouseMoved(int32 inX, int32 inY) = 0;

	//virtual void		ConvertToScreen(HPoint& ioPoint) const = 0;
	//virtual void		ConvertFromScreen(HPoint& ioPoint) const = 0;
	//virtual void		ConvertToScreen(HRect& ioRect) const = 0;
	//virtual void		ConvertFromScreen(HRect& ioRect) const = 0;

  protected:
						MWindowImpl(MWindowFlags inFlags, MWindow* inWindow)
							: mWindow(inWindow), mFlags(inFlags) {}
	  
	MWindow*			mWindow;
	MWindowFlags		mFlags;
};

#endif