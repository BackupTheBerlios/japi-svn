//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include "MControlsImpl.h"

using namespace std;

MControl::MControl(uint32 inID, MRect inBounds, MControlImpl* inImpl)
	: MView(inID, inBounds)
	, MHandler(nil)
	, mImpl(inImpl)
{
}

MControl::~MControl()
{
}

void MControl::Draw(
	MRect			inUpdate)
{
	mImpl->Draw(inUpdate);
}

void MControl::ResizeFrame(int32 inXDelta, int32 inYDelta,
	int32 inWidthDelta, int32 inHeightDelta)
{
	MView::ResizeFrame(inXDelta, inYDelta, inWidthDelta, inHeightDelta);
	mImpl->FrameResized();
}

long MControl::GetValue() const
{
	return mImpl->GetValue();
}

void MControl::SetValue(long inValue)
{
	mImpl->SetValue(inValue);
}
	
void MControl::SetMinValue(long inMinValue)
{
	mImpl->SetMinValue(inMinValue);
}

long MControl::GetMinValue() const
{
	return mImpl->GetMinValue();
}
	
void MControl::SetMaxValue(long inMaxValue)
{
	mImpl->SetMaxValue(inMaxValue);
}

long MControl::GetMaxValue() const
{
	return mImpl->GetMaxValue();
}

string MControl::GetText() const
{
	return mImpl->GetText();
}

void MControl::SetText(string inText)
{
	mImpl->SetText(inText);
}

void MControl::ActivateSelf()
{
	mImpl->ActivateSelf();
}

void MControl::DeactivateSelf()
{
	mImpl->DeactivateSelf();
}

void MControl::EnableSelf()
{
	mImpl->EnableSelf();
}

void MControl::DisableSelf()
{
	mImpl->DisableSelf();
}

void MControl::ShowSelf()
{
	mImpl->ShowSelf();
}

void MControl::HideSelf()
{
	mImpl->HideSelf();
}

void MControl::AddedToWindow()
{
	mImpl->AddedToWindow();
}

// --------------------------------------------------------------------

MButton::MButton(uint32 inID, MRect inBounds, const string& inLabel)
	: MControl(inID, inBounds, MControlImpl::CreateButton(this, inLabel))
{
}

// --------------------------------------------------------------------

MScrollbar::MScrollbar(uint32 inID, MRect inBounds)
	: MControl(inID, inBounds, MControlImpl::CreateScrollbar(this))
{
}

