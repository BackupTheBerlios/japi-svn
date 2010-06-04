//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include "MWinWindowImpl.h"
#include "MWinControlsImpl.h"
#include "MWinUtils.h"

using namespace std;

const int kScrollbarWidth = 16;

template<class CONTROL>
MWinControlImpl<CONTROL>::MWinControlImpl(CONTROL* inControl, const string& inLabel)
	: CONTROL::MImpl(inControl)
	, mLabel(inLabel)
{
}

template<class CONTROL>
MWinControlImpl<CONTROL>::~MWinControlImpl()
{
}

template<class CONTROL>
string MWinControlImpl<CONTROL>::GetText() const
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

template<class CONTROL>
void MWinControlImpl<CONTROL>::SetText(const std::string& inText)
{
	mLabel = inText;

	wstring s(c2w(inText));
	::SetWindowTextW(GetHandle(), s.c_str());
}

template<class CONTROL>
void MWinControlImpl<CONTROL>::ActivateSelf()
{
	::EnableWindow(GetHandle(), mControl->IsActive() and mControl->IsEnabled());
}

template<class CONTROL>
void MWinControlImpl<CONTROL>::DeactivateSelf()
{
	if (::IsWindowEnabled(GetHandle()))
		::EnableWindow(GetHandle(), false);
}

template<class CONTROL>
void MWinControlImpl<CONTROL>::EnableSelf()
{
	::EnableWindow(GetHandle(), mControl->IsActive() and mControl->IsEnabled());
}

template<class CONTROL>
void MWinControlImpl<CONTROL>::DisableSelf()
{
	if (::IsWindowEnabled(GetHandle()))
		::EnableWindow(GetHandle(), false);
}

template<class CONTROL>
void MWinControlImpl<CONTROL>::ShowSelf()
{
	::ShowWindow(GetHandle(), SW_SHOW);
}

template<class CONTROL>
void MWinControlImpl<CONTROL>::HideSelf()
{
	::ShowWindow(GetHandle(), SW_HIDE);
}

template<class CONTROL>
void MWinControlImpl<CONTROL>::Draw(MRect inUpdate)
{
}

template<class CONTROL>
void MWinControlImpl<CONTROL>::FrameResized()
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

template<class CONTROL>
void MWinControlImpl<CONTROL>::GetParentAndBounds(MWinProcMixin*& outParent, MRect& outBounds)
{
	MView* view = mControl;
	MView* parent = view->GetParent();
	
	view->GetBounds(outBounds);
	
	while (parent != nil)
	{
		view->ConvertToParent(outBounds.x, outBounds.y);
		
		// for now we don't support embedding of controls in controls...

		//MControl* ctl = dynamic_cast<MControl*>(parent);
		//
		//if (ctl != nil)
		//{
		//	MWinControlImpl* impl =
		//		dynamic_cast<MWinControlImpl*>(ctl->GetImpl());
		//	
		//	if (impl != nil)
		//	{
		//		outParent = impl;
		//		break;
		//	}
		//}
		
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

template<class CONTROL>
void MWinControlImpl<CONTROL>::AddedToWindow()
{
	MWinProcMixin* parent;
	MRect bounds;

	GetParentAndBounds(parent, bounds);

	CreateHandle(parent, bounds, c2w(GetText()));

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

	RECT r;
	::GetClientRect(GetHandle(), &r);
	if (r.right - r.left != bounds.width or
		r.bottom - r.top != bounds.height)
	{
		::MapWindowPoints(GetHandle(), parent->GetHandle(), (LPPOINT)&r, 2);

		mControl->ResizeFrame(
			r.left - bounds.x, r.top - bounds.y,
			(r.right - r.left) - bounds.width,
			(r.bottom - r.top) - bounds.height);
	}
	
	if (mControl->IsVisible())
		ShowSelf();
	else
		HideSelf();
}

// --------------------------------------------------------------------

MWinButtonImpl::MWinButtonImpl(MButton* inButton, const string& inLabel)
	: MWinControlImpl(inButton, inLabel)
{
}

void MWinButtonImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);

	outStyle = WS_CHILD | BS_PUSHBUTTON | BS_TEXT | WS_TABSTOP;
	outClassName = L"BUTTON";
}

bool MWinButtonImpl::WMCommand(HWND inHWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool result = false;

	if (inMsg == BN_CLICKED)
	{
		mControl->eClicked();

		outResult = 1;
		result = true;
	}

	return result;
}

void MWinButtonImpl::SimulateClick()
{
	mControl->eClicked();
}

void MWinButtonImpl::MakeDefault(bool inDefault)
{
}

MButtonImpl* MButtonImpl::Create(MButton* inButton, const string& inLabel)
{
	return new MWinButtonImpl(inButton, inLabel);
}

// --------------------------------------------------------------------

