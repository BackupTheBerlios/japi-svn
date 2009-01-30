//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <boost/lexical_cast.hpp>

#include "MQuotedRewrapDialog.h"
#include "MTextDocument.h"
#include "MPreferences.h"
#include "MView.h"
#include "MUtils.h"

using namespace std;

namespace {

enum
{
	kQuoteCharsControlID = 'char',
	kWrapWidthControlID = 'wdth'
};

}

MQuotedRewrapDialog::MQuotedRewrapDialog(
	MTextDocument*	inDocument,
	MWindow*		inWindow)
	: MDialog("quoted-rewrap-dialog")
	, mDocument(inDocument)
{
	SetText(kQuoteCharsControlID,
		Preferences::GetString("quoted-rewrap-chars", ">:-()<[]"));
	SetText(kWrapWidthControlID,
		Preferences::GetString("quoted-rewrap-width", "72"));
	
	Show(inWindow);
	SetFocus(kQuoteCharsControlID);
}

bool MQuotedRewrapDialog::OKClicked()
{
	string chars, width;
	GetText(kQuoteCharsControlID, chars);
	GetText(kWrapWidthControlID, width);
	
	Preferences::SetString("quoted-rewrap-chars", chars);
	Preferences::SetString("quoted-rewrap-width", width);
	
	mDocument->QuotedRewrap(chars, boost::lexical_cast<uint32>(width));
	
	return true;
}
