//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINCONTROLSIMPL_H
#define MWINCONTROLSIMPL_H

#include "MControlsImpl.h"
#include "MWinProcMixin.h"

class MWinControlImpl : public MControlImpl, public MWinProcMixin
{
public:
					MWinControlImpl(MControl* inControl, const std::string& inLabel = "");
	virtual			~MWinControlImpl();

	virtual void	GetParentAndBounds(MWinProcMixin*& outParent, MRect& outBounds);

	static MWinControlImpl*
					FetchControlImpl(HWND inHWND);

	virtual void	AddedToWindow();
	virtual void	FrameResized();
	virtual void	Draw(MRect inBounds);
	virtual void	ActivateSelf();
	virtual void	DeactivateSelf();
	virtual void	EnableSelf();
	virtual void	DisableSelf();
	virtual void	ShowSelf();
	virtual void	HideSelf();
	virtual std::string
					GetText() const;
	virtual void	SetText(const std::string& inText);

	virtual bool	WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);

protected:
	std::string		mLabel;
};

// actual implementations

class MButtonImpl : public MWinControlImpl
{
public:
					MButtonImpl(MControl* inControl, const std::string& inLabel);

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);

};

class MScrollbarImpl : public MWinControlImpl
{
public:
					MScrollbarImpl(MControl* inControl);

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);

	virtual void	ShowSelf();
	virtual void	HideSelf();

	virtual long	GetValue() const;
	virtual void	SetValue(long inValue);
	virtual long	GetMinValue() const;
	virtual void	SetMinValue(long inValue);
	virtual long	GetMaxValue() const;
	virtual void	SetMaxValue(long inValue);

	virtual void	SetViewSize(long inValue);

	virtual bool	WMScroll(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
};

#endif
