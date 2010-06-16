//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MGoToLineDialog.cpp 80 2006-09-11 08:50:35Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 20:47:19
*/

#include "MJapi.h"

#include "MGoToLineDialog.h"
#include "MTextDocument.h"
#include "MPreferences.h"
#include "MView.h"
#include "MUtils.h"

using namespace std;

namespace {

const string
	kTextBoxControlID = "edit";

}

MGoToLineDialog::MGoToLineDialog(
	MTextDocument*	inDocument,
	MWindow*		inWindow)
	: MDialog("go-to-line-dialog")
	, mDocument(inDocument)
{
	Show(inWindow);
	SetFocus(kTextBoxControlID);
}

bool MGoToLineDialog::OKClicked()
{
	int32 lineNr = boost::lexical_cast<int32>(GetText(kTextBoxControlID));

	if (lineNr > 0 and mDocument != nil)
		mDocument->GoToLine(lineNr - 1);
	
	return true;
}
