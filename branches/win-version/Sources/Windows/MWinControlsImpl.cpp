//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <Windows.h>
#include <Uxtheme.h>
#include <vssym32.h>

#undef GetNextWindow

#include "MLib.h"

#include "MWinWindowImpl.h"
#include "MWinControlsImpl.h"
#include "MWinUtils.h"

using namespace std;

MWinControlImpl::MWinControlImpl(MControl* inControl, const string& inLabel)
	: MControlImpl(inControl)
	, mLabel(inLabel)
{
}

MWinControlImpl::~MWinControlImpl()
{
}

MWinControlImpl* MWinControlImpl::FetchControlImpl(HWND inHWND)
{
	
}

string MWinControlImpl::GetText() const
{
	string result;

	if (GetHandle())
	{
		const int kBufferSize = 1024;
		vector<wchar_t> buf(kBufferSize);

		int n = ::GetWindowTextW(GetHandle(), &buf[0], kBufferSize);

		result = w2c(&buf[0]);
	}
	else
		result = mLabel;

	return result;
}

void MWinControlImpl::SetText(const std::string& inText)
{
	mLabel = inText;

	wstring s(c2w(inText));
	::SetWindowTextW(GetHandle(), s.c_str());
}

long MWinControlImpl::GetValue() const
{
	return mValue;
}

void MWinControlImpl::SetValue(long inValue)
{
	mValue = inValue;
}

void MWinControlImpl::ActivateSelf()
{
	::EnableWindow(GetHandle(), mControl->IsActive() and mControl->IsEnabled());
}

void MWinControlImpl::DeactivateSelf()
{
	if (::IsWindowEnabled(GetHandle()))
		::EnableWindow(GetHandle(), false);
}

void MWinControlImpl::EnableSelf()
{
	::EnableWindow(GetHandle(), mControl->IsActive() and mControl->IsEnabled());
}

void MWinControlImpl::DisableSelf()
{
	if (::IsWindowEnabled(GetHandle()))
		::EnableWindow(GetHandle(), false);
}

void MWinControlImpl::ShowSelf()
{
	::ShowWindow(GetHandle(), SW_SHOW);
}

void MWinControlImpl::HideSelf()
{
	::ShowWindow(GetHandle(), SW_HIDE);
}

void MWinControlImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	MWinProcMixin::CreateParams(outStyle, outExStyle, outClassName, outMenu);
}

void MWinControlImpl::GetParentAndBounds(MWinProcMixin*& outParent, MRect& outBounds)
{
	MView* view = mControl;
	MView* parent = view->GetParent();
	
	view->GetBounds(outBounds);
	
	while (parent != nil)
	{
		view->ConvertToParent(outBounds.x, outBounds.y);
		
		MControl* ctl = dynamic_cast<MControl*>(parent);
		
		if (ctl != nil)
		{
			MWinControlImpl* impl =
				dynamic_cast<MWinControlImpl*>(ctl->GetImpl());
			
			if (impl != nil)
			{
				outParent = impl;
				break;
			}
		}
		
		MWindow* window = dynamic_cast<MWindow*>(parent);
		if (window != nil)
		{
			outParent = static_cast<MWinWindowImpl*>(window->GetImpl());
			break;
		}
		
		view = parent;
		parent = parent->GetParent();
	}
}

void MWinControlImpl::AddedToWindow()
{
	MWinProcMixin* parent;
	MRect bounds;

	GetParentAndBounds(parent, bounds);
	bounds.width = 75;
	bounds.height = 23;

	Create(parent, bounds, c2w(GetText()));

	::SetWindowLongW(GetHandle(), 

	// set the font to the theme font

	HTHEME hTheme = ::OpenThemeData(GetHandle(), VSCLASS_TEXTSTYLE);
	if (hTheme != nil)
	{
		LOGFONT font;
		if (::GetThemeSysFont(hTheme, TMT_CAPTIONFONT, &font) == S_OK)
		{
			HFONT hFont = ::CreateFontIndirectW(&font);
			if (hFont != nil)
				::SendMessageW(GetHandle(), WM_SETFONT, (WPARAM)hFont, MAKELPARAM(true, 0));
		}

		::CloseThemeData(hTheme);
	}
	
	if (mControl->IsVisible())
		ShowSelf();
	else
		HideSelf();
}

MButtonImpl::MButtonImpl(MControl* inControl, const string& inLabel)
	: MWinControlImpl(inControl, inLabel)
{
}

void MButtonImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);

	outStyle = WS_CHILD | BS_PUSHBUTTON | BS_TEXT | WS_TABSTOP;
	outClassName = L"BUTTON";
}

void MButtonImpl::AddedToWindow()
{
	MWinControlImpl::AddedToWindow();

	AddNotify(BN_CLICKED, GetHandle(), boost::bind(&MButtonImpl::BNClicked, this, _1, _2, _3));
}

bool MButtonImpl::BNClicked(WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	PRINT(("Clicked!"));
	outResult = 1;
	return true;
}

MControlImpl* MControlImpl::CreateButton(MControl* inControl, const string& inLabel)
{
	return new MButtonImpl(inControl, inLabel);
}