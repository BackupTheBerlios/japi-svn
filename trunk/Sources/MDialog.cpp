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

#include <sstream>
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "MDialog.h"
#include "MView.h"
#include "MPreferences.h"
#include "MGlobals.h"

using namespace std;

MDialog* MDialog::sFirst = nil;

const uint32 kActionAreaID = 0x3f3f3f3f;

struct MDialogItem
{
	virtual			~MDialogItem() {}
	
	uint32			mID;
	GtkWidget*		mWidget;
	uint32			mParentID;
	
	// for tables
	uint32			mColumns;
	uint32			mIndex;
	
	// for combo boxes
	int32			mCount;
					
	uint32			GetID() const		{ return mID; }
};

struct MButtonItem : public MDialogItem
{
						MButtonItem(
							MDialog*	inDialog,
							GtkWidget*	inWidget,
							const char*	inSignal)
							: mDialog(inDialog)
							, mClicked(this, &MButtonItem::OnClicked)
						{
							mWidget = inWidget;
							mClicked.Connect(mWidget, inSignal);
						}

	void				OnClicked()
						{
							mDialog->ButtonClicked(mID);
						}
	
	MDialog*			mDialog;
	MSlot<void()>		mClicked;
};

typedef boost::ptr_vector<MDialogItem>	MDialogItemList;

struct MDialogImp
{
	GtkWidget*			mVBox;
	MDialogItemList		mItems;

	MDialogItem&		GetItem(
							uint32			inID);

	void				Add(
							MDialogItem*	inItem);
};

MDialogItem& MDialogImp::GetItem(
	uint32			inID)
{
	MDialogItemList::iterator i = find_if(mItems.begin(), mItems.end(),
		boost::bind(&MDialogItem::GetID, _1) == inID);
	
	assert(i != mItems.end());
	if (i == mItems.end())
		THROW(("Item not found"));
	
	return *i;
}

void MDialogImp::Add(
	MDialogItem*	inItem)
{
	if (inItem->mParentID == 0)
		gtk_box_pack_start(GTK_BOX(mVBox), inItem->mWidget, true, true, 3);
	else if (inItem->mParentID != kActionAreaID)
	{
		MDialogItem& parent = GetItem(inItem->mParentID);
		
		if (GTK_IS_TABLE(parent.mWidget))
		{
			uint32 row = parent.mIndex / parent.mColumns;
			uint32 col = parent.mIndex % parent.mColumns;
			
			GtkAttachOptions opt = GtkAttachOptions(GTK_FILL | GTK_EXPAND);
			
//			if (GTK_IS_COMBO_BOX(inItem->mWidget) or GTK_IS_ENTRY(inItem->mWidget))
//				opt = GTK_EXPAND;
			
			gtk_table_attach(GTK_TABLE(parent.mWidget), inItem->mWidget,
				col, col + 1, row, row + 1, opt, GtkAttachOptions(0), 3, 3);
			
			++parent.mIndex;
		}
		else if (GTK_IS_BOX(parent.mWidget))
			gtk_box_pack_start(GTK_BOX(parent.mWidget), inItem->mWidget, true, true, 12);
		else if (GTK_IS_CONTAINER(parent.mWidget))
			gtk_container_add(GTK_CONTAINER(parent.mWidget), inItem->mWidget);
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
	const char*			inLabel,
	uint32				inButonID)
{
	GtkWidget* button = gtk_dialog_add_button(
							GTK_DIALOG(GetGtkWidget()), inLabel, inButonID);
	mOKClicked.Connect(button, "clicked");
	gtk_dialog_set_default_response(GTK_DIALOG(GetGtkWidget()), inButonID);
}

void MDialog::AddCancelButton(
	const char*			inLabel)
{
	GtkWidget* button = gtk_button_new_with_label(inLabel);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(GetGtkWidget())->action_area), button, true, true, 0);
	mCancelClicked.Connect(button, "clicked");
}

