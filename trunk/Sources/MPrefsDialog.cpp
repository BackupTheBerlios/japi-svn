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

#include "Japie.h"

#include "MGlobals.h"
#include "MColorSwatch.h"
#include "MPrefsDialog.h"
#include "MLanguage.h"
#include "MUtils.h"
#include "MPreferences.h"

using namespace std;

namespace {

const UInt32
	kPageIDs[] = { 0, 128, 129, 130, 131, 132 },
	kPageCount = sizeof(kPageIDs) / sizeof(UInt32) - 1,
	kTabControlID = 140;

const UInt32
	kTabWidthEditTextID			= 101,
	kTabEntersSpacesCheckboxID	= 102,
	kSpacesPerTabEditTextID		= 103,
	kBalanceWhileTypingCheckboxID	= 104,
	kAutoIndentCheckboxID		= 105,
	kSmartIndentCheckboxID		= 106,
	kEncodingPopupID			= 107,
	kBOMCheckboxID				= 108,
	kNewlinePopupID				= 109,
	
	kKeywordColorID				= 201,
	kPreprocessorColorID		= 202,
	kCharConstColorID			= 203,
	kCommentColorID				= 204,
	kStringColorID				= 205,
	kHTMLTagColorID				= 206,
	kHTMLAttributeColorID		= 207,
	
	kOpenOptionControlID		= 501,
	kOpenLastProjectControlID	= 502;

}

MEventOut<void()>	MPrefsDialog::ePrefsChanged;

MPrefsDialog::MPrefsDialog()
{
}

void MPrefsDialog::Create()
{
	auto_ptr<MPrefsDialog> instance(new MPrefsDialog);
	instance->Initialize();
	instance->Show(nil);
	instance->Select();
	instance.release();
}
	
void MPrefsDialog::Initialize()
{
	MView::RegisterSubclass<MColorSwatch>();
	
	MDialog::Initialize(CFSTR("Preferences"));

	// set up the first page
	
	string txt;
	
	SetText(kTabWidthEditTextID, NumToString(gCharsPerTab));
	SetChecked(kTabEntersSpacesCheckboxID, Preferences::GetInteger("tab enters spaces", 0));
	SetText(kSpacesPerTabEditTextID, NumToString(gSpacesPerTab));
	SetChecked(kBalanceWhileTypingCheckboxID, gKiss);
	SetChecked(kAutoIndentCheckboxID, gAutoIndent);
	SetChecked(kSmartIndentCheckboxID, gSmartIndent);
	
	string s = Preferences::GetString("default encoding", "utf-8");
	if (s == "utf-8")
		SetValue(kEncodingPopupID, 1);
	else if (s == "utf-16 be")
		SetValue(kEncodingPopupID, 2);
	else if (s == "utf-16 le")
		SetValue(kEncodingPopupID, 3);
	else if (s == "mac os roman")
		SetValue(kEncodingPopupID, 4);
	
	SetChecked(kBOMCheckboxID, Preferences::GetInteger("add bom", 1));

	s = Preferences::GetString("newline char", "LF");
	if (s == "LF")
		SetValue(kNewlinePopupID, 1);
	else if (s == "CR")
		SetValue(kNewlinePopupID, 2);
	else if (s == "CRLF")
		SetValue(kNewlinePopupID, 3);
	
	// set up the color swatches, keyword first
	mColorSwatches[kKeywordSwatchNr] = MakeSwatch(kKeywordColorID, gLanguageColors[kLKeyWordColor]);
	mColorSwatches[kPreprocessorSwatchNr] = MakeSwatch(kPreprocessorColorID, gLanguageColors[kLPreProcessorColor]);
	mColorSwatches[kCharConstSwatchNr] = MakeSwatch(kCharConstColorID, gLanguageColors[kLCharConstColor]);
	mColorSwatches[kCommentSwatchNr] = MakeSwatch(kCommentColorID, gLanguageColors[kLCommentColor]);
	mColorSwatches[kStringSwatchNr] = MakeSwatch(kStringColorID, gLanguageColors[kLStringColor]);
	mColorSwatches[kHTMLTagSwatchNr] = MakeSwatch(kHTMLTagColorID, gLanguageColors[kLTagColor]);
	mColorSwatches[kHTMLAttributeSwatchNr] = MakeSwatch(kHTMLAttributeColorID, gLanguageColors[kLAttribColor]);

	// set up startup options
	
	SetValue(kOpenOptionControlID, Preferences::GetInteger("at launch", 1));
	SetChecked(kOpenLastProjectControlID, Preferences::GetInteger("reopen project", 0));
	
	// setup the tab control
	ControlRef tabControl = FindControl(kTabControlID);
	
	::SetControlValue(tabControl, 1);
	SelectPage(1);
}

MColorSwatch* MPrefsDialog::MakeSwatch(UInt32 inID, MColor inColor)
{
	ControlRef placeholder = FindControl(inID);
	
	HIRect frame;
	::HIViewGetFrame(placeholder, &frame);
	
	MColorSwatch* result = MView::Create<MColorSwatch>(::HIViewGetSuperview(placeholder), frame);

	result->SetColor(inColor);
	result->SetEnabled(true);
	result->SetVisible(true);

	return result;
}

