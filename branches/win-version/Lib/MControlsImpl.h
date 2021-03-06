//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCONTROLSIMPL_H
#define MCONTROLSIMPL_H

#include "MControls.h"

class MWinProcMixin;

class MControlImplBase
{
  public:

	virtual MWinProcMixin*
					GetWinProcMixin()							{ return nil; }
};

template<class CONTROL>
class MControlImpl : public MControlImplBase
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

	virtual int32	GetTrackValue() const = 0;

	virtual void	SetAdjustmentValues(int32 inMinValue, int32 inMaxValue,
						int32 inScrollUnit, int32 inPageSize, int32 inValue) = 0;

	virtual int32	GetMinValue() const = 0;
//	virtual void	SetMinValue(int32 inValue) = 0;
	virtual int32	GetMaxValue() const = 0;
//	virtual void	SetMaxValue(int32 inValue) = 0;
//
//	virtual void	SetViewSize(int32 inViewSize) = 0;

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

	virtual void	SetPasswordChar(uint32 inUnicode) = 0;

	static MEdittextImpl*
					Create(MEdittext* inEdittext);
};

class MCaptionImpl : public MControlImpl<MCaption>
{
public:
					MCaptionImpl(MCaption* inCaption)
						: MControlImpl(inCaption)				{}

	virtual void	SetText(const std::string& inText) = 0;

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

class MListHeaderImpl : public MControlImpl<MListHeader>
{
  public:
					MListHeaderImpl(MListHeader* inListHeader)
						: MControlImpl(inListHeader) {}
	
	virtual void	AppendColumn(const std::string& inLabel, int inWidth) = 0;

	static MListHeaderImpl*
					Create(MListHeader* inListHeader);
};

class MNotebookImpl : public MControlImpl<MNotebook>
{
public:
					MNotebookImpl(MNotebook* inNotebook)
						: MControlImpl(inNotebook)				{}

	virtual void	AddPage(const std::string& inLabel, MView* inPage) = 0;

	virtual void	SelectPage(uint32 inPage) = 0;
	virtual uint32	GetSelectedPage() const = 0;

	static MNotebookImpl*
					Create(MNotebook* inNotebook);
};

#endif
