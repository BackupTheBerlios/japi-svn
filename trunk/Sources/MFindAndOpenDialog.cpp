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
#include "MTextController.h"
#include "MProjectWindow.h"
#include "MProject.h"
#include "MPreferences.h"
#include "MView.h"
#include "MUnicode.h"
#include "MUtils.h"
#include "MFile.h"
#include "MSound.h"
#include "MJapiApp.h"

using namespace std;

namespace {

enum {
	kTextBoxControlID = 'edit'
};

}

MFindAndOpenDialog::MFindAndOpenDialog(
	MTextController*	inController,
	MWindow*			inWindow)
	: MDialog("find-and-open-dialog")
	, mController(inController)
	, mProject(nil)
{
	SetText(kTextBoxControlID,
		Preferences::GetString("last open include", ""));

	Show(inWindow);
	SetFocus(kTextBoxControlID);
}

MFindAndOpenDialog::MFindAndOpenDialog(
	MProject*			inProject,
	MWindow*			inWindow)
	: MDialog("find-and-open-dialog")
	, mController(nil)
	, mProject(inProject)
{
	SetText(kTextBoxControlID,
		Preferences::GetString("last open include", ""));

	Show(inWindow);
	SetFocus(kTextBoxControlID);
}

bool MFindAndOpenDialog::OKClicked()
{
	string s;

	GetText(kTextBoxControlID, s);

	Preferences::SetString("last open include", s);

	if (mController != nil)
	{
		if (not mController->OpenInclude(s))
			PlaySound("warning");
	}
	else
	{
		MProject* project = mProject;
		
		if (project == nil)
			project = MProject::Instance();

		fs::path p(s);
		
		if (exists(p) or (project != nil and project->LocateFile(s, true, p)))
			gApp->OpenOneDocument(MFile(p));
	}
	
	return true;
}
