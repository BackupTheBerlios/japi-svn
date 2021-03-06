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
