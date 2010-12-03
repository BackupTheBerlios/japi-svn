//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/algorithm/string.hpp>

#include "MWinWindowImpl.h"
#include "MWinControlsImpl.h"
#include "MWinUtils.h"
#include "MUtils.h"

using namespace std;
namespace ba = boost::algorithm;

const int kScrollbarWidth = ::GetThemeSysSize(nil, SM_CXVSCROLL);

template<class CONTROL>
MWinControlImpl<CONTROL>::MWinControlImpl(CONTROL* inControl, const string& inLabel)
	: CONTROL::MImpl(inControl)
	, mLabel(inLabel)
{
//	AddHandler(WM_GETDLGCODE, boost::bind(&MWinControlImpl::WMGetDlgCode, this, _1, _2, _3, _4, _5));
	AddHandler(WM_SETFOCUS, boost::bind(&MWinControlImpl::WMSetFocus, this, _1, _2, _3, _4, _5));
}

template<class CONTROL>
MWinControlImpl<CONTROL>::~MWinControlImpl()
{
}

template<class CONTROL>
void MWinControlImpl<CONTROL>::Focus()
{
	if (GetHandle() != nil)
	{
		::SetFocus(GetHandle());
		::UpdateWindow(GetHandle());
	}
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
		
		MControlBase* ctl = dynamic_cast<MControlBase*>(parent);
		
		if (ctl != nil)
		{
			MControlImplBase* impl = ctl->GetImplBase();

			if (impl != nil and impl->GetWinProcMixin() != nil)
			{
				outParent = impl->GetWinProcMixin();
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

template<class CONTROL>
bool MWinControlImpl<CONTROL>::WMGetDlgCode(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	return false;
}

template<class CONTROL>
bool MWinControlImpl<CONTROL>::WMSetFocus(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	MWindow* w = mControl->GetWindow();
	if (w != nil)
		w->SetFocus(mControl);
	
	return false;
}

template<class CONTROL>
MWinProcMixin* MWinControlImpl<CONTROL>::GetWinProcMixin()
{
	MWinProcMixin* mixin = this;
	return mixin;
}

// --------------------------------------------------------------------

MWinButtonImpl::MWinButtonImpl(MButton* inButton, const string& inLabel)
	: MWinControlImpl(inButton, inLabel)
	, mDefault(false)
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

bool MWinButtonImpl::WMGetDlgCode(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	outResult = DLGC_BUTTON;

	if (mDefault)
		outResult |= DLGC_DEFPUSHBUTTON;

	return true;
}

void MWinButtonImpl::SimulateClick()
{
	::SendMessage(GetHandle(), BM_SETSTATE, 1, 0);
	::UpdateWindow(GetHandle());
	::delay(12.0 / 60.0);
	::SendMessage(GetHandle(), BM_SETSTATE, 0, 0);
	::UpdateWindow(GetHandle());

	mControl->eClicked(mControl->GetID());
}

void MWinButtonImpl::MakeDefault(bool inDefault)
{
	mDefault = inDefault;

	if (GetHandle() != nil)
	{
		if (inDefault)
			::SendMessage(GetHandle(), BM_SETSTYLE, (WPARAM)BS_DEFPUSHBUTTON, 0);
		else
			::SendMessage(GetHandle(), BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, 0);
		::UpdateWindow(GetHandle());
	}
}

void MWinButtonImpl::AddedToWindow()
{
	MWinControlImpl::AddedToWindow();
	if (mDefault)
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

int32 MWinScrollbarImpl::GetTrackValue() const
{
	SCROLLINFO info = { sizeof(SCROLLINFO), SIF_TRACKPOS };
	
	if (GetHandle() != nil)
		::GetScrollInfo(GetHandle(), SB_CTL, &info);

	return info.nTrackPos;
}

void MWinScrollbarImpl::SetAdjustmentValues(int32 inMinValue, int32 inMaxValue,
	int32 inScrollUnit, int32 inPageSize, int32 inValue)
{
	if (GetHandle() != nil)
	{
		SCROLLINFO info = { sizeof(SCROLLINFO), SIF_ALL };
		info.nMin = inMinValue;
		info.nMax = inMaxValue;
		info.nPage = inPageSize;
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

int32 MWinScrollbarImpl::GetMaxValue() const
{
	SCROLLINFO info = { sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE };
	if (GetHandle() != nil)
		::GetScrollInfo(GetHandle(), SB_CTL, &info);

	int32 result = info.nMax;
	if (info.nPage > 1)
		result -= info.nPage - 1;

	return result;
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
//		mControl->GetWindow()->UpdateNow();
	}

	return result;
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
		::UpdateWindow(GetHandle());
	}
}

MStatusbarImpl* MStatusbarImpl::Create(MStatusbar* inStatusbar, uint32 inPartCount, int32 inPartWidths[])
{
	return new MWinStatusbarImpl(inStatusbar, inPartCount, inPartWidths);
}

// --------------------------------------------------------------------

MWinComboboxImpl::MWinComboboxImpl(MCombobox* inCombobox)
	: MWinControlImpl(inCombobox, "")
	, mEditor(this)
{
}

void MWinComboboxImpl::SetChoices(const std::vector<std::string>& inChoices)
{
	if (GetHandle() == nil)
		mChoices = inChoices;
	else
	{
		::SendMessage(GetHandle(), CB_RESETCONTENT, 0, 0);

		foreach (const string& choice, inChoices)
		{
			wstring s(c2w(choice));

			::SendMessage(GetHandle(), CB_INSERTSTRING, (WPARAM)-1, (long)s.c_str());
		}

		if (not inChoices.empty())
			SetText(inChoices.front());
		
		::UpdateWindow(GetHandle());
	}
}

void MWinComboboxImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);
	
	outClassName = L"COMBOBOX";
	outStyle = WS_CHILD | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP;
}

void MWinComboboxImpl::AddedToWindow()
{
	MWinControlImpl::AddedToWindow();
	
	HWND edit = ::GetWindow(GetHandle(), GW_CHILD);
	THROW_IF_NIL(edit);

	mEditor.SetHandle(edit);
	mEditor.SubClass();
	//mEditor.AddMessageHandler(WM_MOUSEWHEEL, this, &HWinComboBoxImp::WMMouseWheel);
	//mEditor.AddMessageHandler(kControlMsgBase + EN_SETFOCUS,
	//	static_cast<HNativeControlNodeImp*>(this), &HNativeControlNodeImp::WMSetFocus);

	AddHandler(WM_MOUSEWHEEL, boost::bind(&MWinComboboxImpl::WMMouseWheel, this, _1, _2, _3, _4, _5));

	if (not mChoices.empty())
		SetChoices(mChoices);
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
			mControl->eValueChanged(mControl->GetID(), GetText());
			break;
		
		case CBN_EDITUPDATE:
			mControl->eValueChanged(mControl->GetID(), GetText());
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

MComboboxImpl* MComboboxImpl::Create(MCombobox* inCombobox)
{
	return new MWinComboboxImpl(inCombobox);
}

// --------------------------------------------------------------------

MWinPopupImpl::MWinPopupImpl(MPopup* inPopup)
	: MWinControlImpl(inPopup, "")
{
}

void MWinPopupImpl::SetChoices(const std::vector<std::string>& inChoices)
{
	if (GetHandle() == nil)
		mChoices = inChoices;
	else
	{
		::SendMessage(GetHandle(), CB_RESETCONTENT, 0, 0);

		foreach (const string& choice, inChoices)
		{
			wstring s(c2w(choice));

			::SendMessage(GetHandle(), CB_INSERTSTRING, (WPARAM)-1, (long)s.c_str());
		}

		if (not inChoices.empty())
			SetValue(0);
		
		::UpdateWindow(GetHandle());
	}
}

void MWinPopupImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);
	
	outClassName = L"COMBOBOX";
	outStyle = WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP;
}

void MWinPopupImpl::AddedToWindow()
{
	MWinControlImpl::AddedToWindow();
	
	AddHandler(WM_MOUSEWHEEL, boost::bind(&MWinPopupImpl::WMMouseWheel, this, _1, _2, _3, _4, _5));

	if (not mChoices.empty())
		SetChoices(mChoices);
}

int32 MWinPopupImpl::GetValue() const
{
	return ::SendMessage(GetHandle(), CB_GETCURSEL, 0, 0) + 1;
}

void MWinPopupImpl::SetValue(int32 inValue)
{
	::SendMessage(GetHandle(), CB_SETCURSEL, (WPARAM)(inValue - 1), 0);
	::UpdateWindow(GetHandle());
}

bool MWinPopupImpl::DispatchKeyDown(uint32 inKeyCode, uint32 inModifiers,
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

bool MWinPopupImpl::WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool result = true;
	
	switch (inUMsg)
	{
		case CBN_SELENDOK:
			mControl->eValueChanged(mControl->GetID(), GetValue());
			break;
		
//		case CBN_EDITUPDATE:
//			mControl->eValueChanged(mControl->GetID(), GetValue());
//			break;
		
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

bool MWinPopupImpl::WMMouseWheel(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult)
{
	return false;
}

MPopupImpl* MPopupImpl::Create(MPopup* inPopup)
{
	return new MWinPopupImpl(inPopup);
}

// --------------------------------------------------------------------

MWinEdittextImpl::MWinEdittextImpl(MEdittext* inEdittext)
	: MWinControlImpl(inEdittext, "")
{
}

void MWinEdittextImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);

	outClassName = L"EDIT";
	outStyle = WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL;
	outExStyle = WS_EX_CLIENTEDGE;

//	if (fMultiLine)
//		ioParams.fStyle |= ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL;
//	else if (fNumber)
//		ioParams.fStyle |= ES_NUMBER | ES_RIGHT;
//	else if (fPassword)
//		ioParams.fStyle |= ES_PASSWORD;
	
	AddHandler(WM_SETFOCUS, boost::bind(&MWinEdittextImpl::WMSetFocus, this, _1, _2, _3, _4, _5));
}

string MWinEdittextImpl::GetText() const
{
	int l = ::SendMessage(GetHandle(), WM_GETTEXTLENGTH, 0, 0);
	vector<wchar_t> buffer(l + 1);
	l = ::SendMessage(GetHandle(), WM_GETTEXT, (WPARAM)(l + 1), (LPARAM)&buffer[0]);

	string text(w2c(&buffer[0]));
	ba::replace_all(text, "\r\n", "\n");
	return text;
}

void MWinEdittextImpl::SetText(const std::string& inText)
{
	wstring text(c2w(inText));
	ba::replace_all(text, L"\n", L"\r\n");
	::SendMessage(GetHandle(), WM_SETTEXT, 0, (LPARAM)text.c_str());
	::UpdateWindow(GetHandle());
}

void MWinEdittextImpl::SetPasswordChar(uint32 inUnicode)
{
	::SendMessage(GetHandle(), EM_SETPASSWORDCHAR, (WPARAM)inUnicode, 0);
}

bool MWinEdittextImpl::DispatchKeyDown(uint32 inKeyCode, uint32 inModifiers,
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

bool MWinEdittextImpl::WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool result = true;
	
	switch (inUMsg)
	{
		case EN_CHANGE:
			mControl->eValueChanged(mControl->GetID(), GetText());
			break;
		
		default:
			result = false;
			break;
	}
	
	return result;
}

bool MWinEdittextImpl::WMSetFocus(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	::SendMessage(GetHandle(), EM_SETSEL, 0, -1);
	return false;
}

MEdittextImpl* MEdittextImpl::Create(MEdittext* inEdittext)
{
	return new MWinEdittextImpl(inEdittext);
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

void MWinCaptionImpl::SetText(const string& inText)
{
	wstring s(c2w(inText));
	::SetWindowTextW(GetHandle(), s.c_str());
	::UpdateWindow(GetHandle());
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
	::UpdateWindow(GetHandle());
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

// --------------------------------------------------------------------

MWinListHeaderImpl::MWinListHeaderImpl(MListHeader* inListHeader)
	: MWinControlImpl(inListHeader, "")
{
}

void MWinListHeaderImpl::CreateHandle(MWinProcMixin* inParent, MRect inBounds,
	const wstring& inTitle)
{
	MWinControlImpl::CreateHandle(inParent, inBounds, inTitle);

	if (inParent != nil)
	{
        // Retrieve the bounding rectangle of the parent window's 
        // client area, and then request size and position values 
        // from the header control. 

//		MRect bounds;
//		mControl->GetBounds(bounds);
//		mControl->ConvertToWindow(bounds.x, bounds.y);

		RECT rc = { inBounds.x, inBounds.y, inBounds.x + inBounds.width, inBounds.y + inBounds.height };
 
        HDLAYOUT hdl;
        WINDOWPOS wp; 

		hdl.prc = &rc;
        hdl.pwpos = &wp; 

		if (::SendMessage(GetHandle(), HDM_LAYOUT, 0, (LPARAM) &hdl))
		{
			// Set the size, position, and visibility of the header control.
			::SetWindowPos(GetHandle(), wp.hwndInsertAfter, wp.x, wp.y, 
				wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW);
			
			MRect frame;
			mControl->GetFrame(frame);
			frame.width = wp.cx;
			frame.height = wp.cy;
			mControl->SetFrame(frame);
		}

		inParent->AddNotify(HDN_TRACK, GetHandle(),
			boost::bind(&MWinListHeaderImpl::HDNTrack, this, _1, _2, _3));
		inParent->AddNotify(HDN_BEGINTRACK, GetHandle(),
			boost::bind(&MWinListHeaderImpl::HDNBeginTrack, this, _1, _2, _3));
		inParent->AddNotify(HDN_BEGINDRAG, GetHandle(),
			boost::bind(&MWinListHeaderImpl::HDNBeginDrag, this, _1, _2, _3));
	}
}

void MWinListHeaderImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);

	outStyle = WS_CHILD | HDS_HORZ;
	outClassName = WC_HEADER;
}

void MWinListHeaderImpl::AppendColumn(const string& inLabel, int inWidth)
{
	HDITEM item = {};
	
	wstring label = c2w(inLabel);
	
	item.mask = HDI_FORMAT | HDI_TEXT;
	item.pszText = const_cast<wchar_t*>(label.c_str());
	item.cchTextMax = label.length();
	item.fmt = HDF_LEFT | HDF_STRING;

	if (inWidth > 0)
	{
		item.mask |= HDI_WIDTH;
		item.cxy = inWidth;
	}
	
	int insertAfter = ::SendMessage(GetHandle(), HDM_GETITEMCOUNT, 0, 0);
	::SendMessage(GetHandle(), HDM_INSERTITEM, (WPARAM)&insertAfter, (LPARAM)&item);
}

bool MWinListHeaderImpl::HDNBeginTrack(WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	outResult = 1;
	return true;
}

bool MWinListHeaderImpl::HDNBeginDrag(WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	outResult = 1;
	return true;
}

bool MWinListHeaderImpl::HDNTrack(WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	NMHEADER& nmh = *(NMHEADER*)inLParam;

	mControl->eColumnResized(nmh.iItem, nmh.pitem->cxy);

	return true;
}

MListHeaderImpl* MListHeaderImpl::Create(MListHeader* inListHeader)
{
	return new MWinListHeaderImpl(inListHeader);
}

// --------------------------------------------------------------------

MWinNotebookImpl::MWinNotebookImpl(MNotebook* inControl)
	: MWinControlImpl(inControl, "")
{
}

void MWinNotebookImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);
	
	outClassName = WC_TABCONTROL;
	outStyle = WS_CHILD | /*BS_Notebook | BS_TEXT | */WS_TABSTOP;
}

