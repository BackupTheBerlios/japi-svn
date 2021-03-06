//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MDialog.cpp 133 2007-05-01 08:34:48Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 13:51:21
*/

#include "MLib.h"

#include <sstream>

#include "MDialog.h"
#include "MWindowImpl.h"
#include "MResources.h"
#include "MPreferences.h"
#include "MError.h"
#include "MControls.h"

using namespace std;

MDialog* MDialog::sFirst;

MDialog::MDialog(
	const string&		inDialogResource)
	: MWindow(MWindowImpl::CreateDialog(inDialogResource, this))
	, eButtonClicked(this, &MDialog::ButtonClicked)
	, eCheckboxClicked(this, &MDialog::CheckboxChanged)
	, eTextChanged(this, &MDialog::TextChanged)
	, mParentWindow(nil)
	, mNext(nil)
{
	mNext = sFirst;
	sFirst = this;

	GetImpl()->Finish();
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

void MDialog::Show(
	MWindow*		inParent)
{
	//if (inParent != nil)
	//{
	//	gtk_window_set_transient_for(
	//		GTK_WINDOW(GetGtkWidget()),
	//		GTK_WINDOW(inParent->GetGtkWidget()));
	//}

	// if parent exists, we position our dialog on top of it
	if (inParent != nil)
	{
		MRect r;
		GetWindowPosition(r);
	
		MRect b;
		inParent->GetWindowPosition(b);
		
		r.x = b.x + (b.width - r.width) / 2;
		r.y = b.y + (b.height - r.height) / 3;

		SetWindowPosition(r);
	}
	
	MWindow::Show();
	MWindow::Select();
}

bool MDialog::OKClicked()
{
	return true;
}

bool MDialog::CancelClicked()
{
	return true;
}

void MDialog::ButtonClicked(
	const string&		inID)
{
	if (inID == "ok")
	{
		if (OKClicked())
			Close();
	}
	else if (inID == "cancel")
	{
		if (CancelClicked())
			Close();
	}
}

void MDialog::CheckboxChanged(
	const string&		inID,
	bool				inChecked)
{
}

void MDialog::TextChanged(
	const string&		inID,
	const string&		inText)
{
}

void MDialog::SetFocus(const std::string& inID)
{
	//MControl* control = dynamic_cast<MControl*>(FindViewByID(inID));
	//if (control != nil)
	//	control->Focus();

	MView* view = FindSubViewByID(inID);
	if (view != nil)
	{
		if (dynamic_cast<MEdittext*>(view) != nil)
			static_cast<MEdittext*>(view)->Focus();
		else if (dynamic_cast<MCombobox*>(view) != nil)
			static_cast<MCombobox*>(view)->Focus();
	}
}

void MDialog::SavePosition(const char* inName)
{
	MRect r;
	GetWindowPosition(r);

	stringstream s;
	s << r.x << ' ' << r.y << ' ' << r.width << ' ' << r.height;
	
	Preferences::SetString(inName, s.str());
}

void MDialog::RestorePosition(const char* inName)
{
	string s = Preferences::GetString(inName, "");
	if (s.length() > 0)
	{
		MRect r;
		
		stringstream ss(s);
		ss >> r.x >> r.y >> r.width >> r.height;
		
		if (GetFlags() & kMFixedSize)
		{
			MRect bounds;
			GetWindowPosition(bounds);
			r.width = bounds.width;
			r.height = bounds.height;
		}

//		SetWindowPosition(r, false);
	}
}

string MDialog::GetText(const string& inID) const
{
	string result;
	
	MView* view = FindSubViewByID(inID);
	THROW_IF_NIL(view);
	if (dynamic_cast<MCombobox*>(view) != nil)
		result = static_cast<MCombobox*>(view)->GetText();
	else if (dynamic_cast<MEdittext*>(view) != nil)
		result = static_cast<MEdittext*>(view)->GetText();
	
	return result;
}

void MDialog::SetText(const string& inID, const std::string& inText)
{
	MView* view = FindSubViewByID(inID);
	THROW_IF_NIL(view);
	if (dynamic_cast<MCombobox*>(view) != nil)
		static_cast<MCombobox*>(view)->SetText(inText);
	else if (dynamic_cast<MEdittext*>(view) != nil)
		static_cast<MEdittext*>(view)->SetText(inText);
	else if (dynamic_cast<MCaption*>(view) != nil)
		static_cast<MCaption*>(view)->SetText(inText);
}

void MDialog::SetPasswordChar(const string& inID, const uint32 inUnicode)
{
	MView* view = FindSubViewByID(inID);
	THROW_IF_NIL(dynamic_cast<MEdittext*>(view));
	static_cast<MEdittext*>(view)->SetPasswordChar(inUnicode);
}

int32 MDialog::GetValue(const std::string& inID) const
{
	int32 result = -1;

	MView* view = FindSubViewByID(inID);
	THROW_IF_NIL(view);
	if (dynamic_cast<MPopup*>(view) != nil)
		result = static_cast<MPopup*>(view)->GetValue();
	
	return result;
}

void MDialog::SetValue(const std::string& inID, int32 inValue)
{
	MView* view = FindSubViewByID(inID);
	THROW_IF_NIL(view);
	if (dynamic_cast<MPopup*>(view) != nil)
		static_cast<MPopup*>(view)->SetValue(inValue);
}

bool MDialog::IsChecked(const string& inID) const
{
	MView* view = FindSubViewByID(inID);
	THROW_IF_NIL(dynamic_cast<MCheckbox*>(view));
	return static_cast<MCheckbox*>(view)->IsChecked();
}

void MDialog::SetChecked(const string& inID, bool inChecked)
{
	MView* view = FindSubViewByID(inID);
	THROW_IF_NIL(dynamic_cast<MCheckbox*>(view));
	static_cast<MCheckbox*>(view)->SetChecked(inChecked);
}

void MDialog::SetChoices(const string& inID, vector<string>& inChoices)
{
	MCombobox* combo = dynamic_cast<MCombobox*>(FindSubViewByID(inID));
	THROW_IF_NIL(combo);
	combo->SetChoices(inChoices);
}

void MDialog::SetEnabled(const string& inID, bool inEnabled)
{
	MView* view = FindSubViewByID(inID);
	THROW_IF_NIL(view);
	if (inEnabled)
		view->Enable();
	else
		view->Disable();
}

void MDialog::SetVisible(const string& inID, bool inVisible)
{
	MView* view = FindSubViewByID(inID);
	THROW_IF_NIL(view);
	if (inVisible)
		view->Show();
	else
		view->Hide();
}

