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

#include "MStyles.h"
#include "MGlobals.h"
#include "MPrefsDialog.h"

MStyleArray* MStyleArray::sInstance;

MStyleArray::MStyleArray(HIObjectRef inObjectRef)
	: MHIObject(inObjectRef)
	, ePrefsChanged(this, &MStyleArray::PrefsChanged)
{
	sInstance = this;
	
	for (UInt32 ix = 0; ix < kLStyleCount; ++ix)
		THROW_IF_OSERROR(::ATSUCreateStyle(&mStyles[ix]));

	for (UInt32 ix = kRawText; ix < kSelectedText; ++ix)
		THROW_IF_OSERROR(::ATSUCreateStyle(&mInputStyles[ix]));

	Install(kEventClassFont, kEventFontSelection, this, &MStyleArray::HandleFontEvent);
	
	AddRoute(MPrefsDialog::ePrefsChanged, ePrefsChanged);
}

MStyleArray::~MStyleArray()
{
	for (unsigned int ix = 0; ix < kLStyleCount; ++ix)
		THROW_IF_OSERROR(::ATSUDisposeStyle(mStyles[ix]));

	for (unsigned int ix = kRawText; ix < kSelectedText; ++ix)
		THROW_IF_OSERROR(::ATSUDisposeStyle(mInputStyles[ix]));
}

OSStatus MStyleArray::Initialize(EventRef ioEvent)
{
	ReInit();
	return noErr;
}

void MStyleArray::ReInit()
{
	MCFString fontName(gFontName);
	
	mATSUFontID = ::FMGetFontFromATSFontRef(
		::ATSFontFindFromName(fontName, 0));
	mFontSize = Long2Fix(gFontSize);

	gSmoothFonts = gFontSize > 10 or
		(gFontName != "Monaco" and gFontName != "Courier" and gFontName != "Courier New");

	RGBColor color;

	ATSUAttributeTag		theTags[] =  { kATSUFontTag, kATSUSizeTag, kATSUColorTag };
	ByteCount				theSizes[] = { sizeof(ATSUFontID), sizeof(Fixed), sizeof(RGBColor) };
	ATSUAttributeValuePtr	theValues[] = { &mATSUFontID, &mFontSize, &color };

	for (unsigned int ix = 0; ix < kLStyleCount; ++ix)
	{
		gLanguageColors[ix].GetRGBColor(color);
		THROW_IF_OSERROR(
			::ATSUSetAttributes(mStyles[ix], 3, theTags, theSizes, theValues));
	}

	::SetFontInfoForSelection(kFontSelectionATSUIType, 1, mStyles, MHIObject::GetSysEventTarget());
	
	gLanguageColors[0].GetRGBColor(color);

	MCFRef<CGColorSpaceRef> space(::CGColorSpaceCreateDeviceRGB(), false);
	for (UInt32 ix = kRawText; ix < kSelectedText; ++ix)
	{
		THROW_IF_OSERROR(
			::ATSUSetAttributes(mInputStyles[ix], 3, theTags, theSizes, theValues));

		float data[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
		
		if (ix == kConvertedText or ix == kSelectedConvertedText)
		{
			data[0] = 1.0f; data[1] = 0.4f; data[2] = 0.4f;
		}

		ATSUStyleLineCountType lineCount = kATSUStyleSingleLineCount;
		Boolean underLine = true;
		CGColorRef color = ::CGColorCreate(space, data);

		ATSUAttributeTag tag[] = { kATSUQDUnderlineTag, kATSUStyleUnderlineCountOptionTag, kATSUStyleUnderlineColorOptionTag };
		ByteCount size[] = { sizeof(Boolean), sizeof(ATSUStyleLineCountType), sizeof(CGColorRef) };
		ATSUAttributeValuePtr value[] = { &underLine, &lineCount, &color };
		
		THROW_IF_OSERROR(::ATSUSetAttributes(mInputStyles[ix], 3, tag, size, value));
		
		::CFRelease(color);
	}
	
//	InitReplacementGlyphs(mStyles[0]);
}

MStyleArray&
MStyleArray::Instance()
{
	if (sInstance == nil)
	{
		RegisterSubclass<MStyleArray>();
		
		// instantiate the object
		ControlRef control;
		MCarbonEvent event(kEventClassHIObject, kEventHIObjectInitialize);
		
		THROW_IF_OSERROR(::HIObjectCreate(GetClassID(), event,
			reinterpret_cast<HIObjectRef*>(&control)));
	}
	
	return *sInstance;
}

OSStatus MStyleArray::HandleFontEvent(EventRef ioEvent)
{
	bool reinit = false;

	if (::GetEventParameter(ioEvent, kEventParamATSUFontID, typeATSUFontID, nil,
		sizeof(mATSUFontID), nil, &mATSUFontID) == noErr)
	{
		CFStringRef name;
		THROW_IF_OSERROR(::ATSFontGetName(::FMGetATSFontRefFromFont(mATSUFontID),
			kATSOptionFlagsDefault, &name));
		
		MCFString(name, false).GetString(gFontName);
		reinit = true;
	}
	
	if (::GetEventParameter(ioEvent, kEventParamATSUFontSize, typeATSUSize, nil,
		sizeof(mFontSize), nil, &mFontSize) == noErr)
	{
		gFontSize = Fix2Long(mFontSize);
		reinit = true;
	}

	RGBColor color;
	if (::GetEventParameter(ioEvent, kEventParamFontColor, typeFontColor, nil,
		sizeof(color), nil, &color) == noErr)
	{
		gLanguageColors[0] = color;
		reinit = true;
	}
	
	if (reinit)
	{
		ReInit();
		eStylesChanged();
	}
	
//	if (n > 0)
//	{
//		for (UInt32 ix = 0; ix < kLStyleCount; ++ix)
//			::ATSUSetAttributes(mStyles[ix], n, theTags, theSizes, theValues);
//
//		for (UInt32 ix = kRawText; ix < kSelectedText; ++ix)
//			::ATSUSetAttributes(mInputStyles[ix], n, theTags, theSizes, theValues);
//		
//		::ATSUSetAttributes(mInvisiblesStyle, n, theTags, theSizes, theValues);
//	}
	
	return noErr;
}

void MStyleArray::PrefsChanged()
{
	ReInit();
}
