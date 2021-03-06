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
	kFileComboControlID = 'file'
};

}

MFindAndOpenDialog::MFindAndOpenDialog(
	MTextController*	inController,
	MWindow*			inWindow)
	: MDialog("find-and-open-dialog")
	, mController(inController)
	, mProject(nil)
{
	vector<string> last;
	
	Preferences::GetArray("open include", last);
	SetValues(kFileComboControlID, last);

	Show(inWindow);
	SetFocus(kFileComboControlID);
}

MFindAndOpenDialog::MFindAndOpenDialog(
	MProject*			inProject,
	MWindow*			inWindow)
	: MDialog("find-and-open-dialog")
	, mController(nil)
	, mProject(inProject)
{
	vector<string> last;
	
	Preferences::GetArray("open include", last);
	SetValues(kFileComboControlID, last);

	Show(inWindow);
	SetFocus(kFileComboControlID);
}

bool MFindAndOpenDialog::OKClicked()
{
	string s;

	GetText(kFileComboControlID, s);

	vector<string> last;
	Preferences::GetArray("open include", last);
	last.erase(remove(last.begin(), last.end(), s), last.end());
	last.insert(last.begin(), s);
	if (last.size() > 10)
		last.erase(last.end() - 1);
	Preferences::SetArray("open include", last);
	
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

		MFile file(s);
		
		if (file.IsLocal() and not file.Exists() and project != nil)
		{
			fs::path p(s);
			if (project->LocateFile(s, true, p))
				file = p;
		}
		
		gApp->OpenOneDocument(file);
	}
	
	return true;
}
