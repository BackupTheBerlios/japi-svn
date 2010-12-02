//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include "MControlsImpl.h"
#include "MWindow.h"

using namespace std;

template<class IMPL>
MControl<IMPL>::MControl(const string& inID, MRect inBounds, IMPL* inImpl)
	: MControlBase(inID, inBounds)
	, mImpl(inImpl)
{
}

template<class IMPL>
MControl<IMPL>::~MControl()
{
}

template<class IMPL>
void MControl<IMPL>::Focus()
{
	mImpl->Focus();
}

template<class IMPL>
void MControl<IMPL>::Draw(
	MRect			inUpdate)
{
	mImpl->Draw(inUpdate);
}

template<class IMPL>
void MControl<IMPL>::ResizeFrame(int32 inXDelta, int32 inYDelta,
	int32 inWidthDelta, int32 inHeightDelta)
{
	MView::ResizeFrame(inXDelta, inYDelta, inWidthDelta, inHeightDelta);
	mImpl->FrameResized();
}

template<class IMPL>
void MControl<IMPL>::ActivateSelf()
{
	mImpl->ActivateSelf();
}

template<class IMPL>
void MControl<IMPL>::DeactivateSelf()
{
	mImpl->DeactivateSelf();
}

template<class IMPL>
void MControl<IMPL>::EnableSelf()
{
	mImpl->EnableSelf();
}

template<class IMPL>
void MControl<IMPL>::DisableSelf()
{
	mImpl->DisableSelf();
}

template<class IMPL>
void MControl<IMPL>::ShowSelf()
{
	mImpl->ShowSelf();
}

template<class IMPL>
void MControl<IMPL>::HideSelf()
{
	mImpl->HideSelf();
}

template<class IMPL>
void MControl<IMPL>::AddedToWindow()
{
	SetSuper(GetWindow());
	mImpl->AddedToWindow();
}

template<class IMPL>
MControlImplBase* MControl<IMPL>::GetImplBase() const
{
	MControlImplBase* impl = mImpl;
	return impl;
}

// --------------------------------------------------------------------

MButton::MButton(const string& inID, MRect inBounds, const string& inLabel)
	: MControl<MButtonImpl>(inID, inBounds, MButtonImpl::Create(this, inLabel))
{
}

void MButton::SimulateClick()
{
	mImpl->SimulateClick();
}

void MButton::MakeDefault(bool inDefault)
{
	mImpl->MakeDefault(inDefault);
}

// --------------------------------------------------------------------

MScrollbar::MScrollbar(const string& inID, MRect inBounds)
	: MControl<MScrollbarImpl>(inID, inBounds, MScrollbarImpl::Create(this))
{
}

int32 MScrollbar::GetValue() const
{
	return mImpl->GetValue();
}

void MScrollbar::SetValue(int32 inValue)
{
	mImpl->SetValue(inValue);
}

int32 MScrollbar::GetTrackValue() const
{
	return mImpl->GetTrackValue();
}

void MScrollbar::SetAdjustmentValues(int32 inMinValue, int32 inMaxValue,
	int32 inScrollUnit, int32 inPageSize, int32 inValue)
{
	mImpl->SetAdjustmentValues(inMinValue, inMaxValue,
		inScrollUnit, inPageSize, inValue);
}

//void MScrollbar::SetMinValue(int32 inMinValue)
//{
//	mImpl->SetMinValue(inMinValue);
//}

int32 MScrollbar::GetMinValue() const
{
	return mImpl->GetMinValue();
}
	
//void MScrollbar::SetMaxValue(int32 inMaxValue)
//{
//	mImpl->SetMaxValue(inMaxValue);
//}

int32 MScrollbar::GetMaxValue() const
{
	return mImpl->GetMaxValue();
}

//void MScrollbar::SetViewSize(int32 inViewSize)
//{
//	mImpl->SetViewSize(inViewSize);
//}

// --------------------------------------------------------------------

