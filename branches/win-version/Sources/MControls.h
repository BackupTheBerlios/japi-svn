//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCONTROLS_H
#define MCONTROLS_H

#include "MView.h"
#include "MHandler.h"
#include "MP2PEvents.h"

template<class I>
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

	I*				GetImpl() const					{ return mImpl; }

protected:

					MControl(uint32 inID, MRect inBounds, I* inImpl);

	virtual void	ActivateSelf();
	virtual void	DeactivateSelf();
	
	virtual void	EnableSelf();
	virtual void	DisableSelf();
	
	virtual void	ShowSelf();
	virtual void	HideSelf();

	virtual void	AddedToWindow();

protected:
					MControl(const MControl&);
	MControl&		operator=(const MControl&);

	I*				mImpl;
};

class MButtonImpl;

class MButton : public MControl<MButtonImpl>
{
public:
	typedef MButtonImpl		MImpl;
	
					MButton(uint32 inID, MRect inBounds, const std::string& inLabel);

	MEventOut<void()>
					eClicked;
};

extern const int kScrollbarWidth;
class MScrollbarImpl;

class MScrollbar : public MControl<MScrollbarImpl>
{
public:
	typedef MScrollbarImpl		MImpl;

					MScrollbar(uint32 inID, MRect inBounds);

	virtual int32	GetValue() const;
	virtual void	SetValue(int32 inValue);
	
	void			SetMinValue(int32 inMinValue);
	int32			GetMinValue() const;
	
	void			SetMaxValue(int32 inMaxValue);
	int32			GetMaxValue() const;

	virtual void	SetViewSize(int32 inViewSize);

	MEventOut<void(MScrollMessage)>
					eScroll;
};

class MStatusbarImpl;

class MStatusbar : public MControl<MStatusbarImpl>
{
public:
	typedef MStatusbarImpl		MImpl;

					MStatusbar(uint32 inID, MRect inBounds, uint32 inPartCount, int32 inPartWidths[]);

	virtual void	SetStatusText(uint32 inPartNr, const std::string& inText, bool inBorder);
};

#endif