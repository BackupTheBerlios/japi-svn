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
#include "MUrl.h"
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

		MPath p(s);
		
		if (exists(p) or (project != nil and project->LocateFile(s, true, p)))
			gApp->OpenOneDocument(MUrl(p));
	}
	
	return true;
}