void MDialog::AddVBox(
	uint32				inID,
	bool				inHomogenous,
	int32				inSpacing,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_vbox_new(inHomogenous, inSpacing);
	item->mID = inID;
	item->mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddVButtonBox(
	uint32				inID,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_vbutton_box_new();
	item->mID = inID;
	item->mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddHBox(
	uint32				inID,
	bool				inHomogenous,
	int32				inSpacing,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_hbox_new(inHomogenous, inSpacing);
	item->mID = inID;
	item->mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddHButtonBox(
	uint32				inID,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_hbutton_box_new();
	item->mID = inID;
	item->mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddTable(
	uint32				inID,
	uint32				inColumnCount,
	uint32				inRowCount,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_table_new(inRowCount, inColumnCount, false);
	item->mID = inID;
	item->mParentID = inParentID;
	item->mColumns = inColumnCount;
	item->mIndex = 0;
	
	mImpl->Add(item);
}

void MDialog::AddAlignment(
	uint32				inID,
	float				inXAlign,
	float				inYAlign,
	float				inXScale,
	float				inYScale,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_alignment_new(inXAlign, inYAlign, inXScale, inYScale);
	item->mID = inID;
	item->mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddButton(
	uint32				inID,
	const std::string&	inLabel,
	uint32				inParentID)
{
	MDialogItem* item = new MButtonItem(this,
		gtk_button_new_with_label(inLabel.c_str()), "clicked");
	
	item->mID = inID;

	if (inParentID == 0)
	{
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(GetGtkWidget())->action_area), item->mWidget, true, true, 0);
		item->mParentID = kActionAreaID;
	}
	else
		item->mParentID = inParentID;

	mImpl->Add(item);
}
	
void MDialog::AddStaticText(
	uint32				inID,
	const std::string&	inLabel,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_label_new(inLabel.c_str());
	item->mID = inID;
	item->mParentID = inParentID;
	
	mImpl->Add(item);

	gtk_label_set_justify(GTK_LABEL(item->mWidget), GTK_JUSTIFY_LEFT);
}
	
void MDialog::AddEditField(
	uint32				inID,
	const std::string&	inText,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_entry_new();
	
	if (inText.length() > 0)
		gtk_entry_set_text(GTK_ENTRY(item->mWidget), inText.c_str());
	
	item->mID = inID;
	item->mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::SetPasswordField(
	uint32				inID)
{
	MDialogItem& item = mImpl->GetItem(inID);
	gtk_entry_set_visibility(GTK_ENTRY(item.mWidget), false);
	gtk_entry_set_invisible_char(GTK_ENTRY(item.mWidget), 0x2022);
}

void MDialog::AddComboBox(
	uint32				inID,
	const vector<string>&
						inOptions,
	uint32				inParentID)
{
	MDialogItem* item = new MButtonItem(this,
		gtk_combo_box_new_text(), "changed");
	
	item->mID = inID;
	item->mParentID = inParentID;
	item->mCount = 0;
	
	mImpl->Add(item);
	
	for (vector<string>::const_iterator s = inOptions.begin(); s != inOptions.end(); ++s)
		gtk_combo_box_append_text(GTK_COMBO_BOX(item->mWidget), s->c_str());
	
	item->mCount = inOptions.size();
	gtk_combo_box_set_active(GTK_COMBO_BOX(item->mWidget), 0);
}
	
void MDialog::AddComboBoxEntry(
	uint32				inID,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_combo_box_entry_new_text();
	item->mID = inID;
	item->mParentID = inParentID;
	item->mCount = 0;
	
	mImpl->Add(item);
}
	
void MDialog::AddHSeparator(
	uint32				inID,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_hseparator_new();
	item->mID = inID;
	item->mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddCheckBox(
	uint32				inID,
	const string&		inLabel,
	uint32				inParentID)
{
	MDialogItem* item = new MButtonItem(this,
		gtk_check_button_new_with_label(inLabel.c_str()), "clicked");
	
	item->mID = inID;
	item->mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddExpander(
	uint32				inID,
	const char*			inLabel,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_expander_new(inLabel);
	item->mID = inID;
	item->mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::AddProgressBar(
	uint32				inID,
	uint32				inParentID)
{
	MDialogItem* item = new MDialogItem;
	
	item->mWidget = gtk_progress_bar_new();
	item->mID = inID;
	item->mParentID = inParentID;
	
	mImpl->Add(item);
}

void MDialog::SetProgressFraction(
	uint32				inID,
	float				inFraction)
{
	MDialogItem item = mImpl->GetItem(inID);
	if (GTK_IS_PROGRESS_BAR(item.mWidget))
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(item.mWidget), inFraction);
	else
		THROW(("item is not a progress bar"));
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
	else if (GTK_IS_LABEL(item.mWidget))
		gtk_label_set_text(GTK_LABEL(item.mWidget), inText.c_str());
	else if (GTK_IS_PROGRESS_BAR(item.mWidget))
	{
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(item.mWidget), inText.c_str());
		gtk_progress_bar_set_ellipsize(GTK_PROGRESS_BAR(item.mWidget),
			PANGO_ELLIPSIZE_MIDDLE);
	}
	else
		THROW(("item is not an entry"));
}

int32 MDialog::GetValue(uint32 inID) const
{
	int32 result = 0;
	
	MDialogItem item = mImpl->GetItem(inID);
	if (GTK_IS_COMBO_BOX(item.mWidget))
		result = gtk_combo_box_get_active(GTK_COMBO_BOX(item.mWidget)) + 1;
	else
		THROW(("Cannot get value"));
	return result;
}

void MDialog::SetValue(uint32 inID, int32 inValue)
{
	MDialogItem item = mImpl->GetItem(inID);
	if (GTK_IS_COMBO_BOX(item.mWidget))
		gtk_combo_box_set_active(GTK_COMBO_BOX(item.mWidget), inValue - 1);
	else
		THROW(("Cannot get value"));
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
	MDialogItem& item = mImpl->GetItem(inID);
	assert(GTK_IS_COMBO_BOX(item.mWidget));
	
	while (item.mCount-- > 0)
		gtk_combo_box_remove_text(GTK_COMBO_BOX(item.mWidget), 0);

	for (vector<string>::const_iterator s = inValues.begin(); s != inValues.end(); ++s)
		gtk_combo_box_append_text(GTK_COMBO_BOX(item.mWidget), s->c_str());
	
	item.mCount = inValues.size();
	gtk_combo_box_set_active(GTK_COMBO_BOX(item.mWidget), 0);
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
	assert(GTK_IS_TOGGLE_BUTTON(item.mWidget));
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item.mWidget));
}

void MDialog::SetChecked(uint32 inID, bool inOn)
{
	MDialogItem item = mImpl->GetItem(inID);
	assert(GTK_IS_CHECK_BUTTON(item.mWidget));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item.mWidget), inOn);
}

bool MDialog::IsVisible(uint32 inID) const
{
	MDialogItem item = mImpl->GetItem(inID);
	return GTK_WIDGET_VISIBLE(item.mWidget);
}

void MDialog::SetVisible(uint32 inID, bool inVisible)
{
	MDialogItem item = mImpl->GetItem(inID);
	if (inVisible)
		gtk_widget_show(item.mWidget);
	else
		gtk_widget_hide(item.mWidget);
}

bool MDialog::IsEnabled(uint32 inID) const
{
	MDialogItem item = mImpl->GetItem(inID);
	return GTK_WIDGET_IS_SENSITIVE(item.mWidget);
}

void MDialog::SetEnabled(uint32 inID, bool inEnabled)
{
	MDialogItem item = mImpl->GetItem(inID);
	gtk_widget_set_sensitive(item.mWidget, inEnabled);
}

bool MDialog::IsExpanded(
	uint32				inID) const
{
	MDialogItem& item = mImpl->GetItem(inID);
	assert(GTK_IS_EXPANDER(item.mWidget));
	return gtk_expander_get_expanded(GTK_EXPANDER(item.mWidget));
}

void MDialog::SetExpanded(
	uint32				inID,
	bool				inExpanded)
{
	MDialogItem& item = mImpl->GetItem(inID);
	assert(GTK_IS_EXPANDER(item.mWidget));
	gtk_expander_set_expanded(GTK_EXPANDER(item.mWidget), inExpanded);
}

void MDialog::SavePosition(const char* inName)
{
	int x, y;
	gtk_window_get_position(GTK_WINDOW(GetGtkWidget()), &x, &y);
	
	stringstream s;
	s << x << ' ' << y;
	
	Preferences::SetString(inName, s.str());
}

void MDialog::RestorePosition(const char* inName)
{
	string s = Preferences::GetString(inName, "");
	if (s.length() > 0)
	{
		int x, y;
		
		stringstream ss(s);
		ss >> x >> y;
		
		gtk_window_move(GTK_WINDOW(GetGtkWidget()), x, y);
	}
}

void MDialog::SetCloseImmediatelyFlag(
	bool inCloseImmediately)
{
	mCloseImmediatelyOnOK = inCloseImmediately;
}

void MDialog::ButtonClicked(
	uint32	inButonID)
{
	cout << "Button clicked: " << hex << inButonID << dec << endl;
}
