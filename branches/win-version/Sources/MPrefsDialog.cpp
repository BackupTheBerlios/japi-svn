//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <boost/lexical_cast.hpp>

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
	kWhiteSpaceColorID			= 'witc',
	kEncodingPopupID			= 'enco',
	kBOMCheckboxID				= 'abom',
	kNewlinePopupID				= 'newl',
	
	kPlaySoundsCheckboxID		= 'snds',
	
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
	kForceNLControlID			= 'nlae',
	
	kDefaultCompilerEditTextID	= 'comp',
	kConcurrentJobsEditTextID	= 'conc',
	
	kBoostLibraryNameExtEditTextID
								= 'bste';

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
	SetChecked(kTabEntersSpacesCheckboxID, Preferences::GetBoolean("tab enters spaces", false));
	SetText(kSpacesPerTabEditTextID, NumToString(gSpacesPerTab));
	SetEnabled(kSpacesPerTabEditTextID, gTabEntersSpaces);
	SetChecked(kBalanceWhileTypingCheckboxID, gKiss);
	SetChecked(kAutoIndentCheckboxID, gAutoIndent);
	SetChecked(kSmartIndentCheckboxID, gSmartIndent);
	SetEnabled(kSmartIndentCheckboxID, gAutoIndent);
	SetColor(kWhiteSpaceColorID, gWhiteSpaceColor);
	SetChecked(kPlaySoundsCheckboxID, gPlaySounds);
	
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
	
	SetChecked(kBOMCheckboxID, Preferences::GetBoolean("add bom", true));

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
	
	SetChecked(kOpenLastProjectControlID, Preferences::GetBoolean("reopen project", false));
	SetChecked(kOpenWorksheetControlID, Preferences::GetBoolean("open worksheet", false));
	SetChecked(kSaveStateControlID, Preferences::GetBoolean("save state", true));
	SetChecked(kForceNLControlID, Preferences::GetBoolean("force newline at eof", true));
	
	// compiler and build options
	
	SetText(kDefaultCompilerEditTextID, Preferences::GetString("c++", "/usr/bin/c++"));
	SetText(kConcurrentJobsEditTextID, boost::lexical_cast<string>(gConcurrentJobs));
	
	SetText(kBoostLibraryNameExtEditTextID, Preferences::GetString("boost-ext", "-mt"));
}

bool MPrefsDialog::DoClose()
{
	sInstance = nil;
	return true;
}

void MPrefsDialog::SelectPage(uint32 inPage)
{
}

bool MPrefsDialog::OKClicked()
{
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
			Preferences::SetBoolean("tab enters spaces",
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
			Preferences::SetBoolean("kiss", gKiss = IsChecked(kBalanceWhileTypingCheckboxID));
			break;
		
		case kAutoIndentCheckboxID:
			Preferences::SetBoolean("auto indent", gAutoIndent = IsChecked(kAutoIndentCheckboxID));
			SetEnabled(kSmartIndentCheckboxID, gAutoIndent);
			break;
		
		case kSmartIndentCheckboxID:
			Preferences::SetBoolean("smart indent", gSmartIndent = IsChecked(kSmartIndentCheckboxID));
			break;

		case kWhiteSpaceColorID:
			Preferences::SetColor("whitespace color",  gWhiteSpaceColor = GetColor(kWhiteSpaceColorID));
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
			Preferences::SetBoolean("add bom", IsChecked(kBOMCheckboxID));
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
		
		case kPlaySoundsCheckboxID:
			Preferences::SetBoolean("play sounds", gPlaySounds = IsChecked(kPlaySoundsCheckboxID));
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
			Preferences::SetBoolean("reopen project", IsChecked(kOpenLastProjectControlID));
			break;
		
		case kOpenWorksheetControlID:
			Preferences::SetBoolean("open worksheet", IsChecked(kOpenWorksheetControlID));
			break;
	
		case kSaveStateControlID:
			Preferences::SetBoolean("save state", IsChecked(kSaveStateControlID));
			break;
		
		case kForceNLControlID:
			Preferences::SetBoolean("force newline at eof", IsChecked(kForceNLControlID));
			break;
		
		case kDefaultCompilerEditTextID:
			Preferences::SetString("c++", GetText(kDefaultCompilerEditTextID));
			break;
		
		case kConcurrentJobsEditTextID:
			n = StringToNum(GetText(kConcurrentJobsEditTextID));
			if (n > 0 and n < 12)
				Preferences::SetInteger("concurrent-jobs", gConcurrentJobs = n);
			break;
		
		case kBoostLibraryNameExtEditTextID:
			Preferences::SetString("boost-ext", GetText(kBoostLibraryNameExtEditTextID));
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

