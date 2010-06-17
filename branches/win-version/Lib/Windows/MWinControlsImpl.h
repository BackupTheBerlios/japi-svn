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

	virtual void	Focus();

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

	virtual bool	WMGetDlgCode(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);

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

	virtual void	GetIdealSize(int32& outWidth, int32& outHeight);

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);

	virtual bool	WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMGetDlgCode(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);

	virtual void	AddedToWindow();

private:
	bool			mDefault;
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

class MWinComboboxImpl : public MWinControlImpl<MCombobox>
{
public:
					MWinComboboxImpl(MCombobox* inCombobox);
	
	virtual void	SetChoices(const std::vector<std::string>& inChoices);

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);
	virtual void	AddedToWindow();

	virtual bool	DispatchKeyDown(uint32 inKeyCode, uint32 inModifiers,
						const std::string& inText);

	virtual bool	WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseWheel(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult);

private:

	class MEditPart : public MWinProcMixin
	{
	public:
							MEditPart(MWinComboboxImpl* inImpl)
								: mImpl(inImpl)								{}
		virtual bool		DispatchKeyDown(uint32 inKeyCode, uint32 inModifiers,
								const std::string& inText)
							{
								return mImpl->DispatchKeyDown(inKeyCode, inModifiers, inText);
							}

	private:
		MWinComboboxImpl*	mImpl;
	};
	
	MEditPart		mEditor;
	std::vector<std::string>
					mChoices;
};

class MWinPopupImpl : public MWinControlImpl<MPopup>
{
public:
					MWinPopupImpl(MPopup* inPopup);
	
	virtual void	SetChoices(const std::vector<std::string>& inChoices);

	virtual int32	GetValue() const;
	virtual void	SetValue(int32 inValue);

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);
	virtual void	AddedToWindow();

	virtual bool	DispatchKeyDown(uint32 inKeyCode, uint32 inModifiers,
						const std::string& inText);

	virtual bool	WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
	virtual bool	WMMouseWheel(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult);

private:
	std::vector<std::string>
					mChoices;
};

class MWinEdittextImpl : public MWinControlImpl<MEdittext>
{
public:
					MWinEdittextImpl(MEdittext* inEdittext);
	
	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);

	virtual std::string
					GetText() const;
	virtual void	SetText(const std::string& inText);

	virtual bool	DispatchKeyDown(uint32 inKeyCode, uint32 inModifiers,
						const std::string& inText);

	virtual bool	WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
};

class MWinCaptionImpl : public MWinControlImpl<MCaption>
{
public:
					MWinCaptionImpl(MCaption* inControl, const std::string& inText);

	virtual void	SetText(const std::string& inText);

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);
};

class MWinSeparatorImpl : public MWinControlImpl<MSeparator>
{
public:
					MWinSeparatorImpl(MSeparator* inControl);

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);
};

class MWinCheckboxImpl : public MWinControlImpl<MCheckbox>
{
public:
					MWinCheckboxImpl(MCheckbox* inControl, const std::string& inText);

	virtual bool	IsChecked() const;
	virtual void	SetChecked(bool inChecked);

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);

	virtual bool	WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);
};

#endif
