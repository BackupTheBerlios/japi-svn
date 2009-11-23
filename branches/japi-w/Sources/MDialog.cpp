//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MDialog.cpp 133 2007-05-01 08:34:48Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 13:51:21
*/

#include "MJapi.h"

#include <sstream>

#include "MDialog.h"
#include "MResources.h"
#include "MPreferences.h"
#include "MError.h"

using namespace std;

MDialog* MDialog::sFirst;

MDialog::MDialog(
	const char*		inDialogResource)
	: MWindow(inDialogResource, "dialog")
	, mParentWindow(nil)
	, mNext(nil)
	, mCloseImmediatelyOnOK(true)
{
	mNext = sFirst;
	sFirst = this;

	glade_xml_signal_connect_data(GetGladeXML(), "on_std_btn_click", 
		G_CALLBACK(&MDialog::StdBtnClickedCallBack), this);
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
	if (inParent != nil)
	{
		gtk_window_set_transient_for(
			GTK_WINDOW(GetGtkWidget()),
			GTK_WINDOW(inParent->GetGtkWidget()));
	}
	
	MWindow::Show();
}

bool MDialog::OKClicked()
{
	return true;
}

bool MDialog::CancelClicked()
{
	return true;
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
			DisplayError(e);
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
