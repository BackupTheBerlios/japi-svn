//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include "MNewGroupDialog.h"

using namespace std;

namespace {

enum {
	kTextBoxControlID = 'edit'
};

}

MNewGroupDialog::MNewGroupDialog(
	MWindow*	inProject)
	: MDialog("new-group-dialog")
{
	Show(inProject);
	SetFocus(kTextBoxControlID);
}

bool MNewGroupDialog::OKClicked()
{
	string s;
	GetText(kTextBoxControlID, s);

	eCreateNewGroup(s);
	
	return true;
}