void MPrefsDialog::SelectPage(UInt32 inPage)
{
	ControlRef select = nil;
	
	for (UInt32 page = 1; page <= kPageCount; ++page)
	{
		ControlRef pane = FindControl(kPageIDs[page]);
		
		if (page == inPage)
			select = pane;
		else
		{
            ::SetControlVisibility(pane, false, false);
            ::DisableControl(pane);
		}
	}
	
    if (select != NULL)
    {
        ::EnableControl(select);
        ::SetControlVisibility(select, true, true);
    }

	mCurrentPage = inPage;
}

OSStatus MPrefsDialog::DoControlHit(EventRef inEvent)
{
    ControlRef theControl;
	::GetEventParameter(inEvent, kEventParamDirectObject,
		typeControlRef, NULL, sizeof(ControlRef), NULL, &theControl);

	ControlID controlID;
	::GetControlID(theControl, &controlID);
	
	OSStatus result = noErr;
	
	switch (controlID.id)
	{
		case kTabControlID:
			if (static_cast<uint32>(::GetControlValue(theControl)) != mCurrentPage)
			{
				SelectPage(::GetControlValue(theControl));
				result = noErr;
			}
			break;
		
		default:
	    	result = MDialog::DoControlHit(inEvent);
	}
    
    return result;
}

bool MPrefsDialog::OKClicked()
{
	// save the changed values
	
	string s;
	
	GetText(kTabWidthEditTextID, s);
	UInt32 n = StringToNum(s);
	if (n > 0 and n < 100)
		Preferences::SetInteger("chars per tab", gCharsPerTab = n);
	
	Preferences::SetInteger("tab enters spaces", gTabEntersSpaces = IsChecked(kTabEntersSpacesCheckboxID));

	GetText(kSpacesPerTabEditTextID, s);
	n = StringToNum(s);
	if (n > 0 and n < 100)
		Preferences::SetInteger("spaces per tab", gSpacesPerTab = n);

	Preferences::SetInteger("kiss", gKiss = IsChecked(kBalanceWhileTypingCheckboxID));
	Preferences::SetInteger("auto indent", gAutoIndent = IsChecked(kAutoIndentCheckboxID));
	Preferences::SetInteger("smart indent", gSmartIndent = IsChecked(kSmartIndentCheckboxID));
	
	switch (GetValue(kEncodingPopupID))
	{
		case 1:
			Preferences::SetString("default encoding", "utf-8");
			break;
		
		case 2:
			Preferences::SetString("default encoding", "utf-16 be");
			break;
		
		case 3:
			Preferences::SetString("default encoding", "utf-16 le");
			break;
		
		case 4:
			Preferences::SetString("default encoding", "mac os roman");
			break;
	}
	
	Preferences::SetInteger("add bom", IsChecked(kBOMCheckboxID));
	
	switch (GetValue(kNewlinePopupID))
	{
		case 1:
			Preferences::SetString("newline char", "LF");
			break;
		
		case 2:
			Preferences::SetString("newline char", "CR");
			break;
		
		case 3:
			Preferences::SetString("newline char", "CRLF");
			break;
		
	}
	
	// appearance
	
//	Preferences::SetColor("text color",
//	gLanguageColors[kLTextColor] = mColorSwatches[0]->GetColor());

	Preferences::SetColor("keyword color",
		gLanguageColors[kLKeyWordColor] = mColorSwatches[kKeywordSwatchNr]->GetColor());
	Preferences::SetColor("preprocessor color",
		gLanguageColors[kLPreProcessorColor] = mColorSwatches[kPreprocessorSwatchNr]->GetColor());
	Preferences::SetColor("char const color",
		gLanguageColors[kLCharConstColor] = mColorSwatches[kCharConstSwatchNr]->GetColor());
	Preferences::SetColor("comment color",
		gLanguageColors[kLCommentColor] = mColorSwatches[kCommentSwatchNr]->GetColor());
	Preferences::SetColor("string color", 
		gLanguageColors[kLStringColor] = mColorSwatches[kStringSwatchNr]->GetColor());
	Preferences::SetColor("tag color", 
		gLanguageColors[kLTagColor] = mColorSwatches[kHTMLTagSwatchNr]->GetColor());
	Preferences::SetColor("attribute color", 
		gLanguageColors[kLAttribColor] = mColorSwatches[kHTMLAttributeSwatchNr]->GetColor());

//	Preferences::GetColor("invisibles color", 
//		gLanguageColors[kLInvisiblesColor] = mColorSwatches[7]->GetColor());
	
	// startup options
	
	Preferences::SetInteger("at launch", GetValue(kOpenOptionControlID));
	Preferences::SetInteger("reopen project", IsChecked(kOpenLastProjectControlID));
	
	ePrefsChanged();
	
	return true;
}

bool MPrefsDialog::CancelClicked()
{
	return true;
}
