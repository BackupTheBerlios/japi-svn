//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MFindAndOpenDialog.cpp 158 2007-05-28 10:08:18Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 20:47:19
*/

#include "MJapi.h"

#include "MFindAndOpenDialog.h"
#include "MTextDocument.h"
//#include "MProjectWindow.h"
//#include "MProject.h"
#include "MPreferences.h"
#include "MSound.h"
#include "MJapiApp.h"

using namespace std;

namespace {

const string
	kTextBoxControlID = "edit";

}

MFindAndOpenDialog::MFindAndOpenDialog(
	MTextDocument*		inDocument,
	MWindow*			inWindow)
	: MDialog("find-and-open-dialog")
	, mDocument(inDocument)
//	, mProject(nil)
{
	SetText(kTextBoxControlID,
		Preferences::GetString("last open include", ""));

	Show(inWindow);
	SetFocus(kTextBoxControlID);
}

//MFindAndOpenDialog::MFindAndOpenDialog(
//	MProject*			inProject,
//	MWindow*			inWindow)
//	: MDialog("find-and-open-dialog")
//	, mController(nil)
//	, mProject(inProject)
//{
//	SetText(kTextBoxControlID,
//		Preferences::GetString("last open include", ""));
//
//	Show(inWindow);
//	SetFocus(kTextBoxControlID);
//}

bool MFindAndOpenDialog::OKClicked()
{
	string s = GetText(kTextBoxControlID);

	Preferences::SetString("last open include", s);

	bool opened = false;

	if (mDocument != nil)
		opened = mDocument->OpenInclude(s);

	if (not opened)
	{
//		MProject* project = mProject;
//		
//		if (project == nil)
//			project = MProject::Instance();
//		if (project != nil and project->LocateFile(s, true, p))
//			opened = (gApp->OpenOneDocument(MFile(p)) != nil);
	}

	if (not opened)
	{
		fs::path p(s);
		if (fs::exists(p))
			opened = (gApp->OpenOneDocument(MFile(p)) != nil);
	}

	if (not opened)
		PlaySound("warning");
	
	return true;
}
