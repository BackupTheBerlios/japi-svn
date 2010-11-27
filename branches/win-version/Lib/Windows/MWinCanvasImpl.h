//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINCANVASIMPL_H
#define MWINCANVASIMPL_H

#include "MWinProcMixin.h"
#include "MCanvasImpl.h"

class MWinCanvasImpl : public MCanvasImpl, public MWinProcMixin
{
  public:
					MWinCanvasImpl(
						MCanvas*		inCanvas);

	virtual 		~MWinCanvasImpl();
	
	ID2D1RenderTarget*
					GetRenderTarget();

	virtual void	ResizeFrame(
						int32			inXDelta,
						int32			inYDelta,
						int32			inWidthDelta,
						int32			inHeightDelta);

	virtual void	AddedToWindow();

  protected:

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);
	virtual void	RegisterParams(UINT& outStyle, int& outWndExtra, HCURSOR& outCursor,
						HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground);

	virtual bool	WMPaint(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMSize(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);

	virtual bool	WMMouseDown(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseMove(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseExit(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseUp(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);

	virtual bool	WMSetCursor(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	
  private:

	ID2D1HwndRenderTarget*
					mRenderTarget;
	bool			mInBeginEndDrawSection;

	double			mLastClickTime;
	uint32			mClickCount;
};

#endif
