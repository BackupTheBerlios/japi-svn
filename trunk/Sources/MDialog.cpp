/*
	Copyright (c) 2006, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*	$Id: MDialog.cpp 133 2007-05-01 08:34:48Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 13:51:21
*/

#include "MJapieG.h"

#include "MDialog.h"
#include "MView.h"
#include "MPreferences.h"
#include "MGlobals.h"

using namespace std;

MDialog* MDialog::sFirst = nil;

MDialog::MDialog()
	: mParentWindow(nil)
	, mNext(nil)
	, mCloseImmediatelyOnOK(true)
{
	mNext = sFirst;
	sFirst = this;
}

MDialog::~MDialog()
{
	if (sFirst == this)
		sFirst = mNext;
	else
	{
		MDialog* dlog = sFirst;
		while (dlog->mNext != nil)
		{
			if (dlog->mNext == this)
			{
				dlog->mNext = mNext;
				break;
			}
			dlog = dlog->mNext;
		}
	}
}

bool MDialog::OKClicked()
{
	return true;
}

bool MDialog::CancelClicked()
{
	return true;
}

void MDialog::CloseAllDialogs()
{
	MDialog* dlog = sFirst;
	
	while (dlog != nil)
	{
		MDialog* next = dlog->mNext;
		dlog->Close();
		dlog = next;
	}
}

void MDialog::Initialize(CFStringRef inNib, MWindow* inParentWindow)
{
	MWindow::Initialize(inNib);
	
	mParentWindow = inParentWindow;

	Install(kEventClassControl, kEventControlHit, this, &MDialog::DoControlHit);
}

void MDialog::Show(MWindow*	inParent)
{
	if (inParent != nil)
	{
		THROW_IF_OSERROR(
			::ShowSheetWindow(GetSysWindow(), inParent->GetSysWindow()));
	}
	else
		::ShowWindow(GetSysWindow());
}

void MDialog::GetText(uint32 inID, string& outText) const
{
	CFStringRef s;
	THROW_IF_OSERROR(
		::GetControlData(FindControl(inID), kControlEntireControl,
			kControlEditTextCFStringTag, sizeof(CFStringRef), &s, nil));

	MCFString txt(s, false);
	
	txt.GetString(outText);
}

void MDialog::SetText(uint32 inID, const string& inText)
{
	MCFString txt(inText);
	CFStringRef txtRef = txt;

	THROW_IF_OSERROR(
		::SetControlData(FindControl(inID), kControlEntireControl,
			kControlEditTextCFStringTag, sizeof(CFStringRef), &txtRef));
}

void MDialog::GetText(uint32 inID, ustring& outText) const
{
	CFStringRef s;
	THROW_IF_OSERROR(
		::GetControlData(FindControl(inID), kControlEntireControl,
			kControlEditTextCFStringTag, sizeof(CFStringRef), &s, nil));

	MCFString txt(s, false);
	
	txt.GetString(outText);
}

void MDialog::SetText(uint32 inID, const ustring& inText)
{
	MCFString txt(inText);
	CFStringRef txtRef = txt;

	THROW_IF_OSERROR(
		::SetControlData(FindControl(inID), kControlEntireControl,
			kControlEditTextCFStringTag, sizeof(CFStringRef), &txtRef));
}

int32 MDialog::GetValue(uint32 inID) const
{
	return ::GetControl32BitValue(FindControl(inID));
}

void MDialog::SetValue(uint32 inID, int32 inValue)
{
	::SetControl32BitValue(FindControl(inID), inValue);
}

void MDialog::GetValues(uint32 inID, vector<string>& outValues) const
{
	outValues.clear();
	
	string s;
	GetText(inID, s);
	outValues.push_back(s);
	
	ControlRef cntrl = FindControl(inID);
	
	uint32 n = ::HIComboBoxGetItemCount(cntrl);
	for (uint32 i = 0; i < n; ++i)
	{
		CFStringRef txt;
		::HIComboBoxCopyTextItemAtIndex(cntrl, i, &txt);
		
		MCFString(txt, false).GetString(s);
		
		if (i > 0 or s != outValues.back())
			outValues.push_back(s);
	}
}

void MDialog::SetValues(uint32 inID, const vector<string>& inValues)
{
	ControlRef cntrl = FindControl(inID);

	if (inValues.size() > 0)
		SetText(inID, inValues[0]);
	else
		SetText(inID, "");
	
	while (::HIComboBoxGetItemCount(cntrl) > 0)
		::HIComboBoxRemoveItemAtIndex(cntrl, 0);

	for (uint32 i = 0; i < inValues.size(); ++i)
		::HIComboBoxAppendTextItem(cntrl, MCFString(inValues[i]), NULL);
}

