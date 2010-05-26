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

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);

	////virtual void	Embed(HNode* /*inParent*/)					{}
	virtual void	AddedToWindow();
	//virtual void	ResizeFrame(long,long,long,long)			{}
	//virtual void	Draw(MRect /*inBounds*/)					{}
	//virtual void	Click(int32 inX, int32 inY)					{}
	virtual void	ActivateSelf();
	virtual void	DeactivateSelf();
	virtual void	EnableSelf();
	virtual void	DisableSelf();
	virtual void	ShowSelf();
	virtual void	HideSelf();
	virtual std::string
					GetText() const;
	virtual void	SetText(const std::string& inText);
	virtual long	GetValue() const;
	virtual void	SetValue(long inValue);
	//virtual long	GetMinValue() const							{ assert(false); return 0; }
	//virtual void	SetMinValue(long /*inValue*/)				{ assert(false); }
	//virtual long	GetMaxValue() const							{ assert(false); return 0; }
	//virtual void	SetMaxValue(long /*inValue*/)				{ assert(false); }
	//virtual void	SimulateClick()								{}
	//virtual void	SetViewSize(long /*inViewSize*/)			{}
	//virtual void	Idle()										{}
	//
	//void			SetControl(MControl* inControl);
	//
	////virtual void	SetFont(const HFont& /*inFont*/)			{}
	//virtual void	MakeDefault(bool /*inDefault*/)				{}
	//
	////virtual bool	AllowBeFocus(HHandler* /*inNewFocus*/)		{ return false; }
	////virtual bool	AllowDontBeFocus(HHandler* /*inNewFocus*/)	{ return true; }
	////virtual void	BeFocus()									{}
	////virtual void	DontBeFocus()								{}

	////virtual void	InsertText(std::string /*inText*/)			{ assert(false); }
	////virtual void	SetPasswordChar(HUnicode inUnicode)			{ assert(false); }
	////
	////virtual int		CountItems() const							{ return 0; }
	////virtual std::string
	////				ItemAt(int /*index*/) const					{ assert(false); return kEmptyString; }
	////virtual void	AddItem(std::string /*inText*/)				{ assert(false); }
	////virtual void	AddSeparatorItem()							{ assert(false); }
	////virtual void	RemoveItems(int /*inFromIndex*/)			{ assert(false); }

	////virtual void	GetFontForSelection(HFont& /*outFont*/)		{ }
	////virtual void	SelectFont(const HFont& /*inFont*/)			{ }

	virtual bool	WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);

protected:
	long			mValue;
	std::string		mLabel;
};

// actual implementations

class MButtonImpl : public MWinControlImpl
{
public:
					MButtonImpl(MControl* inControl, const std::string& inLabel);

					virtual void	AddedToWindow();

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);

};

#endif
