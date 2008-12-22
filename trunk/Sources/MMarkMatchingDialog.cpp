//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MMarkMatchingDialog.cpp 75 2006-09-05 09:56:43Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 20:47:19
*/

#include "MJapi.h"

#include "MMarkMatchingDialog.h"
#include "MTextDocument.h"
#include "MPreferences.h"
#include "MView.h"
#include "MUtils.h"

using namespace std;

namespace {

enum
{
	kTextBoxControlID = 'edit',
	kIgnoreCaseControlID = 'ignc',
	kRegularExpressionControlID = 'regx'
};

}

MMarkMatchingDialog::MMarkMatchingDialog(
	MTextDocument*	inDocument,
	MWindow*		inWindow)
	: MDialog("mark-matching-dialog")
	, mDocument(inDocument)
{
	Show(inWindow);
	SetFocus(kTextBoxControlID);
}

bool MMarkMatchingDialog::OKClicked()
{
	string txt;
	GetText(kTextBoxControlID, txt);

	if (txt.length() > 0 and mDocument != nil)
	{
		mDocument->MarkMatching(txt, 
			IsChecked(kIgnoreCaseControlID),
			IsChecked(kRegularExpressionControlID));
	}
	
	return true;
}
