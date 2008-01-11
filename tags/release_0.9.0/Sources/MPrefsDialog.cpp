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

#include "MJapieG.h"

#include "MGlobals.h"
#include "MPrefsDialog.h"
#include "MLanguage.h"
#include "MUtils.h"
#include "MPreferences.h"

using namespace std;

namespace {

const uint32
	kPageIDs[] = { 0, 128, 129, 130, 131, 132 },
	kPageCount = sizeof(kPageIDs) / sizeof(uint32) - 1,
	kTabControlID = 140;

const uint32
	kTabWidthEditTextID			= 'tabw',
	kTabEntersSpacesCheckboxID	= 'stab',
	kSpacesPerTabEditTextID		= 'tabs',
	kBalanceWhileTypingCheckboxID	= 'bala',
	kAutoIndentCheckboxID		= 'auto',
	kSmartIndentCheckboxID		= 'smar',
	kEncodingPopupID			= 'enco',
	kBOMCheckboxID				= 'abom',
	kNewlinePopupID				= 'newl',
	
	kKeywordColorID				= 'clr1',
	kPreprocessorColorID		= 'clr2',
	kCharConstColorID			= 'clr3',
	kCommentColorID				= 'clr4',
	kStringColorID				= 'clr5',
	kHTMLTagColorID				= 'clr6',
	kHTMLAttributeColorID		= 'clr7',
	
	kFontButtonID				= 'font',
	kSelectionColorID			= 'clr8',
	kMarkedLineColorID			= 'clr9',
	kCurrentLineColorID			= 'cl10',
	
	kOpenLastProjectControlID	= 'lstp',
	kOpenWorksheetControlID		= 'wrks',
	kSaveStateControlID			= 'stat',
	kForceNLControlID			= 'nlae';

}

MEventOut<void()>	MPrefsDialog::ePrefsChanged;
MPrefsDialog* MPrefsDialog::sInstance;

void MPrefsDialog::Create()
{
	if (sInstance != nil)
		sInstance->Select();
	else
	{
		sInstance = new MPrefsDialog();
		sInstance->Select();
	}
}
	
MPrefsDialog::MPrefsDialog()
	: MDialog("prefs-dialog")
{
	string txt;
	
	SetText(kTabWidthEditTextID, NumToString(gCharsPerTab));
	SetChecked(kTabEntersSpacesCheckboxID, Preferences::GetInteger("tab enters spaces", 0));
	SetText(kSpacesPerTabEditTextID, NumToString(gSpacesPerTab));
	SetEnabled(kSpacesPerTabEditTextID, gTabEntersSpaces);
	SetChecked(kBalanceWhileTypingCheckboxID, gKiss);
	SetChecked(kAutoIndentCheckboxID, gAutoIndent);
	SetChecked(kSmartIndentCheckboxID, gSmartIndent);
	SetEnabled(kSmartIndentCheckboxID, gAutoIndent);
	
	string s = Preferences::GetString("default encoding", "utf-8");
	if (s == "utf-8")
		SetValue(kEncodingPopupID, 1);
	else if (s == "utf-16 be")
		SetValue(kEncodingPopupID, 2);
	else if (s == "utf-16 le")
		SetValue(kEncodingPopupID, 3);
	else if (s == "mac os roman")
		SetValue(kEncodingPopupID, 4);
	else if (s == "iso-8859-1")
		SetValue(kEncodingPopupID, 5);
	
	SetChecked(kBOMCheckboxID, Preferences::GetInteger("add bom", 1));

	s = Preferences::GetString("newline char", "LF");
	if (s == "LF")
		SetValue(kNewlinePopupID, 1);
	else if (s == "CR")
		SetValue(kNewlinePopupID, 2);
	else if (s == "CRLF")
		SetValue(kNewlinePopupID, 3);
	
	// set up the color swatches, keyword first
	SetColor(kKeywordColorID, gLanguageColors[kLKeyWordColor]);
	SetColor(kPreprocessorColorID, gLanguageColors[kLPreProcessorColor]);
	SetColor(kCharConstColorID, gLanguageColors[kLCharConstColor]);
	SetColor(kCommentColorID, gLanguageColors[kLCommentColor]);
	SetColor(kStringColorID, gLanguageColors[kLStringColor]);
	SetColor(kHTMLTagColorID, gLanguageColors[kLTagColor]);
	SetColor(kHTMLAttributeColorID, gLanguageColors[kLAttribColor]);
	
	SetText(kFontButtonID, Preferences::GetString("font", "monospace 9"));

	SetColor(kSelectionColorID, gHiliteColor);
	SetColor(kMarkedLineColorID, gMarkedLineColor);
	SetColor(kCurrentLineColorID, gCurrentLineColor);

	// set up startup options
	
	SetChecked(kOpenLastProjectControlID, Preferences::GetInteger("reopen project", 0));
	SetChecked(kOpenWorksheetControlID, Preferences::GetInteger("open worksheet", 0));
	SetChecked(kSaveStateControlID, Preferences::GetInteger("save state", 1));
	SetChecked(kForceNLControlID, Preferences::GetInteger("force newline at eof", 1));
}