void MDialog::GetValues(uint32 inID, vector<ustring>& outValues) const
{
	outValues.clear();
	
	ustring s;
	GetText(inID, s);
	outValues.push_back(s);
	
	ControlRef cntrl = FindControl(inID);
	
	uint32 n = ::HIComboBoxGetItemCount(cntrl);
	for (uint32 i = 0; i < n; ++i)
	{
		CFStringRef txt;
		::HIComboBoxCopyTextItemAtIndex(cntrl, i, &txt);
		
		MCFString(txt, false).GetString(s);
		
		if (i > 0 or s != outValues.back())
			outValues.push_back(s);
	}
}

void MDialog::SetValues(uint32 inID, const vector<ustring>& inValues)
{
	ControlRef cntrl = FindControl(inID);

	if (inValues.size() > 0)
		SetText(inID, inValues[0]);
	else
		SetText(inID, "");
	
	while (::HIComboBoxGetItemCount(cntrl) > 0)
		::HIComboBoxRemoveItemAtIndex(cntrl, 0);

	for (uint32 i = 0; i < inValues.size(); ++i)
		::HIComboBoxAppendTextItem(cntrl, MCFString(inValues[i]), NULL);
}

OSStatus MDialog::DoControlHit(EventRef inEvent)
{
	ControlRef theControl;
	::GetEventParameter(inEvent, kEventParamDirectObject,
		typeControlRef, nil, sizeof(theControl), nil, &theControl);

	ControlID id;
	THROW_IF_OSERROR(::GetControlID(theControl, &id));

	if (id.signature == kJapieSignature)
	{
		switch (id.id)
		{
			case kMDialogOKButtonID:
				if (OKClicked())
				{
					if (mCloseImmediatelyOnOK and mParentWindow)
					{
						::DetachSheetWindow(GetSysWindow());
						::HideWindow(GetSysWindow());
						MWindow::Close();
					}
					else
						Close();
				}
				break;
	
			case kMDialogCancelButtonID:
				if (CancelClicked())
					Close();
				break;
		}
	}
	
	return noErr;
}

void MDialog::Close()
{
	if (mParentWindow != NULL)
		::HideSheetWindow(GetSysWindow());

	delete this;
}

OSStatus MDialog::DoWindowClose(EventRef inEvent)
{
	Close();

	return noErr;
}

ControlRef MDialog::FindControl(uint32 inID) const
{
	HIViewID id;
	id.signature = kJapieSignature;
	id.id = inID;

	ControlRef result = nil;
	
	THROW_IF_OSERROR(::HIViewFindByID(GetContentViewRef(), id, &result));
	
	return result;
}

void MDialog::SetFocus(uint32 inID)
{
	ControlRef currentFocus;
	if (::GetKeyboardFocus(GetSysWindow(), &currentFocus) != noErr)
		currentFocus = nil;
	
	ControlRef nextFocus = FindControl(inID);
	
	if (currentFocus != nextFocus)
		::SetKeyboardFocus(GetSysWindow(), nextFocus, kControlFocusNextPart);
}

bool MDialog::IsChecked(uint32 inID) const
{
	return ::GetControl32BitValue(FindControl(inID)) != 0;
}

void MDialog::SetChecked(uint32 inID, bool inOn)
{
	::SetControl32BitValue(FindControl(inID), inOn);
}

bool MDialog::IsVisible(uint32 inID) const
{
	return ::IsControlVisible(FindControl(inID));
}

void MDialog::SetVisible(uint32 inID, bool inVisible)
{
	if (inVisible)
		::ShowControl(FindControl(inID));
	else
		::HideControl(FindControl(inID));
}

bool MDialog::IsEnabled(uint32 inID) const
{
	return ::IsControlEnabled(FindControl(inID));
}

void MDialog::SetEnabled(uint32 inID, bool inEnabled)
{
	if (inEnabled)
		::EnableControl(FindControl(inID));
	else
		::DisableControl(FindControl(inID));
}

void MDialog::SavePosition(const char* inName)
{//
//	Rect r;
//	::GetWindowBounds(GetSysWindow(), kWindowContentRgn, &r);
//	
//	Preferences::SetRect(inName, r);
}

void MDialog::RestorePosition(const char* inName)
{//
//	Rect r;
//	::GetWindowBounds(GetSysWindow(), kWindowContentRgn, &r);
//	
//	r = Preferences::GetRect(inName, r);
//	
//	::MoveWindow(GetSysWindow(), r.left, r.top, true);
////	::SizeWindow(GetSysWindow(), r.right - r.left, r.bottom - r.top, true);
//	::ConstrainWindowToScreen(GetSysWindow(),
//		kWindowStructureRgn, kWindowConstrainStandardOptions,
//		NULL, NULL);
}

void MDialog::SetCloseImmediatelyFlag(
	bool inCloseImmediately)
{
	mCloseImmediatelyOnOK = inCloseImmediately;
}

