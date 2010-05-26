//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCONTROLS_H
#define MCONTROLS_H

#include "MView.h"
#include "MHandler.h"
#include "MP2PEvents.h"

class MControlImpl;

class MControl : public MView, public MHandler
{
public:

	virtual			~MControl();

	virtual void	ResizeFrame(
						int32			inXDelta,
						int32			inYDelta,
						int32			inWidthDelta,
						int32			inHeightDelta);

	virtual void	Draw(
						MRect			inUpdate);

	virtual long	GetValue() const;
	virtual void	SetValue(long inValue);
	
	void			SetMinValue(long inMinValue);
	long			GetMinValue() const;
	
	void			SetMaxValue(long inMaxValue);
	long			GetMaxValue() const;

	virtual std::string
					GetText() const;
	virtual void	SetText(std::string inText);

	MControlImpl*	GetImpl() const					{ return mImpl; }

protected:

					MControl(uint32 inID, MRect inBounds, MControlImpl* inImpl);

	virtual void	ActivateSelf();
	virtual void	DeactivateSelf();
	
	virtual void	EnableSelf();
	virtual void	DisableSelf();
	
	virtual void	ShowSelf();
	virtual void	HideSelf();

	virtual void	AddedToWindow();

private:
					MControl(const MControl&);
	MControl&		operator=(const MControl&);

	MControlImpl*	mImpl;
};

class MButton : public MControl
{
public:
					MButton(uint32 inID, MRect inBounds, const std::string& inLabel);

	MEventOut<void()>
					eClicked;
};

extern const int kScrollBarWidth;

class MScrollbar : public MControl
{
public:
					MScrollbar(uint32 inID, MRect inBounds);

	MEventOut<void(MScrollMessage)>
					eScroll;
};

#endif