//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include <boost/algorithm/string.hpp>

#include "MClipboardImpl.h"
#include "MWinProcMixin.h"
#include "MWinUtils.h"

using namespace std;
namespace ba = boost::algorithm;

class MWinClipboardImpl : public MClipboardImpl, public MWinProcMixin
{
public:
					MWinClipboardImpl(MClipboard* inClipboard);

	virtual void	LoadClipboardIfNeeded();
	virtual void	Reset();
	virtual void	Commit();
	
	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);

	virtual bool	WMRenderClipboard(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult);

	unsigned long	mSequenceNumber;
};

MWinClipboardImpl::MWinClipboardImpl(MClipboard* inClipboard)
	: MClipboardImpl(inClipboard)
	, mSequenceNumber(0)
{
	MWinProcMixin::CreateHandle(nil, MRect(0, 0, 0, 0), L"MWinClipboardImpl");

	AddHandler(WM_RENDERFORMAT,	boost::bind(&MWinClipboardImpl::WMRenderClipboard, this, _1, _2, _3, _4, _5));
//	AddMessageHandler(WM_RENDERALLFORMATS,
//										boost::bind(&MWinClipboardImpl::WMRenderClipboard, this, _1, _2, _3, _4, _5));
}

void MWinClipboardImpl::CreateParams(
	DWORD& outStyle, DWORD& outExStyle, wstring& outClassName, HMENU& outMenu)
{
	MWinProcMixin::CreateParams(outStyle, outExStyle, outClassName, outMenu);
	
	outStyle = WS_OVERLAPPED;
	outClassName = L"MWinClipboardImpl";
}

bool MWinClipboardImpl::WMRenderClipboard(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	if (not mClipboard->HasData())
		return false;
	
	uint32 format = inWParam;
	if (format == 0)
		format = CF_UNICODETEXT;
	
	try
	{
		string text;
		bool block;

		mClipboard->GetData(text, block);
		ba::replace_all(text, "\n", "\r\n");
		
		if (format == CF_UNICODETEXT)
		{
			wstring wtext = c2w(text);
			
			HANDLE hMem = ::GlobalAlloc(GHND, (wtext.length() + 1) * sizeof(wchar_t));
			wchar_t* data = reinterpret_cast<wchar_t*>(::GlobalLock(hMem));
			
			copy(wtext.begin(), wtext.end(), data);
			data[wtext.length()] = 0;
			
			::GlobalUnlock(hMem);
			::SetClipboardData(CF_UNICODETEXT, hMem);
		}
	}
	catch (...) {}

	return true;
}

void MWinClipboardImpl::LoadClipboardIfNeeded()
{
	if (::GetClipboardSequenceNumber() != mSequenceNumber)
	{
		::OpenClipboard(GetHandle());
	
		HANDLE hMem = ::GetClipboardData(CF_UNICODETEXT);
		if (hMem != nil)
		{
			wchar_t* data = reinterpret_cast<wchar_t*>(::GlobalLock(hMem));
			
			try
			{
				wstring text(data);
				ba::replace_all(text, L"\r\n", L"\n");
				mClipboard->SetData(w2c(text), false);
			
				::GlobalUnlock(hMem);
			}
			catch (...) {}
		}
		
		::CloseClipboard();
		mSequenceNumber = ::GetClipboardSequenceNumber();
	}
}

void MWinClipboardImpl::Reset()
{
	::OpenClipboard(GetHandle());
	::EmptyClipboard();
	::CloseClipboard();

	mSequenceNumber = ::GetClipboardSequenceNumber();
}

void MWinClipboardImpl::Commit()
{
	::OpenClipboard(GetHandle());
	::EmptyClipboard();
	::SetClipboardData(CF_UNICODETEXT, NULL);
	::CloseClipboard();

	mSequenceNumber = ::GetClipboardSequenceNumber();
}

MClipboardImpl* MClipboardImpl::Create(MClipboard* inClipboard)
{
	return new MWinClipboardImpl(inClipboard);
}
