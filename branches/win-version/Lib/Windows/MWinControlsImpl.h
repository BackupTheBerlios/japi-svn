//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINCONTROLSIMPL_H
#define MWINCONTROLSIMPL_H

#include "MControlsImpl.h"
#include "MWinProcMixin.h"

template<class CONTROL>
class MWinControlImpl : public CONTROL::MImpl, public MWinProcMixin
{
public:
					MWinControlImpl(CONTROL* inControl, const std::string& inLabel);
	virtual			~MWinControlImpl();

	virtual void	GetParentAndBounds(MWinProcMixin*& outParent, MRect& outBounds);

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

protected:
	std::string		mLabel;
};

// actual implementations

class MWinButtonImpl : public MWinControlImpl<MButton>
{
public:
					MWinButtonImpl(MButton* inButton, const std::string& inLabel);

	virtual void	SimulateClick();
	virtual void	MakeDefault(bool inDefault);

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);

	virtual bool	WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
};

class MWinScrollbarImpl : public MWinControlImpl<MScrollbar>
{
public:
					MWinScrollbarImpl(MScrollbar* inScrollbar);

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);

	virtual void	ShowSelf();
	virtual void	HideSelf();

	virtual int32	GetValue() const;
	virtual void	SetValue(int32 inValue);
	virtual int32	GetMinValue() const;
	virtual void	SetMinValue(int32 inValue);
	virtual int32	GetMaxValue() const;
	virtual void	SetMaxValue(int32 inValue);

	virtual void	SetViewSize(int32 inValue);

	virtual bool	WMScroll(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
};

class MWinStatusbarImpl : public MWinControlImpl<MStatusbar>
{
public:
					MWinStatusbarImpl(MStatusbar* inControl, uint32 inPartCount, int32 inPartWidths[]);

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);

	virtual void	SetStatusText(uint32 inPartNr, const std::string& inText, bool inBorder);

	virtual void	AddedToWindow();

private:
	std::vector<int32>		mOffsets;
};

#endif
