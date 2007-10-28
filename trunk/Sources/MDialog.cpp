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

#include <boost/bind.hpp>

#include "MDialog.h"
#include "MView.h"
#include "MPreferences.h"
#include "MGlobals.h"

using namespace std;

MDialog* MDialog::sFirst = nil;

struct MDialogItem
{
	uint32			mID;
	GtkWidget*		mWidget;
	uint32			mParentID;
	
	// for tables
	uint32			mColumns;
	uint32			mIndex;
	
	uint32			GetID() const		{ return mID; }
};

struct MDialogImp
{
	GtkWidget*			mVBox;
	vector<MDialogItem>	mItems;
	
	MDialogItem&		GetItem(
							uint32		inID);

	void				Add(
							MDialogItem	inItem);
};

MDialogItem& MDialogImp::GetItem(
	uint32			inID)
{
	vector<MDialogItem>::iterator i = find_if(mItems.begin(), mItems.end(),
		boost::bind(&MDialogItem::GetID, _1) == inID);
	
	assert(i != mItems.end());
	if (i == mItems.end())
		THROW(("Item not found"));
	
	return *i;
}

void MDialogImp::Add(
	MDialogItem		inItem)
{
	if (inItem.mParentID == 0)
		gtk_box_pack_start(GTK_BOX(mVBox), inItem.mWidget, false, false, 12);
	else
	{
		MDialogItem& parent = GetItem(inItem.mParentID);
		
		if (GTK_IS_TABLE(parent.mWidget))
		{
			uint32 row = parent.mIndex / parent.mColumns;
			uint32 col = parent.mIndex % parent.mColumns;
			
			gtk_table_attach(GTK_TABLE(parent.mWidget), inItem.mWidget,
				col, col + 1, row, row + 1, GTK_FILL, GTK_FILL, 3, 3);
			
			++parent.mIndex;
		}
		else if (GTK_IS_BOX(parent.mWidget))
			gtk_box_pack_start(GTK_BOX(parent.mWidget), inItem.mWidget, false, false, 12);
		else if (GTK_IS_CONTAINER(parent.mWidget))
			gtk_container_add(GTK_CONTAINER(parent.mWidget), inItem.mWidget);
		else
			THROW(("Cannot add widget to this parent"));
	}
	
	mItems.push_back(inItem);
}

MDialog::MDialog()
	: MWindow(gtk_dialog_new())
	, mOKClicked(this, &MDialog::OnOKClickedEvent)
	, mCancelClicked(this, &MDialog::OnCancelClickedEvent)
	, mParentWindow(nil)
	, mNext(nil)
	, mImpl(new MDialogImp)
	, mCloseImmediatelyOnOK(true)
{
	mNext = sFirst;
	sFirst = this;

	gtk_window_set_resizable(GTK_WINDOW(GetGtkWidget()), false);
	
	mImpl->mVBox = GTK_DIALOG(GetGtkWidget())->vbox;
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
	
	delete mImpl;
}

bool MDialog::OKClicked()
{
	return true;
}

bool MDialog::OnOKClickedEvent()
{
	if (OKClicked())
		Close();

	return true;
}

bool MDialog::CancelClicked()
{
	return true;
}

bool MDialog::OnCancelClickedEvent()
{
	if (CancelClicked())
		Close();
	
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

//void MDialog::Initialize(CFStringRef inNib, MWindow* inParentWindow)
//{
//	MWindow::Initialize(inNib);
//	
//	mParentWindow = inParentWindow;
//
//	Install(kEventClassControl, kEventControlHit, this, &MDialog::DoControlHit);
//}

void MDialog::AddOKButton(
	const char*			inLabel)
{
	GtkWidget* button = gtk_button_new_with_label(inLabel);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(GetGtkWidget())->action_area), button, false, false, 0);
	mOKClicked.Connect(button, "clicked");
}

void MDialog::AddCancelButton(
	const char*			inLabel)
{
	GtkWidget* button = gtk_button_new_with_label(inLabel);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(GetGtkWidget())->action_area), button, false, false, 0);
	mCancelClicked.Connect(button, "clicked");
}

