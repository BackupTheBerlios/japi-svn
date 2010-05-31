//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINCANVASIMPL_H
#define MWINCANVASIMPL_H

#include "MCanvasImpl.h"
#include "MWinControlsImpl.h"

class MCanvas;

class MWinCanvasImpl : public MCanvasImpl, public MWinControlImpl
{
public:
					MWinCanvasImpl(MCanvas* inCanvas);
					~MWinCanvasImpl();

	virtual void	Create();

	ID2D1HwndRenderTarget*
					GetRenderTarget(ID2D1Factory* inD2DFactory);

	virtual void	FrameResized();

protected:

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);
	virtual void	RegisterParams(UINT& outStyle, HCURSOR& outCursor,
						HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground);

	virtual bool	WMPaint(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);

	ID2D1HwndRenderTarget*
					mRenderTarget;
};

#endif