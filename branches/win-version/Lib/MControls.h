//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCONTROLS_H
#define MCONTROLS_H

#include "MView.h"
#include "MHandler.h"
#include "MP2PEvents.h"

class MControlImplBase;

class MControlBase : public MView, public MHandler
{
  public:
					MControlBase(const std::string& inID, MRect inBounds)
						: MView(inID, inBounds)
						, MHandler(nil) {}

  virtual MControlImplBase*
					GetImplBase() const = 0;
};

template<class I>
class MControl : public MControlBase
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

	virtual void	Focus();

	I*				GetImpl() const					{ return mImpl; }

	virtual MControlImplBase*
					GetImplBase() const;

  protected:

					MControl(const std::string& inID, MRect inBounds, I* inImpl);

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

// --------------------------------------------------------------------

class MButtonImpl;

class MButton : public MControl<MButtonImpl>
{
  public:
	typedef MButtonImpl		MImpl;
	
					MButton(const std::string& inID, MRect inBounds, const std::string& inLabel);

	void			SimulateClick();
	void			MakeDefault(bool inDefault = true);

	MEventOut<void(const std::string&)>
					eClicked;
};

// --------------------------------------------------------------------

extern const int kScrollbarWidth;
class MScrollbarImpl;

class MScrollbar : public MControl<MScrollbarImpl>
{
  public:
	typedef MScrollbarImpl		MImpl;

					MScrollbar(const std::string& inID, MRect inBounds);

	virtual int32	GetValue() const;
	virtual void	SetValue(int32 inValue);

	virtual int32	GetTrackValue() const;
	virtual int32	GetMinValue() const;
	virtual int32	GetMaxValue() const;
	
	virtual void	SetAdjustmentValues(int32 inMinValue, int32 inMaxValue,
						int32 inScrollUnit, int32 inPageSize, int32 inValue);

	MEventOut<void(MScrollMessage)>
					eScroll;
};

// --------------------------------------------------------------------

class MStatusbarImpl;

class MStatusbar : public MControl<MStatusbarImpl>
{
  public:
	typedef MStatusbarImpl		MImpl;

					MStatusbar(const std::string& inID, MRect inBounds, uint32 inPartCount, int32 inPartWidths[]);

	virtual void	SetStatusText(uint32 inPartNr, const std::string& inText, bool inBorder);
};

// --------------------------------------------------------------------

class MComboboxImpl;

class MCombobox : public MControl<MComboboxImpl>
{
  public:
	typedef MComboboxImpl		MImpl;
		
					MCombobox(const std::string& inID, MRect inBounds);

	MEventOut<void(const std::string&,const std::string&)>
					eValueChanged;
	
	virtual void	SetText(const std::string& inText);
	virtual std::string
					GetText() const;
	
	virtual void	SetChoices(const std::vector<std::string>& inChoices);
};

// --------------------------------------------------------------------

class MPopupImpl;

class MPopup : public MControl<MPopupImpl>
{
  public:
	typedef MPopupImpl		MImpl;
		
					MPopup(const std::string& inID, MRect inBounds);

	MEventOut<void(const std::string&,int32)>
					eValueChanged;
	
	virtual void	SetValue(int32 inValue);
	virtual int32	GetValue() const;
	
	virtual void	SetChoices(const std::vector<std::string>& inChoices);
};

// --------------------------------------------------------------------

class MCaptionImpl;

class MCaption : public MControl<MCaptionImpl>
{
  public:
	typedef MCaptionImpl		MImpl;
		
					MCaption(const std::string& inID, MRect inBounds,
						const std::string& inText);

	virtual void	SetText(const std::string& inText);
};

// --------------------------------------------------------------------

class MEdittextImpl;

class MEdittext : public MControl<MEdittextImpl>
{
  public:
	typedef MEdittextImpl		MImpl;
		
					MEdittext(const std::string& inID, MRect inBounds);

	MEventOut<void(const std::string&,const std::string&)>
					eValueChanged;
	
	virtual void	SetText(const std::string& inText);
	virtual std::string
					GetText() const;

	virtual void	SetPasswordChar(uint32 inUnicode = 0x2022);
};

// --------------------------------------------------------------------

class MSeparatorImpl;

class MSeparator : public MControl<MSeparatorImpl>
{
  public:
	typedef MSeparatorImpl		MImpl;
		
					MSeparator(const std::string& inID, MRect inBounds);
};

// --------------------------------------------------------------------

class MCheckboxImpl;

class MCheckbox : public MControl<MCheckboxImpl>
{
  public:
	typedef MCheckboxImpl		MImpl;
		
					MCheckbox(const std::string& inID, MRect inBounds,
						const std::string& inTitle);

	bool			IsChecked() const;
	void			SetChecked(bool inChecked);

	MEventOut<void(const std::string&,bool)>
					eValueChanged;
};

// --------------------------------------------------------------------

class MListHeaderImpl;

class MListHeader : public MControl<MListHeaderImpl>
{
  public:
	typedef MListHeaderImpl		MImpl;
					MListHeader(const std::string& inID, MRect inBounds);
	
	MEventOut<void(uint32,uint32)>			eColumnResized;

	void			AppendColumn(const std::string& inLabel, int inWidth = -1);
};

// --------------------------------------------------------------------

class MNotebookImpl;

class MNotebook : public MControl<MNotebookImpl>
{
  public:
	typedef MNotebookImpl		MImpl;
		
					MNotebook(const std::string& inID, MRect inBounds);

	void			AddPage(const std::string& inLabel, MView* inPage);

	void			SelectPage(uint32 inPage);
	uint32			GetSelectedPage() const;

	MEventOut<void(uint32)>
					ePageSelected;
};

#endif
