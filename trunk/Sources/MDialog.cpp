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

#include "MDialog.h"
#include "MResources.h"
#include "MPreferences.h"

using namespace std;

MDialog* MDialog::sFirst;

void MDialog::CreateGladeAndWidgets(
	const char*		inResource,
	GladeXML*&		outGlade,
	GtkWidget*&		outWidget)
{
	const char* xml;
	uint32 size;
	
	if (not LoadResource(inResource, xml, size))
		THROW(("Could not load dialog resource %s", inResource));
	
	outGlade = glade_xml_new_from_buffer(xml, size, nil, "japie");
	if (outGlade == nil)
		THROW(("Failed to create glade from resource"));
	
	outWidget = glade_xml_get_widget(outGlade, "dialog");
	if (outWidget == nil)
		THROW(("Failed to extract root widget from glade ('dialog')"));
}

MDialog::MDialog(
	GladeXML*		inGlade,
	GtkWidget*		inRoot)
	: MWindow(inRoot)
	, mChildFocus(this, &MDialog::ChildFocus)
	, mGlade(inGlade)
	, mParentWindow(nil)
	, mNext(nil)
	, mCloseImmediatelyOnOK(true)
{
	mNext = sFirst;
	sFirst = this;

	glade_xml_signal_connect_data(mGlade, "on_changed", 
		G_CALLBACK(&MDialog::ChangedCallBack), this);

	glade_xml_signal_connect_data(mGlade, "on_std_btn_click", 
		G_CALLBACK(&MDialog::StdBtnClickedCallBack), this);

	gtk_container_foreach(GTK_CONTAINER(inRoot),
		&MDialog::DoForEachCallBack, this);
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

	g_object_unref(mGlade);
}

void MDialog::Init()
{
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

const char* MDialog::IDToName(
	uint32			inID,
	char			inName[5]) const
{
	inName[4] = 0;
	inName[3] = inID & 0x000000ff; inID >>= 8;
	inName[2] = inID & 0x000000ff; inID >>= 8;
	inName[1] = inID & 0x000000ff; inID >>= 8;
	inName[0] = inID & 0x000000ff;
	
	return inName;
}

GtkWidget* MDialog::GetWidget(
	uint32				inID) const
{
	char name[5];
	GtkWidget* wdgt = glade_xml_get_widget(mGlade, IDToName(inID, name));
	if (wdgt == nil)
		THROW(("Widget '%s' does not exist", name));
	return wdgt;
}

void MDialog::SetFocus(
	uint32				inID)
{
	gtk_widget_grab_focus(GetWidget(inID));
}

void MDialog::GetText(
	uint32				inID,
	std::string&		outText) const
{
	GtkWidget* wdgt = GetWidget(inID);
	if (GTK_IS_COMBO_BOX(wdgt))
		outText = gtk_combo_box_get_active_text(GTK_COMBO_BOX(wdgt));
	else if (GTK_IS_ENTRY(wdgt))
		outText = gtk_entry_get_text(GTK_ENTRY(wdgt));
	else
		THROW(("item is not an entry"));
}

void MDialog::SetText(
	uint32				inID,
	const std::string&	inText)
{
	GtkWidget* wdgt = GetWidget(inID);
	if (GTK_IS_COMBO_BOX(wdgt))
assert(false);//		gtk_combo_box_set_active_text(GTK_COMBO_BOX(wdgt), inText.c_str());
	else if (GTK_IS_ENTRY(wdgt))
		gtk_entry_set_text(GTK_ENTRY(wdgt), inText.c_str());
	else if (GTK_IS_LABEL(wdgt))
		gtk_label_set_text(GTK_LABEL(wdgt), inText.c_str());
	else if (GTK_IS_BUTTON(wdgt))
		gtk_button_set_label(GTK_BUTTON(wdgt), inText.c_str());
	else if (GTK_IS_PROGRESS_BAR(wdgt))
	{
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(wdgt), inText.c_str());
		gtk_progress_bar_set_ellipsize(GTK_PROGRESS_BAR(wdgt),
			PANGO_ELLIPSIZE_MIDDLE);
	}
	else
		THROW(("item is not an entry"));
}

int32 MDialog::GetValue(
	uint32				inID) const
{
	int32 result = 0;
	GtkWidget* wdgt = GetWidget(inID);
	
	if (GTK_IS_COMBO_BOX(wdgt))
		result = gtk_combo_box_get_active(GTK_COMBO_BOX(wdgt)) + 1;
	else
		THROW(("Cannot get value"));
		
	return result;
}

void MDialog::SetValue(
	uint32				inID,
	int32				inValue)
{
	GtkWidget* wdgt = GetWidget(inID);

	if (GTK_IS_COMBO_BOX(wdgt))
		gtk_combo_box_set_active(GTK_COMBO_BOX(wdgt), inValue - 1);
	else
		THROW(("Cannot get value"));
}

// for comboboxes
void MDialog::GetValues(
	uint32				inID,
	vector<string>& 	outValues) const
{
	assert(false);
}

