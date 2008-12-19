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

#include "MJapi.h"

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
