//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCONTROLSIMPL_H
#define MCONTROLSIMPL_H

#include "MControls.h"

class MControlImpl
{
public:
					MControlImpl(MView* inControl)
						: mControl(inControl)					{}
	virtual			~MControlImpl()								{}

	//virtual void	Embed(HNode* /*inParent*/)					{}
	virtual void	AddedToWindow()								{}
	virtual void	FrameResized()								{}
	virtual void	Draw(MRect inBounds)						{}
	virtual void	Click(int32 inX, int32 inY)					{}
	virtual void	ActivateSelf()								{}
	virtual void	DeactivateSelf()							{}
	virtual void	EnableSelf()								{}
	virtual void	DisableSelf()								{}
	virtual void	ShowSelf()									{}
	virtual void	HideSelf()									{}
	virtual std::string
					GetText() const	= 0;
	virtual void	SetText(std::string /*inText*/)				{}
	virtual long	GetValue() const							{ return 0; }
	virtual void	SetValue(long inValue)						{}
	virtual long	GetMinValue() const							{ return 0; }
	virtual void	SetMinValue(long /*inValue*/)				{ }
	virtual long	GetMaxValue() const							{ return 0; }
	virtual void	SetMaxValue(long /*inValue*/)				{ }
	virtual void	SimulateClick()								{}
	virtual void	SetViewSize(long /*inViewSize*/)			{}
	virtual void	Idle()										{}
	
	void			SetControl(MControl* inControl);
	
	//virtual void	SetFont(const HFont& /*inFont*/)			{}
	virtual void	MakeDefault(bool /*inDefault*/)				{}
	
	//virtual bool	AllowBeFocus(HHandler* /*inNewFocus*/)		{ return false; }
	//virtual bool	AllowDontBeFocus(HHandler* /*inNewFocus*/)	{ return true; }
	//virtual void	BeFocus()									{}
	//virtual void	DontBeFocus()								{}

	//virtual void	InsertText(std::string /*inText*/)			{ assert(false); }
	//virtual void	SetPasswordChar(HUnicode inUnicode)			{ assert(false); }
	//
	//virtual int		CountItems() const							{ return 0; }
	//virtual std::string
	//				ItemAt(int /*index*/) const					{ assert(false); return kEmptyString; }
	//virtual void	AddItem(std::string /*inText*/)				{ assert(false); }
	//virtual void	AddSeparatorItem()							{ assert(false); }
	//virtual void	RemoveItems(int /*inFromIndex*/)			{ assert(false); }

	//virtual void	GetFontForSelection(HFont& /*outFont*/)		{ }
	//virtual void	SelectFont(const HFont& /*inFont*/)			{ }

	static MControlImpl*	CreateButton(MControl* inControl, const std::string& inLabel);
	static MControlImpl*	CreateScrollbar(MControl* inControl);

protected:
	MView*			mControl;
};

#endif