MWinScrollbarImpl::MWinScrollbarImpl(MScrollbar* inScrollbar)
	: MWinControlImpl(inScrollbar, "")
{
}

void MWinScrollbarImpl::ShowSelf()
{
	::ShowScrollBar(GetHandle(), SB_CTL, TRUE);
}

void MWinScrollbarImpl::HideSelf()
{
	::ShowScrollBar(GetHandle(), SB_CTL, FALSE);
}

void MWinScrollbarImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
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

int32 MWinScrollbarImpl::GetValue() const
{
	SCROLLINFO info = { sizeof(SCROLLINFO), SIF_POS };
	
	if (GetHandle() != nil)
		::GetScrollInfo(GetHandle(), SB_CTL, &info);

	return info.nPos;
}

void MWinScrollbarImpl::SetValue(int32 inValue)
{
	if (GetHandle() != nil)
	{
		SCROLLINFO info = { sizeof(SCROLLINFO), SIF_POS };
		info.nPos = inValue;
		
		::SetScrollInfo(GetHandle(), SB_CTL, &info, true);
	}
}

int32 MWinScrollbarImpl::GetMinValue() const
{
	SCROLLINFO info = { sizeof(SCROLLINFO), SIF_RANGE };
	
	if (GetHandle() != nil)
		::GetScrollInfo(GetHandle(), SB_CTL, &info);

	return info.nMin;
}

void MWinScrollbarImpl::SetMinValue(int32 inValue)
{
	if (GetHandle() != nil)
	{
		SCROLLINFO info = { sizeof(SCROLLINFO), SIF_RANGE };
	
		::GetScrollInfo(GetHandle(), SB_CTL, &info);
		info.nMin = inValue;
		::SetScrollInfo(GetHandle(), SB_CTL, &info, true);
	}
}

int32 MWinScrollbarImpl::GetMaxValue() const
{
	SCROLLINFO info = { sizeof(SCROLLINFO), SIF_RANGE };
	if (GetHandle() != nil)
		::GetScrollInfo(GetHandle(), SB_CTL, &info);

	return info.nMax;
}

void MWinScrollbarImpl::SetMaxValue(int32 inValue)
{
	if (GetHandle() != nil)
	{
		SCROLLINFO info = { sizeof(SCROLLINFO), SIF_RANGE };
	
		::GetScrollInfo(GetHandle(), SB_CTL, &info);
		info.nMax = inValue;
		::SetScrollInfo(GetHandle(), SB_CTL, &info, true);
	}
}

bool MWinScrollbarImpl::WMScroll(HWND inHandle, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
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

void MWinScrollbarImpl::SetViewSize(int32 inViewSize)
{
	SCROLLINFO info = { sizeof(info), SIF_PAGE };
	
	::GetScrollInfo(GetHandle(), SB_CTL, &info);
	info.nPage = inViewSize;
	::SetScrollInfo(GetHandle(), SB_CTL, &info, true);
}

MScrollbarImpl* MScrollbarImpl::Create(MScrollbar* inScrollbar)
{
	return new MWinScrollbarImpl(inScrollbar);
}

// --------------------------------------------------------------------

MWinStatusbarImpl::MWinStatusbarImpl(MStatusbar* inStatusbar, uint32 inPartCount, int32 inPartWidths[])
	: MWinControlImpl(inStatusbar, "")
{
	int32 offset = 0;
	for (uint32 p = 0; p < inPartCount; ++p)
	{
		if (inPartWidths[p] > 0)
		{
			offset += inPartWidths[p];
			mOffsets.push_back(offset);
		}
		else
			mOffsets.push_back(-1);
	}
}

void MWinStatusbarImpl::CreateParams(
	DWORD& outStyle, DWORD& outExStyle, wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);

	outClassName = STATUSCLASSNAME;
	outStyle = WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP;
	outExStyle = 0;//WS_EX_CLIENTEDGE;
}

void MWinStatusbarImpl::AddedToWindow()
{
	MWinControlImpl::AddedToWindow();

	::SendMessageW(GetHandle(), SB_SETPARTS, mOffsets.size(), (LPARAM)&mOffsets[0]);
}

void MWinStatusbarImpl::SetStatusText(uint32 inPartNr, const string& inText, bool inBorder)
{
	if (inPartNr >= 0 and inPartNr < mOffsets.size())
	{
		if (inBorder == false)
			inPartNr |= SBT_NOBORDERS;
	
		wstring text(c2w(inText));
		::SendMessageW(GetHandle(), SB_SETTEXT, inPartNr, (LPARAM)text.c_str());
	}
}

MStatusbarImpl* MStatusbarImpl::Create(MStatusbar* inStatusbar, uint32 inPartCount, int32 inPartWidths[])
{
	return new MWinStatusbarImpl(inStatusbar, inPartCount, inPartWidths);
}

