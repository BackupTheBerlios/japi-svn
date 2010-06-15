//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINWINDOWIMPL_H
#define MWINWINDOWIMPL_H

#include <map>

#include "MWindowImpl.h"
#include "MWinProcMixin.h"

interface ID2D1HwndRenderTarget;

class MWinWindowImpl : public MWindowImpl, public MWinProcMixin
{
  public:
					MWinWindowImpl(MWindowFlags inFlags,
						const std::string& inMenu, MWindow* inWindow);
	virtual			~MWinWindowImpl();

	virtual void	Create(MRect inBounds, const std::wstring& inTitle);

	virtual void	SetTitle(std::string inTitle);
	//virtual std::string
	//				GetTitle() const;

	virtual void	Show();
	virtual void	Hide();

	virtual bool	Visible() const;

	virtual void	Select();
	virtual void	Close();

	//virtual void	ActivateSelf();
	//virtual void	DeactivateSelf();
	//virtual void	BeFocus();
	//virtual void	SubFocusChanged();
	
	virtual void	SetWindowPosition(MRect inBounds, bool inTransition);
	virtual void	GetWindowPosition(MRect& outBounds) const;
	
	virtual void	Invalidate(MRect inRect);
	virtual void	Validate(MRect inRect);
	virtual void	UpdateNow();

	virtual void	ScrollRect(MRect inRect, int32 inDeltaH, int32 inDeltaV);
	
	virtual bool	GetMouse(int32& outX, int32& outY, uint32& outModifiers);
	virtual bool	WaitMouseMoved(int32 inX, int32 inY);

	virtual void	SetCursor(MCursor inCursor);

	virtual void	ConvertToScreen(int32& ioX, int32& ioY) const;
	virtual void	ConvertFromScreen(int32& ioX, int32& ioY) const;

	MWindow*		GetWindow() const						{ return mWindow; }

	ID2D1RenderTarget*
					GetRenderTarget();
	void			ReleaseRenderTarget();

	bool			IsDialogMessage(MSG& inMesssage);

  protected:

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);
	virtual void	RegisterParams(UINT& outStyle, int& outWndExtra, HCURSOR& outCursor,
						HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground);

	virtual bool	WMClose(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMDestroy(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMActivate(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseActivate(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMSize(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMSizing(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMPaint(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMEraseBkgnd(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMInitMenu(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMenuCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseDown(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseMove(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseExit(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseUp(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
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

	virtual bool	DispatchKeyDown(uint32 inKeyCode, uint32 inModifiers,
						const std::string& inText);

	HWND			mSizeBox;
	HWND			mStatus;
	int32			mMinWidth, mMinHeight;
	MMenu*			mMenubar;
	int32			mLastGetMouseX, mLastGetMouseY;
//	ID2D1HwndRenderTarget*
//					mRenderTarget;
	ID2D1DCRenderTarget*
					mRenderTarget;
	MView*			mMousedView;
	uint32			mClickCount;
	double			mLastClickTime;
};

#endif
