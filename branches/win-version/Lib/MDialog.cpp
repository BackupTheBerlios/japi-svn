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

using namespace std;

MDialog* MDialog::sFirst;

MDialog::MDialog(
	const string&		inDialogResource)
	: MWindow(MWindowImpl::CreateDialog(inDialogResource, this))
	, mParentWindow(nil)
	, mNext(nil)
	, mCloseImmediatelyOnOK(true)
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

//void MDialog::StdBtnClicked()
//{
//	const char* name = gtk_buildable_get_name(GTK_BUILDABLE(mStdBtnClicked.GetSourceGObject()));
//	if (name != nil)
//	{
//		uint32 id = 0;
//		for (uint32 i = 0; i < 4 and name[i]; ++i)
//			id = (id << 8) | name[i];
//		
//		try
//		{
//			switch (id)
//			{
//				case 'okok':
//					if (OKClicked())
//						Close();
//					break;
//				
//				case 'cncl':
//					if (CancelClicked())
//						Close();
//					break;
//				
//				default:
//					ValueChanged(id);
//					break;
//			}
//		}
//		catch (exception& e)
//		{
//			DisplayError(e);
//		}
//		catch (...) {}
//	}
//}

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
		
		SetWindowPosition(r, false);
	}
}

void MDialog::SetCloseImmediatelyFlag(
	bool inCloseImmediately)
{
	mCloseImmediatelyOnOK = inCloseImmediately;
}
