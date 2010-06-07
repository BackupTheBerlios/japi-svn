//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCONTROLSIMPL_H
#define MCONTROLSIMPL_H

#include "MControls.h"

template<class CONTROL>
class MControlImpl
{
public:
					MControlImpl(CONTROL* inControl)
						: mControl(inControl)					{}
	virtual			~MControlImpl()								{}

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

protected:
	CONTROL*		mControl;
};

class MScrollbarImpl : public MControlImpl<MScrollbar>
{
public:
					MScrollbarImpl(MScrollbar* inControl)
						: MControlImpl(inControl)				{}

	virtual int32	GetValue() const = 0;
	virtual void	SetValue(int32 inValue) = 0;
	virtual int32	GetMinValue() const = 0;
	virtual void	SetMinValue(int32 inValue) = 0;
	virtual int32	GetMaxValue() const = 0;
	virtual void	SetMaxValue(int32 inValue) = 0;

	virtual void	SetViewSize(int32 inViewSize) = 0;

	static MScrollbarImpl*
					Create(MScrollbar* inControl);
};

class MButtonImpl : public MControlImpl<MButton>
{
public:
					MButtonImpl(MButton* inButton)
						: MControlImpl(inButton)				{}

	virtual void	SimulateClick() = 0;
	virtual void	MakeDefault(bool inDefault) = 0;

	static MButtonImpl*
					Create(MButton* inButton, const std::string& inLabel);
};

class MStatusbarImpl : public MControlImpl<MStatusbar>
{
public:
					MStatusbarImpl(MStatusbar* inStatusbar)
						: MControlImpl(inStatusbar)				{}

	virtual void	SetStatusText(uint32 inPartNr, const std::string& inText, bool inBorder) = 0;

	static MStatusbarImpl*
					Create(MStatusbar* inStatusbar, uint32 inPartCount, int32 inPartWidths[]);
};

#endif