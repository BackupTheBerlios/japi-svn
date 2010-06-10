//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include "MControlsImpl.h"

using namespace std;

template<class IMPL>
MControl<IMPL>::MControl(const string& inID, MRect inBounds, IMPL* inImpl)
	: MView(inID, inBounds)
	, MHandler(nil)
	, mImpl(inImpl)
{
}

template<class IMPL>
MControl<IMPL>::~MControl()
{
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

//template<class IMPL>
//long MControl<IMPL>::GetValue() const
//{
//	return mImpl->GetValue();
//}
//
//void MControl::SetValue(long inValue)
//{
//	dynamic_cast<MValueControlImplMixin*>(mImpl)->SetValue(inValue);
//}
//	
//void MControl::SetMinValue(long inMinValue)
//{
//	dynamic_cast<MValueControlImplMixin*>(mImpl)->SetMinValue(inMinValue);
//}
//
//long MControl::GetMinValue() const
//{
//	return dynamic_cast<MValueControlImplMixin*>(mImpl)->GetMinValue();
//}
//	
//void MControl::SetMaxValue(long inMaxValue)
//{
//	dynamic_cast<MValueControlImplMixin*>(mImpl)->SetMaxValue(inMaxValue);
//}
//
//long MControl::GetMaxValue() const
//{
//	return dynamic_cast<MValueControlImplMixin*>(mImpl)->GetMaxValue();
//}
//
//string MControl::GetText() const
//{
//	return dynamic_cast<MTextControlImplMixin*>(mImpl)->GetText();
//}
//
//void MControl::SetText(string inText)
//{
//	dynamic_cast<MTextControlImplMixin*>(mImpl)->SetText(inText);
//}

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
	mImpl->AddedToWindow();
}

// --------------------------------------------------------------------

MButton::MButton(const string& inID, MRect inBounds, const string& inLabel)
	: MControl<MButtonImpl>(inID, inBounds, MButtonImpl::Create(this, inLabel))
{
}

void MButton::GetIdealSize(int32& outWidth, int32& outHeight)
{
	mImpl->GetIdealSize(outWidth, outHeight);
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
	
void MScrollbar::SetMinValue(int32 inMinValue)
{
	mImpl->SetMinValue(inMinValue);
}

int32 MScrollbar::GetMinValue() const
{
	return mImpl->GetMinValue();
}
	
void MScrollbar::SetMaxValue(int32 inMaxValue)
{
	mImpl->SetMaxValue(inMaxValue);
}

int32 MScrollbar::GetMaxValue() const
{
	return mImpl->GetMaxValue();
}

void MScrollbar::SetViewSize(int32 inViewSize)
{
	mImpl->SetViewSize(inViewSize);
}

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

MCombobox::MCombobox(const string& inID, MRect inBounds, bool inEditable)
	: MControl<MComboboxImpl>(inID, inBounds, MComboboxImpl::Create(this, inEditable))
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

MCaption::MCaption(const string& inID, MRect inBounds, const string& inText)
	: MControl<MCaptionImpl>(inID, inBounds, MCaptionImpl::Create(this, inText))
{
}