void MWinNotebookImpl::AddedToWindow()
{
	MWinControlImpl::AddedToWindow();

	MRect bounds;
	MWinProcMixin* parent;

	GetParentAndBounds(parent, bounds);
	
	parent->AddNotify(TCN_SELCHANGE, GetHandle(),
		boost::bind(&MWinNotebookImpl::TCNSelChange, this, _1, _2, _3));
}

void MWinNotebookImpl::FrameResized()
{
	MWinControlImpl<MNotebook>::FrameResized();

	MRect bounds;
	MWinProcMixin* parent;

	GetParentAndBounds(parent, bounds);

	RECT rc = { 0, 0, bounds.width, bounds.height };
	TabCtrl_AdjustRect(GetHandle(), false, &rc);
	
	foreach (MView* page, mPages)
	{
		MRect frame;
		page->GetFrame(frame);

		int32 dx, dy, dw, dh;
		dx = rc.left - frame.x;
		dy = rc.top - frame.y;
		dw = rc.right - rc.left - frame.width;
		dh = rc.bottom - rc.top - frame.height;

		page->ResizeFrame(dx, dy, dw, dh);
	}
	
//	
//
// HDWP hdwp; 
//                RECT rc; 
//
//                // Calculate the display rectangle, assuming the 
//                // tab control is the size of the client area. 
//                SetRect(&rc, 0, 0, 
//                        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); 
//                TabCtrl_AdjustRect(g_hwndTab, FALSE, &rc); 
//
//                // Size the tab control to fit the client area. 
//                hdwp = BeginDeferWindowPos(2); 
//                DeferWindowPos(hdwp, g_hwndTab, NULL, 0, 0, 
//                    GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 
//                    SWP_NOMOVE | SWP_NOZORDER 
//                    ); 
//
//                // Position and size the static control to fit the 
//                // tab control's display area, and make sure the 
//                // static control is in front of the tab control. 
//                DeferWindowPos(hdwp, 
//                    g_hwndDisplay, HWND_TOP, rc.left, rc.top, 
//                    rc.right - rc.left, rc.bottom - rc.top, 0 
//                    ); 
//                EndDeferWindowPos(hdwp);
}