void MDialog::AddVBox(
	uint32				inID,
	bool				inHomogenous,
	int32				inSpacing,
	uint32				inParentID)
{
	MDialogItem item;
	
	item.mWidget = gtk_vbox_new(inHomogenous, inSpacing);
	item.mID = inID;
	item.mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddVButtonBox(
	uint32				inID,
	uint32				inParentID)
{
	MDialogItem item;
	
	item.mWidget = gtk_vbutton_box_new();
	item.mID = inID;
	item.mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddHBox(
	uint32				inID,
	bool				inHomogenous,
	int32				inSpacing,
	uint32				inParentID)
{
	MDialogItem item;
	
	item.mWidget = gtk_hbox_new(inHomogenous, inSpacing);
	item.mID = inID;
	item.mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddHButtonBox(
	uint32				inID,
	uint32				inParentID)
{
	MDialogItem item;
	
	item.mWidget = gtk_hbutton_box_new();
	item.mID = inID;
	item.mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddTable(
	uint32				inID,
	uint32				inColumnCount,
	uint32				inRowCount,
	uint32				inParentID)
{
	MDialogItem item;
	
	item.mWidget = gtk_table_new(inRowCount, inColumnCount, false);
	item.mID = inID;
	item.mParentID = inParentID;
	item.mColumns = inColumnCount;
	item.mIndex = 0;
	
	mImpl->Add(item);
}

void MDialog::AddButton(
	uint32				inID,
	const std::string&	inLabel,
	uint32				inParentID)
{
	MDialogItem item;
	
	item.mWidget = gtk_button_new_with_label(inLabel.c_str());
	item.mID = inID;
	item.mParentID = inParentID;
	
	mImpl->Add(item);
}
	
void MDialog::AddStaticText(
	uint32				inID,
	const std::string&	inLabel,
	uint32				inParentID)
{
	MDialogItem item;
	
	item.mWidget = gtk_label_new(inLabel.c_str());
	gtk_label_set_justify(GTK_LABEL(item.mWidget), GTK_JUSTIFY_LEFT);
	item.mID = inID;
	item.mParentID = inParentID;
	
	mImpl->Add(item);
}
	
void MDialog::AddEditField(
	uint32				inID,
	const std::string&	inText,
	uint32				inParentID)
{
	MDialogItem item;
	
	item.mWidget = gtk_entry_new();
	
	if (inText.length() > 0)
		gtk_entry_set_text(GTK_ENTRY(item.mWidget), inText.c_str());
	
	item.mID = inID;
	item.mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddComboBoxEntry(
	uint32				inID,
	uint32				inParentID)
{
	MDialogItem item;
	
	item.mWidget = gtk_combo_box_entry_new();
	item.mID = inID;
	item.mParentID = inParentID;
	
	mImpl->Add(item);
}
	
void MDialog::AddHSeparator(
	uint32				inID,
	uint32				inParentID)
{
	MDialogItem item;
	
	item.mWidget = gtk_hseparator_new();
	item.mID = inID;
	item.mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddCheckBox(
	uint32				inID,
	const string&		inLabel,
	uint32				inParentID)
{
	MDialogItem item;
	
	item.mWidget = gtk_check_button_new_with_label(inLabel.c_str());
	item.mID = inID;
	item.mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::Show(
	MWindow*		inParent)
{
	if (inParent != nil)
	{
		gtk_window_set_transient_for(
			GTK_WINDOW(GetGtkWidget()),
			GTK_WINDOW(inParent->GetGtkWidget()));
	}
	
	MWindow::Show();
}

void MDialog::GetText(
	uint32			inID,
	string&			outText) const
{
	MDialogItem item = mImpl->GetItem(inID);
	if (GTK_IS_COMBO_BOX(item.mWidget))
		outText = gtk_combo_box_get_active_text(GTK_COMBO_BOX(item.mWidget));
	else if (GTK_IS_ENTRY(item.mWidget))
		outText = gtk_entry_get_text(GTK_ENTRY(item.mWidget));
	else
		THROW(("item is not an entry"));
}

void MDialog::SetText(
	uint32			inID,
	const string&	inText)
{
	MDialogItem item = mImpl->GetItem(inID);
	if (GTK_IS_COMBO_BOX(item.mWidget))
;//		gtk_combo_box_set_active_text(GTK_COMBO_BOX(item.mWidget), inText.c_str());
	else if (GTK_IS_ENTRY(item.mWidget))
		gtk_entry_set_text(GTK_ENTRY(item.mWidget), inText.c_str());
	else
		THROW(("item is not an entry"));
}

int32 MDialog::GetValue(uint32 inID) const
{
//	return ::GetControl32BitValue(FindControl(inID));
}

void MDialog::SetValue(uint32 inID, int32 inValue)
{
//	::SetControl32BitValue(FindControl(inID), inValue);
}

void MDialog::GetValues(
	uint32			inID,
	vector<string>&	outValues) const
{
//	outValues.clear();
//	
//	string s;
//	GetText(inID, s);
//	outValues.push_back(s);
//	
//	ControlRef cntrl = FindControl(inID);
//	
//	uint32 n = ::HIComboBoxGetItemCount(cntrl);
//	for (uint32 i = 0; i < n; ++i)
//	{
//		CFStringRef txt;
//		::HIComboBoxCopyTextItemAtIndex(cntrl, i, &txt);
//		
//		MCFString(txt, false).GetString(s);
//		
//		if (i > 0 or s != outValues.back())
//			outValues.push_back(s);
//	}
}

void MDialog::SetValues(
	uint32					inID,
	const vector<string>&	inValues)
{
//	ControlRef cntrl = FindControl(inID);
//
//	if (inValues.size() > 0)
//		SetText(inID, inValues[0]);
//	else
//		SetText(inID, "");
//	
//	while (::HIComboBoxGetItemCount(cntrl) > 0)
//		::HIComboBoxRemoveItemAtIndex(cntrl, 0);
//
//	for (uint32 i = 0; i < inValues.size(); ++i)
//		::HIComboBoxAppendTextItem(cntrl, MCFString(inValues[i]), NULL);
}

//OSStatus MDialog::DoControlHit(EventRef inEvent)
//{
//	ControlRef theControl;
//	::GetEventParameter(inEvent, kEventParamDirectObject,
//		typeControlRef, nil, sizeof(theControl), nil, &theControl);
//
//	ControlID id;
//	THROW_IF_OSERROR(::GetControlID(theControl, &id));
//
//	if (id.signature == kJapieSignature)
//	{
//		switch (id.id)
//		{
//			case kMDialogOKButtonID:
//				if (OKClicked())
//				{
//					if (mCloseImmediatelyOnOK and mParentWindow)
//					{
//						::DetachSheetWindow(GetSysWindow());
//						::HideWindow(GetSysWindow());
//						MWindow::Close();
//					}
//					else
//						Close();
//				}
//				break;
//	
//			case kMDialogCancelButtonID:
//				if (CancelClicked())
//					Close();
//				break;
//		}
//	}
//	
//	return noErr;
//}

//void MDialog::Close()
//{
//	if (mParentWindow != NULL)
//		::HideSheetWindow(GetSysWindow());
//
//	delete this;
//}

void MDialog::SetFocus(
	uint32			inID)
{
	MDialogItem item = mImpl->GetItem(inID);
	gtk_widget_grab_focus(item.mWidget);
}

bool MDialog::IsChecked(uint32 inID) const
{
	MDialogItem item = mImpl->GetItem(inID);
	assert(GTK_IS_CHECK_BUTTON(item.mWidget));
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item.mWidget));
}

void MDialog::SetChecked(uint32 inID, bool inOn)
{
	MDialogItem item = mImpl->GetItem(inID);
	assert(GTK_IS_CHECK_BUTTON(item.mWidget));
	return gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item.mWidget), inOn);
}

bool MDialog::IsVisible(uint32 inID) const
{
//	return ::IsControlVisible(FindControl(inID));
}

void MDialog::SetVisible(uint32 inID, bool inVisible)
{
//	if (inVisible)
//		::ShowControl(FindControl(inID));
//	else
//		::HideControl(FindControl(inID));
}

bool MDialog::IsEnabled(uint32 inID) const
{
//	return ::IsControlEnabled(FindControl(inID));
}

void MDialog::SetEnabled(uint32 inID, bool inEnabled)
{
//	if (inEnabled)
//		::EnableControl(FindControl(inID));
//	else
//		::DisableControl(FindControl(inID));
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

