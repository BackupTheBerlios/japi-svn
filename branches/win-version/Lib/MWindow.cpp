//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "MCommands.h"
#include "MWindow.h"
#include "MError.h"
#include "MApplication.h"
#include "MWindowImpl.h"

using namespace std;

// --------------------------------------------------------------------
//
//	MWindow
//

list<MWindow*> MWindow::sWindowList;

MWindow::MWindow(const string& inTitle, const MRect& inBounds,
		MWindowFlags inFlags, const string& inMenu)
	: MView("window", inBounds)
	, MHandler(gApp)
	, mImpl(MWindowImpl::Create(inTitle, inBounds, inFlags, inMenu, this))
	, mFocus(this)
{
	mBounds.x = mBounds.y = 0;

	SetBindings(true, true, true, true);
	
	sWindowList.push_back(this);
}

MWindow::MWindow(MWindowImpl* inImpl)
	: MView("window", MRect(0, 0, 100, 100))
	, MHandler(gApp)
	, mImpl(inImpl)
	, mFocus(this)
{
	SetBindings(true, true, true, true);

	sWindowList.push_back(this);
}

MWindow::~MWindow()
{
	sWindowList.erase(
		remove(sWindowList.begin(), sWindowList.end(), this),
		sWindowList.end());
}

void MWindow::SetImpl(
	MWindowImpl*	inImpl)
{
	if (mImpl != nil)
		delete mImpl;
	mImpl = inImpl;
}

MWindowFlags MWindow::GetFlags() const
{
	return mImpl->GetFlags();
}

MWindow* MWindow::GetFirstWindow()
{
	MWindow* result = nil;
	if (not sWindowList.empty())
		result = sWindowList.front();
	return result;
}

MWindow* MWindow::GetNextWindow() const
{
	MWindow* result = nil;

	list<MWindow*>::const_iterator w =
		find(sWindowList.begin(), sWindowList.end(), this);

	if (w != sWindowList.end())
	{
		++w;
		if (w != sWindowList.end())
			result = *w;
	}

	return result;
}

MWindow* MWindow::GetWindow() const
{
	return const_cast<MWindow*>(this);
}

void MWindow::Show()
{
	if (mVisible == eTriStateOff)
	{
		mVisible = eTriStateOn;
		MView::Show();
		ShowSelf();
	}
}

void MWindow::ShowSelf()
{
	mImpl->Show();
}

void MWindow::HideSelf()
{
	mImpl->Hide();
}

void MWindow::Select()
{
	if (not mImpl->Visible())
		mImpl->Show();
	mImpl->Select();
}

void MWindow::Activate()
{
	if (mActive == eTriStateOff and IsVisible())
	{
		mActive = eTriStateOn;
		ActivateSelf();
		MView::Activate();
	}

	if (not sWindowList.empty() and sWindowList.front() != this)
	{
		if (sWindowList.front()->IsActive())
			sWindowList.front()->Deactivate();

		sWindowList.erase(find(sWindowList.begin(), sWindowList.end(), this));
		sWindowList.push_front(this);
	}
}

void MWindow::UpdateNow()
{
	mImpl->UpdateNow();
}

bool MWindow::DoClose()
{
	return true;
}

void MWindow::Close()
{
	if (DoClose() and mImpl != nil)
		mImpl->Close();
}

void MWindow::SetTitle(
	const string&	inTitle)
{
	mTitle = inTitle;
	
	if (mModified)
		mImpl->SetTitle(mTitle + "*");
	else
		mImpl->SetTitle(mTitle);
}

string MWindow::GetTitle() const
{
	return mTitle;
}

void MWindow::SetModifiedMarkInTitle(
	bool		inModified)
{
	if (mModified != inModified)
	{
		mModified = inModified;
		SetTitle(mTitle);
	}
}

bool MWindow::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = true;	
	
	switch (inCommand)
	{
		case cmd_Close:
			outEnabled = true;
			break;
		
		default:
			result = MHandler::UpdateCommandStatus(
				inCommand, inMenu, inItemIndex, outEnabled, outChecked);
	}
	
	return result;
}

bool MWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;	
	
	switch (inCommand)
	{
		case cmd_Close:
			Close();
			break;
		
		default:
			result = MHandler::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
			break;
	}
	
	return result;
}

void MWindow::GetWindowPosition(
	MRect&			outPosition)
{
	mImpl->GetWindowPosition(outPosition);
}

void MWindow::SetWindowPosition(
	const MRect&	inPosition,
	bool			inTransition)
{
	mImpl->SetWindowPosition(inPosition, inTransition);
}

void MWindow::ConvertToScreen(int32& ioX, int32& ioY) const
{
	mImpl->ConvertToScreen(ioX, ioY);
}

void MWindow::ConvertFromScreen(int32& ioX, int32& ioY) const
{
	mImpl->ConvertFromScreen(ioX, ioY);
}

void MWindow::GetMouse(int32& outX, int32& outY, uint32& outModifiers) const
{
	mImpl->GetMouse(outX, outY, outModifiers);
}

void MWindow::Invalidate(MRect inRect)
{
	mImpl->Invalidate(inRect);
}

void MWindow::ScrollRect(MRect inRect, int32 inX, int32 inY)
{
	mImpl->ScrollRect(inRect, inX, inY);
}

void MWindow::SetCursor(
	MCursor			inCursor)
{
	mImpl->SetCursor(inCursor);
}

void MWindow::ObscureCursor()
{
	mImpl->ObscureCursor();
}
