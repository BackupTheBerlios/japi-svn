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

const int kScrollbarWidth = 16;

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
	return reinterpret_cast<MWinControlImpl*>(::GetPropW(inHWND, L"control_impl"));
}

string MWinControlImpl::GetText() const
{
	string result;

	if (GetHandle())
	{
		const int kBufferSize = 1024;
		vector<wchar_t> buf(kBufferSize);

		::GetWindowTextW(GetHandle(), &buf[0], kBufferSize);

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

void MWinControlImpl::Draw(MRect inUpdate)
{
}

void MWinControlImpl::FrameResized()
{
	if (GetHandle() != nil)
	{
		MRect bounds;
		MWinProcMixin* parent;
		
		GetParentAndBounds(parent, bounds);

		::MoveWindow(GetHandle(), bounds.x, bounds.y,
			bounds.width, bounds.height, false);
	}
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

	Create(parent, bounds, c2w(GetText()));

	::SetPropW(GetHandle(), L"control_impl", this);

	// set the font to the theme font
	HTHEME theme = ::OpenThemeData(GetHandle(), VSCLASS_TEXTSTYLE);
	if (theme != nil)
	{
		LOGFONT font;
		if (::GetThemeSysFont(theme, TMT_CAPTIONFONT, &font) == S_OK)
		{
			HFONT hFont = ::CreateFontIndirectW(&font);
			if (hFont != nil)
				::SendMessageW(GetHandle(), WM_SETFONT, (WPARAM)hFont, MAKELPARAM(true, 0));
		}

		::CloseThemeData(theme);
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

bool MWinControlImpl::WMCommand(HWND inHWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool result = false;

	if (inMsg == BN_CLICKED)
	{
		MButton* button = dynamic_cast<MButton*>(mControl);
		if (button != nil)
			button->eClicked();

		outResult = 1;
		result = true;
	}

	return result;
}

MControlImpl* MControlImpl::CreateButton(MControl* inControl, const string& inLabel)
{
	return new MButtonImpl(inControl, inLabel);
}

// --------------------------------------------------------------------

MScrollbarImpl::MScrollbarImpl(MControl* inControl)
	: MWinControlImpl(inControl)
{
}

void MScrollbarImpl::ShowSelf()
{
	::ShowScrollBar(GetHandle(), SB_CTL, TRUE);
}

void MScrollbarImpl::HideSelf()
{
	::ShowScrollBar(GetHandle(), SB_CTL, FALSE);
}

void MScrollbarImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);
	
	outClassName = L"SCROLLBAR";
	outStyle = WS_CHILD;

	MRect bounds;
	mControl->GetBounds(bounds);

	if (bounds.width > bounds.height)
		outStyle |= SBS_HORZ;
	else
		outStyle |= SBS_VERT;
}

long MScrollbarImpl::GetValue() const
{
	SCROLLINFO info = { sizeof(SCROLLINFO), SIF_POS };
	
	if (GetHandle() != nil)
		::GetScrollInfo(GetHandle(), SB_CTL, &info);

	return info.nPos;
}

void MScrollbarImpl::SetValue(long inValue)
{
	if (GetHandle() != nil)
	{
		SCROLLINFO info = { sizeof(SCROLLINFO), SIF_POS };
		info.nPos = inValue;
		
		::SetScrollInfo(GetHandle(), SB_CTL, &info, true);
	}
}

long MScrollbarImpl::GetMinValue() const
{
	SCROLLINFO info = { sizeof(SCROLLINFO), SIF_RANGE };
	
	if (GetHandle() != nil)
		::GetScrollInfo(GetHandle(), SB_CTL, &info);

	return info.nMin;
}

void MScrollbarImpl::SetMinValue(long inValue)
{
	if (GetHandle() != nil)
	{
		SCROLLINFO info = { sizeof(SCROLLINFO), SIF_RANGE };
	
		::GetScrollInfo(GetHandle(), SB_CTL, &info);
		info.nMin = inValue;
		::SetScrollInfo(GetHandle(), SB_CTL, &info, true);
	}
}

long MScrollbarImpl::GetMaxValue() const
{
	SCROLLINFO info = { sizeof(SCROLLINFO), SIF_RANGE };
	if (GetHandle() != nil)
		::GetScrollInfo(GetHandle(), SB_CTL, &info);

	return info.nMax;
}

void MScrollbarImpl::SetMaxValue(long inValue)
{
	if (GetHandle() != nil)
	{
		SCROLLINFO info = { sizeof(SCROLLINFO), SIF_RANGE };
	
		::GetScrollInfo(GetHandle(), SB_CTL, &info);
		info.nMax = inValue;
		::SetScrollInfo(GetHandle(), SB_CTL, &info, true);
	}
}

bool MScrollbarImpl::WMScroll(HWND inHandle, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool result = false;

	MScrollbar* scrollbar = dynamic_cast<MScrollbar*>(mControl);

	if (scrollbar != nil and (inUMsg == WM_HSCROLL or inUMsg == WM_VSCROLL))
	{
		outResult = 0;
		result = true;

		switch (LOWORD(inWParam))
		{
			case SB_LINEDOWN:
				scrollbar->eScroll(kScrollLineDown);
				break;
			case SB_LINEUP:
				scrollbar->eScroll(kScrollLineUp);
				break;
			case SB_PAGEDOWN:
				scrollbar->eScroll(kScrollPageDown);
				break;
			case SB_PAGEUP:
				scrollbar->eScroll(kScrollPageUp);
				break;
			case SB_THUMBTRACK:
			{
				SCROLLINFO info = { sizeof(info), SIF_TRACKPOS };
				::GetScrollInfo(GetHandle(), SB_CTL, &info);
				SetValue(info.nTrackPos);
				scrollbar->eScroll(kScrollToThumb);
				break;
			}
		}
		mControl->GetWindow()->UpdateNow();
	}

	return result;
}

//void MScrollbarImpl::SetViewSize(long inViewSize)
//{
//#if 0
//	SCROLLINFO info = { 0 };
//	
//	info.cbSize = sizeof(info);
//	info.fMask = SIF_PAGE;
//	
//	::GetScrollInfo(GetHandle(), SB_CTL, &info);
//	
//	info.nPage = inViewSize;
//	
//	::SetScrollInfo(GetHandle(), SB_CTL, &info, true);
//#endif
//}

MControlImpl* MControlImpl::CreateScrollbar(MControl* inControl)
{
	return new MScrollbarImpl(inControl);
}