void MWinNotebookImpl::AddPage(const string& inLabel, MView* inPage)
{
	wstring s(c2w(inLabel));
	
	TCITEM tci = {};
	tci.mask = TCIF_TEXT;
	tci.pszText = const_cast<wchar_t*>(s.c_str());
	
	::SendMessage(GetHandle(), TCM_INSERTITEM, (WPARAM)mPages.size(), (LPARAM)&tci);
	
	mControl->AddChild(inPage);
	mPages.push_back(inPage);
	
	if (mPages.size() > 1)
		inPage->Hide();
}

void MWinNotebookImpl::SelectPage(uint32 inPage)
{
	if (inPage != ::SendMessage(GetHandle(), TCM_GETCURSEL, 0, 0))
	{
		::SendMessage(GetHandle(), TCM_SETCURSEL, (WPARAM)inPage, 0);
		::UpdateWindow(GetHandle());
	}
	
	if (inPage < mPages.size())
	{
		for_each(mPages.begin(), mPages.end(), boost::bind(&MView::Hide, _1));
		for_each(mPages.begin(), mPages.end(), boost::bind(&MView::Disable, _1));
		mPages[inPage]->Show();
		mPages[inPage]->Enable();
		mControl->ePageSelected(inPage);
	}
}

uint32 MWinNotebookImpl::GetSelectedPage() const
{
	return ::SendMessage(GetHandle(), TCM_GETCURSEL, 0, 0);
}

bool MWinNotebookImpl::TCNSelChange(WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	SelectPage(::SendMessage(GetHandle(), TCM_GETCURSEL, 0, 0));


	outResult = 0;
	return true;
}

MNotebookImpl* MNotebookImpl::Create(MNotebook* inNotebook)
{
	return new MWinNotebookImpl(inNotebook);
}

