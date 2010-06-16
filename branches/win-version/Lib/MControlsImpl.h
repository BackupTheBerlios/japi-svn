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

	virtual void	Focus()										{}

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

	virtual void	GetIdealSize(int32& outWidth, int32& outHeight) = 0;

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

class MComboboxImpl : public MControlImpl<MCombobox>
{
public:
					MComboboxImpl(MCombobox* inCombobox)
						: MControlImpl(inCombobox) {}
	
	virtual void	SetText(const std::string& inText) = 0;
	virtual std::string
					GetText() const = 0;
	
	virtual void	SetChoices(const std::vector<std::string>& inChoices) = 0;

	static MComboboxImpl*
					Create(MCombobox* inCombobox);
};

class MPopupImpl : public MControlImpl<MPopup>
{
public:
					MPopupImpl(MPopup* inPopup)
						: MControlImpl(inPopup) {}
	
	virtual void	SetValue(int32 inValue) = 0;
	virtual int32	GetValue() const = 0;
	
	virtual void	SetChoices(const std::vector<std::string>& inChoices) = 0;

	static MPopupImpl*
					Create(MPopup* inPopup);
};

class MEdittextImpl : public MControlImpl<MEdittext>
{
public:
					MEdittextImpl(MEdittext* inEdittext)
						: MControlImpl(inEdittext) {}
	
	virtual void	SetText(const std::string& inText) = 0;
	virtual std::string
					GetText() const = 0;

	static MEdittextImpl*
					Create(MEdittext* inEdittext);
};

class MCaptionImpl : public MControlImpl<MCaption>
{
public:
					MCaptionImpl(MCaption* inCaption)
						: MControlImpl(inCaption)				{}

	static MCaptionImpl*
					Create(MCaption* inCaption, const std::string& inText);
};

class MSeparatorImpl : public MControlImpl<MSeparator>
{
public:
					MSeparatorImpl(MSeparator* inSeparator)
						: MControlImpl(inSeparator)				{}

	static MSeparatorImpl*
					Create(MSeparator* inSeparator);
};

class MCheckboxImpl : public MControlImpl<MCheckbox>
{
public:
					MCheckboxImpl(MCheckbox* inCheckbox)
						: MControlImpl(inCheckbox)				{}

	virtual bool	IsChecked() const = 0;
	virtual void	SetChecked(bool inChecked) = 0;

	static MCheckboxImpl*
					Create(MCheckbox* inCheckbox, const std::string& inTitle);
};

#endif