bool MPrefsDialog::DoClose()
{
	sInstance = nil;
	return true;
}

void MPrefsDialog::SelectPage(uint32 inPage)
{
//	ControlRef select = nil;
//	
//	for (uint32 page = 1; page <= kPageCount; ++page)
//	{
//		ControlRef pane = FindControl(kPageIDs[page]);
//		
//		if (page == inPage)
//			select = pane;
//		else
//		{
//            ::SetControlVisibility(pane, false, false);
//            ::DisableControl(pane);
//		}
//	}
//	
//    if (select != NULL)
//    {
//        ::EnableControl(select);
//        ::SetControlVisibility(select, true, true);
//    }
//
//	mCurrentPage = inPage;
}

//OSStatus MPrefsDialog::DoControlHit(EventRef inEvent)
//{
//    ControlRef theControl;
//	::GetEventParameter(inEvent, kEventParamDirectObject,
//		typeControlRef, NULL, sizeof(ControlRef), NULL, &theControl);
//
//	ControlID controlID;
//	::GetControlID(theControl, &controlID);
//	
//	OSStatus result = noErr;
//	
//	switch (controlID.id)
//	{
//		case kTabControlID:
//			if (static_cast<uint32>(::GetControlValue(theControl)) != mCurrentPage)
//			{
//				SelectPage(::GetControlValue(theControl));
//				result = noErr;
//			}
//			break;
//		
//		default:
//	    	result = MDialog::DoControlHit(inEvent);
//	}
//    
//    return result;
//}
//
bool MPrefsDialog::OKClicked()
{
//	// save the changed values
//	
//	string s;
//	
//	GetText(kTabWidthEditTextID, s);
//	uint32 n = StringToNum(s);
//	if (n > 0 and n < 100)
//		Preferences::SetInteger("chars per tab", gCharsPerTab = n);
//	
//	Preferences::SetInteger("tab enters spaces", gTabEntersSpaces = IsChecked(kTabEntersSpacesCheckboxID));
//
//	GetText(kSpacesPerTabEditTextID, s);
//	n = StringToNum(s);
//	if (n > 0 and n < 100)
//		Preferences::SetInteger("spaces per tab", gSpacesPerTab = n);
//
//	Preferences::SetInteger("kiss", gKiss = IsChecked(kBalanceWhileTypingCheckboxID));
//	Preferences::SetInteger("auto indent", gAutoIndent = IsChecked(kAutoIndentCheckboxID));
//	Preferences::SetInteger("smart indent", gSmartIndent = IsChecked(kSmartIndentCheckboxID));
//	
//	switch (GetValue(kEncodingPopupID))
//	{
//		case 1:
//			Preferences::SetString("default encoding", "utf-8");
//			break;
//		
//		case 2:
//			Preferences::SetString("default encoding", "utf-16 be");
//			break;
//		
//		case 3:
//			Preferences::SetString("default encoding", "utf-16 le");
//			break;
//		
//		case 4:
//			Preferences::SetString("default encoding", "mac os roman");
//			break;
//	}
//	
//	Preferences::SetInteger("add bom", IsChecked(kBOMCheckboxID));
//	
//	switch (GetValue(kNewlinePopupID))
//	{
//		case 1:
//			Preferences::SetString("newline char", "LF");
//			break;
//		
//		case 2:
//			Preferences::SetString("newline char", "CR");
//			break;
//		
//		case 3:
//			Preferences::SetString("newline char", "CRLF");
//			break;
//		
//	}
//	
//	// appearance
//	
////	Preferences::SetColor("text color",
////	gLanguageColors[kLTextColor] = mColorSwatches[0]->GetColor());
//
//
////	Preferences::GetColor("invisibles color", 
////		gLanguageColors[kLInvisiblesColor] = mColorSwatches[7]->GetColor());
//	
//	// startup options
//	
//	Preferences::SetInteger("at launch", GetValue(kOpenOptionControlID));
//	Preferences::SetInteger("reopen project", IsChecked(kOpenLastProjectControlID));
//	
//	ePrefsChanged();
	
	return true;
}