void MDialog::SetValues(
	uint32				inID,
	const vector<string>&
						inValues)
{
	GtkWidget* wdgt = GetWidget(inID);

	char name[5];
	if (not GTK_IS_COMBO_BOX(wdgt))
		THROW(("Item %s is not a combo box", IDToName(inID, name)));

	GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(wdgt));
	int32 count = gtk_tree_model_iter_n_children(model, nil);

	while (count-- > 0)
		gtk_combo_box_remove_text(GTK_COMBO_BOX(wdgt), count);

	for (vector<string>::const_iterator s = inValues.begin(); s != inValues.end(); ++s)
		gtk_combo_box_append_text(GTK_COMBO_BOX(wdgt), s->c_str());

	gtk_combo_box_set_active(GTK_COMBO_BOX(wdgt), 0);
}

bool MDialog::IsChecked(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	if (not GTK_IS_TOGGLE_BUTTON(wdgt))
		THROW(("Widget '%d' is not of the correct type", inID));
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wdgt));
}

void MDialog::SetChecked(
	uint32				inID,
	bool				inOn)
{
	GtkWidget* wdgt = GetWidget(inID);
	if (not GTK_IS_TOGGLE_BUTTON(wdgt))
		THROW(("Widget '%d' is not of the correct type", inID));
	return gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wdgt), inOn);
}	

bool MDialog::IsVisible(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	return GTK_WIDGET_VISIBLE(wdgt);
}

void MDialog::SetVisible(
	uint32				inID,
	bool				inVisible)
{
	GtkWidget* wdgt = GetWidget(inID);
	if (inVisible)
		gtk_widget_show(wdgt);
	else
		gtk_widget_hide(wdgt);
}

bool MDialog::IsEnabled(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	return GTK_WIDGET_IS_SENSITIVE(wdgt);
}

void MDialog::SetEnabled(
	uint32				inID,
	bool				inEnabled)
{
	GtkWidget* wdgt = GetWidget(inID);
	gtk_widget_set_sensitive(wdgt, inEnabled);
}

bool MDialog::IsExpanded(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	assert(GTK_IS_EXPANDER(wdgt));
	return gtk_expander_get_expanded(GTK_EXPANDER(wdgt));
}

void MDialog::SetExpanded(
	uint32				inID,
	bool				inExpanded)
{
	GtkWidget* wdgt = GetWidget(inID);
	assert(GTK_IS_EXPANDER(wdgt));
	gtk_expander_set_expanded(GTK_EXPANDER(wdgt), inExpanded);
}

void MDialog::SetProgressFraction(
	uint32				inID,
	float				inFraction)
{
	GtkWidget* wdgt = GetWidget(inID);
	assert(GTK_IS_PROGRESS_BAR(wdgt));
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(wdgt), inFraction);
}

void MDialog::ValueChanged(
	uint32				inID)
{
//	char name[5];
//	cout << "Value Changed for " << IDToName(inID, name) << endl;
}

bool MDialog::OKClicked()
{
	return true;
}

bool MDialog::CancelClicked()
{
	return true;
}

void MDialog::ChangedCallBack(
	GtkWidget*			inWidget,
	gpointer			inUserData)
{
	MDialog* self = reinterpret_cast<MDialog*>(inUserData);
	
	const char* name = glade_get_widget_name(inWidget);
	if (name != nil)
	{
		uint32 id = 0;
		for (uint32 i = 0; i < 4 and name[i]; ++i)
			id = (id << 8) | name[i];
		
		try
		{
			self->ValueChanged(id);
		}
		catch (exception& e)
		{
			MError::DisplayError(e);
		}
		catch (...) {}
	}
}

void MDialog::StdBtnClickedCallBack(
	GtkWidget*			inWidget,
	gpointer			inUserData)
{
	MDialog* self = reinterpret_cast<MDialog*>(inUserData);
	
	const char* name = glade_get_widget_name(inWidget);
	if (name != nil)
	{
		uint32 id = 0;
		for (uint32 i = 0; i < 4 and name[i]; ++i)
			id = (id << 8) | name[i];
		
		try
		{
			switch (id)
			{
				case 'okok':
					if (self->OKClicked())
						self->Close();
					break;
				
				case 'cncl':
					if (self->CancelClicked())
						self->Close();
					break;
				
				default:
					self->ValueChanged(id);
					break;
			}
		}
		catch (exception& e)
		{
			MError::DisplayError(e);
		}
		catch (...) {}
	}
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

void MDialog::DoForEachCallBack(
	GtkWidget*			inWidget,
	gpointer			inUserData)
{
	MDialog* self = reinterpret_cast<MDialog*>(inUserData);
	self->DoForEach(inWidget);
}

void MDialog::DoForEach(
	GtkWidget*			inWidget)
{
	gboolean canFocus = false;

	g_object_get(G_OBJECT(inWidget), "can-focus", &canFocus, NULL);

	if (canFocus)
		mChildFocus.Connect(inWidget, "focus-in-event");
	
	if (GTK_IS_CONTAINER(inWidget))
		gtk_container_foreach(GTK_CONTAINER(inWidget), &MDialog::DoForEachCallBack, this);
}

bool MDialog::ChildFocus(
	GdkEventFocus*		inEvent)
{
	try
	{
		TakeFocus();
		FocusChanged(0);
	}
	catch (...) {}
	return false;
}

void MDialog::FocusChanged(
	uint32				inFocussedID)
{
}