MStatusbar::MStatusbar(const string& inID, MRect inBounds, uint32 inPartCount, int32 inPartWidths[])
	: MControl<MStatusbarImpl>(inID, inBounds, MStatusbarImpl::Create(this, inPartCount, inPartWidths))
{
	SetBindings(true, false, true, true);
}

void MStatusbar::SetStatusText(uint32 inPartNr, const string& inText, bool inBorder)
{
	mImpl->SetStatusText(inPartNr, inText, inBorder);
}

// --------------------------------------------------------------------

MCombobox::MCombobox(const string& inID, MRect inBounds)
	: MControl<MComboboxImpl>(inID, inBounds, MComboboxImpl::Create(this))
{
}

void MCombobox::SetText(const string& inText)
{
	mImpl->SetText(inText);
}

string MCombobox::GetText() const
{
	return mImpl->GetText();
}

void MCombobox::SetChoices(const vector<string>& inChoices)
{
	mImpl->SetChoices(inChoices);
}

// --------------------------------------------------------------------

MPopup::MPopup(const string& inID, MRect inBounds)
	: MControl<MPopupImpl>(inID, inBounds, MPopupImpl::Create(this))
{
}

void MPopup::SetValue(int32 inValue)
{
	mImpl->SetValue(inValue);
}

int32 MPopup::GetValue() const
{
	return mImpl->GetValue();
}

void MPopup::SetChoices(const vector<string>& inChoices)
{
	mImpl->SetChoices(inChoices);
}

// --------------------------------------------------------------------

MEdittext::MEdittext(const string& inID, MRect inBounds)
	: MControl<MEdittextImpl>(inID, inBounds, MEdittextImpl::Create(this))
{
}

void MEdittext::SetText(const string& inText)
{
	mImpl->SetText(inText);
}

string MEdittext::GetText() const
{
	return mImpl->GetText();
}

void MEdittext::SetPasswordChar(uint32 inUnicode)
{
	mImpl->SetPasswordChar(inUnicode);
}

// --------------------------------------------------------------------

MCaption::MCaption(const string& inID, MRect inBounds, const string& inText)
	: MControl<MCaptionImpl>(inID, inBounds, MCaptionImpl::Create(this, inText))
{
}

void MCaption::SetText(const string& inText)
{
	mImpl->SetText(inText);
}

// --------------------------------------------------------------------

MSeparator::MSeparator(const string& inID, MRect inBounds)
	: MControl<MSeparatorImpl>(inID, inBounds, MSeparatorImpl::Create(this))
{
}

// --------------------------------------------------------------------

MCheckbox::MCheckbox(const string& inID, MRect inBounds, const string& inTitle)
	: MControl<MCheckboxImpl>(inID, inBounds, MCheckboxImpl::Create(this, inTitle))
{
}

bool MCheckbox::IsChecked() const
{
	return mImpl->IsChecked();
}

void MCheckbox::SetChecked(bool inChecked)
{
	mImpl->SetChecked(inChecked);
}

// --------------------------------------------------------------------

MListHeader::MListHeader(const std::string& inID, MRect inBounds)
	: MControl(inID, inBounds, MListHeaderImpl::Create(this))
{
}

void MListHeader::AppendColumn(const string& inLabel, int inWidth)
{
	mImpl->AppendColumn(inLabel, inWidth);
}

// --------------------------------------------------------------------

MNotebook::MNotebook(const string& inID, MRect inBounds)
	: MControl<MNotebookImpl>(inID, inBounds, MNotebookImpl::Create(this))
{
}

void MNotebook::AddPage(const string& inLabel, MView* inPage)
{
	mImpl->AddPage(inLabel, inPage);
}

void MNotebook::SelectPage(uint32 inPage)
{
	mImpl->SelectPage(inPage);
}

uint32 MNotebook::GetSelectedPage() const
{
	return mImpl->GetSelectedPage();
}