void MPrefsDialog::ValueChanged(
	uint32			inID)
{
	string s;
	uint32 n;
	bool handled = true;

	switch (inID)
	{
		case kTabWidthEditTextID:
			GetText(kTabWidthEditTextID, s);
			n = StringToNum(s);
			if (n > 0 and n < 100)
				Preferences::SetInteger("chars per tab", gCharsPerTab = n);
			break;

		case kTabEntersSpacesCheckboxID:
			Preferences::SetInteger("tab enters spaces",
				gTabEntersSpaces = IsChecked(kTabEntersSpacesCheckboxID));
			SetEnabled(kSpacesPerTabEditTextID, gTabEntersSpaces);
			break;
		
		case kSpacesPerTabEditTextID:
			GetText(kSpacesPerTabEditTextID, s);
			n = StringToNum(s);
			if (n > 0 and n < 100)
				Preferences::SetInteger("spaces per tab", gSpacesPerTab = n);
			break;

		case kBalanceWhileTypingCheckboxID:
			Preferences::SetInteger("kiss", gKiss = IsChecked(kBalanceWhileTypingCheckboxID));
			break;
		
		case kAutoIndentCheckboxID:
			Preferences::SetInteger("auto indent", gAutoIndent = IsChecked(kAutoIndentCheckboxID));
			SetEnabled(kSmartIndentCheckboxID, gAutoIndent);
			break;
		
		case kSmartIndentCheckboxID:
			Preferences::SetInteger("smart indent", gSmartIndent = IsChecked(kSmartIndentCheckboxID));
			break;

		case kEncodingPopupID:
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
			break;

		case kBOMCheckboxID:
			Preferences::SetInteger("add bom", IsChecked(kBOMCheckboxID));
			break;
		
		case kNewlinePopupID:
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
			break;
		
		case kKeywordColorID:
			Preferences::SetColor("keyword color",
				gLanguageColors[kLKeyWordColor] = GetColor(kKeywordSwatchNr));
			break;
		
		case kPreprocessorColorID:
			Preferences::SetColor("preprocessor color",
				gLanguageColors[kLPreProcessorColor] = GetColor(kPreprocessorSwatchNr));
			break;
		
		case kCharConstColorID:
			Preferences::SetColor("char const color",
				gLanguageColors[kLCharConstColor] = GetColor(kCharConstSwatchNr));
			break;
		
		case kCommentColorID:
			Preferences::SetColor("comment color",
				gLanguageColors[kLCommentColor] = GetColor(kCommentSwatchNr));
			break;
		
		case kStringColorID:
			Preferences::SetColor("string color", 
				gLanguageColors[kLStringColor] = GetColor(kStringSwatchNr));
			break;
		
		case kHTMLTagColorID:
			Preferences::SetColor("tag color", 
				gLanguageColors[kLTagColor] = GetColor(kHTMLTagSwatchNr));
			break;
		
		case kHTMLAttributeColorID:
			Preferences::SetColor("attribute color", 
				gLanguageColors[kLAttribColor] = GetColor(kHTMLAttributeSwatchNr));
			break;
		
		case kFontButtonID:
			GetText(kFontButtonID, s);
			Preferences::SetString("font", s);
			break;
		
		case kSelectionColorID:
			Preferences::SetColor("hilite color",  gHiliteColor = GetColor(kSelectionColorID));
			break;
		
		case kMarkedLineColorID:
			Preferences::SetColor("marked line color",  gMarkedLineColor = GetColor(kMarkedLineColorID));
			break;
		
		case kCurrentLineColorID:
			Preferences::SetColor("current line color",  gCurrentLineColor = GetColor(kCurrentLineColorID));
			break;

		case kOpenLastProjectControlID:
			Preferences::SetInteger("reopen project", IsChecked(kOpenLastProjectControlID));
			break;
		
		case kOpenWorksheetControlID:
			Preferences::SetInteger("open worksheet", IsChecked(kOpenWorksheetControlID));
			break;
	
		case kSaveStateControlID:
			Preferences::SetInteger("save state", IsChecked(kSaveStateControlID));
			break;
		
		case kForceNLControlID:
			Preferences::SetInteger("force newline at eof", IsChecked(kForceNLControlID));
			break;
		
		default:
			handled = false;
			break;
	}
	
	if (handled)
		ePrefsChanged();
	else
		PRINT(("Changed preferences %x not handled", inID));
}

