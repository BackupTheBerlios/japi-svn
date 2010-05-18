//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINWINDOWIMPL_H
#define MWINWINDOWIMPL_H

#include <map>

#include "MWindowImpl.h"
#include "MWinProcMixin.h"

class MWinMenubar;

class MWinWindowImpl : public MWindowImpl, public MWinProcMixin
{
  public:
					MWinWindowImpl(MWindowFlags inFlags, MWindow* inWindow);
	virtual			~MWinWindowImpl();

	virtual void	Create(MRect inBounds, const std::wstring& inTitle);

	virtual void	SetTitle(std::string inTitle);
	virtual std::string
					GetTitle() const;

	virtual void	Show();
	virtual void	Hide();

	virtual bool	Visible() const;

	virtual void	Select();
	virtual void	Close();

	//virtual void	ActivateSelf();
	//virtual void	DeactivateSelf();
	//virtual void	BeFocus();
	//virtual void	SubFocusChanged();
	
	virtual void	SetGlobalBounds(MRect inBounds);
	virtual void	GetGlobalBounds(MRect& outBounds) const;
	
//	virtual void	Invalidate(const HRegion& inRegion);
//	virtual void	Validate(const HRegion& inRegion);
	virtual void	UpdateIfNeeded(bool inFlush);

	virtual void	ScrollBits(MRect inRect, int32 inDeltaH, int32 inDeltaV);
	
	virtual bool	GetMouse(int32& outX, int32& outY, unsigned long& outModifiers);
	virtual bool	WaitMouseMoved(int32 inX, int32 inY);

	//virtual void	ConvertToScreen(HPoint& ioPoint) const;
	//virtual void	ConvertFromScreen(HPoint& ioPoint) const;
	//virtual void	ConvertToScreen(HRect& ioRect) const;
	//virtual void	ConvertFromScreen(HRect& ioRect) const;

	MWindow*		GetWindow() const						{ return mWindow; }

  protected:

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);
	virtual void	RegisterParams(UINT& outStyle, HCURSOR& outCursor,
						HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground);

	virtual bool	WMClose(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMDestroy(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMActivate(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseActivate(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMSize(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMSizing(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMPaint(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMInitMenu(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseDown(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseWheel(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMScroll(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMSetFocus(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMContextMenu(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMSetCursor(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
//	virtual bool	WMImeComposition(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
//	virtual bool	WMImeStartComposition(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMImeRequest(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMQueryEndSession(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMDropFiles(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMThemeChanged(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);

	HWND			mSizeBox;
	HWND			mStatus;
	std::string		mTitle;
	int32			mMinWidth, mMinHeight;
	MWinMenubar*	mMenubar;
};

#endif