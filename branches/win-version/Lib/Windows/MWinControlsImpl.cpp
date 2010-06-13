//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "MWinWindowImpl.h"
#include "MWinControlsImpl.h"
#include "MWinUtils.h"

using namespace std;

const int kScrollbarWidth = ::GetThemeSysSize(nil, SM_CXVSCROLL);

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
	
	SubClass();

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
		mControl->eClicked(mControl->GetID());

		outResult = 1;
		result = true;
	}

	return result;
}

void MWinButtonImpl::SimulateClick()
{
	mControl->eClicked(mControl->GetID());
}

void MWinButtonImpl::MakeDefault(bool inDefault)
{
	if (inDefault)
		::SendMessage(GetHandle(), BM_SETSTYLE, (WPARAM)BS_DEFPUSHBUTTON, 0);
	else
		::SendMessage(GetHandle(), BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, 0);
}

void MWinButtonImpl::GetIdealSize(int32& outWidth, int32& outHeight)
{
	outWidth = 75;
	outHeight = 23;

	SIZE size;
	if (GetHandle() != nil and Button_GetIdealSize(GetHandle(), &size))
	{
		if (outWidth < size.cx + 20)
			outWidth = size.cx + 20;

		if (outHeight < size.cy + 2)
			outHeight = size.cy + 2;
	}
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

// --------------------------------------------------------------------

MWinComboboxImpl::MWinComboboxImpl(MCombobox* inCombobox, bool inEditable)
	: MWinControlImpl(inCombobox, "")
	, mEditor(this)
	, mEditable(inEditable)
{
}

void MWinComboboxImpl::SetChoices(const std::vector<std::string>& inChoices)
{
	::SendMessage(GetHandle(), CB_RESETCONTENT, 0, 0);

	foreach (const string& choice, inChoices)
	{
		wstring s(c2w(choice));

		::SendMessage(GetHandle(), CB_INSERTSTRING, (WPARAM)-1, (long)s.c_str());
	}

	if (not inChoices.empty())
		SetText(inChoices.front());
}

void MWinComboboxImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);
	
	outClassName = L"COMBOBOX";
	if (mEditable)
		outStyle = WS_CHILD | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP;
	else
		outStyle = WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP;
}

void MWinComboboxImpl::AddedToWindow()
{
	MWinControlImpl::AddedToWindow();
	
	HWND edit = ::GetWindow(GetHandle(), GW_CHILD);
	if (edit != nil)
	{
		mEditor.SetHandle(edit);
		mEditor.SubClass();
		//mEditor.AddMessageHandler(WM_MOUSEWHEEL, this, &HWinComboBoxImp::WMMouseWheel);
		//mEditor.AddMessageHandler(kControlMsgBase + EN_SETFOCUS,
		//	static_cast<HNativeControlNodeImp*>(this), &HNativeControlNodeImp::WMSetFocus);
	}

	AddHandler(WM_MOUSEWHEEL, boost::bind(&MWinComboboxImpl::WMMouseWheel, this, _1, _2, _3, _4, _5));
}

bool MWinComboboxImpl::DispatchKeyDown(uint32 inKeyCode, uint32 inModifiers,
						const std::string& inText)
{
	bool result = false;

	if (inKeyCode == kReturnKeyCode or
		inKeyCode == kEnterKeyCode or
		inKeyCode == kTabKeyCode or
		inKeyCode == kEscapeKeyCode or
		(inModifiers & ~kShiftKey) != 0)
	{
		result = MWinControlImpl::DispatchKeyDown(inKeyCode, inModifiers, inText);
	}

	return result;
}

bool MWinComboboxImpl::WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool result = true;
	
	switch (inUMsg)
	{
		case CBN_SELENDOK:
			mControl->eValueChanged(mControl->GetID(), "Nieuw!");
			break;
		
		case CBN_EDITUPDATE:
			mControl->eValueChanged(mControl->GetID(), "Nieuw!");
			break;
		
		case CBN_DROPDOWN:
		{
			int count = ::SendMessage(GetHandle(), CB_GETCOUNT, 0, 0) + 1;
			if (count < 1) count = 1;
			if (count > 8) count = 8;
			
			MRect bounds;
			mControl->GetBounds(bounds);
			
			int itemHeight = ::SendMessage(GetHandle(), CB_GETITEMHEIGHT, 0, 0);
			::SetWindowPos(GetHandle(), 0, 0, 0, bounds.width,
				count * itemHeight + bounds.height + 2,
				SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_HIDEWINDOW);
			::SetWindowPos(GetHandle(), 0, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_SHOWWINDOW);
			break;
		}
		
		default:
			result = false;
			break;
	}
	
	return result;
}

bool MWinComboboxImpl::WMMouseWheel(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult)
{
	return false;
}

MComboboxImpl* MComboboxImpl::Create(MCombobox* inCombobox, bool inEditable)
{
	return new MWinComboboxImpl(inCombobox, inEditable);
}

// --------------------------------------------------------------------

MWinCaptionImpl::MWinCaptionImpl(MCaption* inControl, const string& inText)
	: MWinControlImpl(inControl, inText)
{
}

void MWinCaptionImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);
	
	outClassName = L"STATIC";
	outStyle = WS_CHILD;
}

MCaptionImpl* MCaptionImpl::Create(MCaption* inCaption, const std::string& inText)
{
	return new MWinCaptionImpl(inCaption, inText);
}

// --------------------------------------------------------------------

MWinSeparatorImpl::MWinSeparatorImpl(MSeparator* inControl)
	: MWinControlImpl(inControl, "")
{
}

void MWinSeparatorImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);
	
	outClassName = L"STATIC";
	outStyle = WS_CHILD | SS_SUNKEN;
}

MSeparatorImpl* MSeparatorImpl::Create(MSeparator* inSeparator)
{
	return new MWinSeparatorImpl(inSeparator);
}

// --------------------------------------------------------------------

MWinCheckboxImpl::MWinCheckboxImpl(MCheckbox* inControl, const string& inText)
	: MWinControlImpl(inControl, inText)
{
}

void MWinCheckboxImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);
	
	outClassName = L"BUTTON";
	outStyle = WS_CHILD | BS_CHECKBOX | BS_TEXT | WS_TABSTOP;
}

bool MWinCheckboxImpl::IsChecked() const
{
	return ::SendMessage(GetHandle(), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void MWinCheckboxImpl::SetChecked(bool inChecked)
{
	if (inChecked)
		::SendMessage(GetHandle(), BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	else
		::SendMessage(GetHandle(), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
}

bool MWinCheckboxImpl::WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool result = false;

	if (inUMsg == BN_CLICKED)
	{
		bool checked = not IsChecked();

		SetChecked(checked);
		mControl->eValueChanged(mControl->GetID(), checked);

		outResult = 1;
		result = true;
	}

	return result;
}

MCheckboxImpl* MCheckboxImpl::Create(MCheckbox* inCheckbox, const std::string& inText)
{
	return new MWinCheckboxImpl(inCheckbox, inText);
}

