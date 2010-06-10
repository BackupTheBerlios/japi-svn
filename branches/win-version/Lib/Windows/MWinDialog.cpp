//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include "zeep/xml/document.hpp"
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/algorithm/string.hpp>

#include "MWinWindowImpl.h"
#include "MWindow.h"
#include "MError.h"
#include "MWinApplicationImpl.h"
#include "MWinControlsImpl.h"
#include "MUtils.h"
#include "MWinUtils.h"
#include "MWinMenu.h"
#include "MDevice.h"
#include "MResources.h"
#include "MAcceleratorTable.h"
#include "MControls.h"

using namespace std;
using namespace zeep;
namespace io = boost::iostreams;
namespace ba = boost::algorithm;

class MWinDialogImpl : public MWinWindowImpl
{
  public:
					MWinDialogImpl(const string& inResource, MWindow* inWindow);
	virtual			~MWinDialogImpl();

	virtual void	Finish();

private:

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle, wstring& outClassName, HMENU& outMenu);
	virtual void	RegisterParams(UINT& outStyle, int& outWndExtra, HCURSOR& outCursor, HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground);

	MView*			CreateControls(MView* inParent, int32 inX, int32 inY,
						xml::element* inTemplate);

	void			GetTextMetrics(const string& inText, const wchar_t* inClass,
						int inPartID, int inStateID, int32& outWidth, int32& outHeight);

	string			mRsrc;
	HDC				mDC;
};

MWinDialogImpl::MWinDialogImpl(const string& inResource, MWindow* inWindow)
	: MWinWindowImpl(MWindowFlags(0), "", inWindow)
	, mRsrc(inResource)
	, mDC(nil)
{
}

MWinDialogImpl::~MWinDialogImpl()
{
}

void MWinDialogImpl::CreateParams(DWORD& outStyle,
	DWORD& outExStyle, wstring& outClassName, HMENU& outMenu)
{
	MWinWindowImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);

	outClassName = L"MWinDialogImpl";
	outStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	outExStyle = WS_EX_CONTROLPARENT;
	outMenu = nil;
}

void MWinDialogImpl::RegisterParams(UINT& outStyle, int& outWndExtra, HCURSOR& outCursor,
	HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground)
{
	MWinWindowImpl::RegisterParams(outStyle, outWndExtra,
		outCursor, outIcon, outSmallIcon, outBackground);
	
	HINSTANCE inst = MWinApplicationImpl::GetInstance()->GetHInstance();
	
	outStyle = 0;// CS_HREDRAW | CS_VREDRAW;
	outWndExtra = DLGWINDOWEXTRA;
	//outIcon = ::LoadIcon(inst, MAKEINTRESOURCE(ID_DEF_DOC_ICON));
	//outSmallIcon = ::LoadIcon(inst, MAKEINTRESOURCE(ID_DEF_DOC_ICON));
	outCursor = ::LoadCursor(NULL, IDC_ARROW);
	outBackground = (HBRUSH)(COLOR_BTNFACE + 1);
}

void MWinDialogImpl::Finish()
{
	mFlags = kMFixedSize;

	mrsrc::rsrc rsrc(string("Dialogs/") + mRsrc + ".xml");
		
	if (not rsrc)
		THROW(("Dialog resource not found: %s", mRsrc.c_str()));

	io::stream<io::array_source> data(rsrc.data(), rsrc.size());
	xml::document doc(data);

	xml::element* dialog = doc.find_first("/dialog");
	if (dialog == nil)
		THROW(("Invalid dialog resource"));
	
	wstring title = c2w(dialog->get_attribute("title"));

	uint32 minWidth = 100;
	if (not dialog->get_attribute("width").empty())
		minWidth = boost::lexical_cast<uint32>(dialog->get_attribute("width"));
	uint32 minHeight = 100;
	if (not dialog->get_attribute("height").empty())
		minHeight = boost::lexical_cast<uint32>(dialog->get_attribute("height"));

	MRect bounds(CW_USEDEFAULT, CW_USEDEFAULT, minWidth, minHeight);

	// now create the dialog
	MWinWindowImpl::Create(bounds, title);

	// now we have the handle, get the DC and theme
	mDC = ::GetDC(GetHandle());

	// create the dialog controls, all stacked on top of each other
	CreateControls(mWindow, 0, 0, dialog);

	RECT cr;
	::GetClientRect(GetHandle(), &cr);

	mWindow->GetBounds(bounds);

	int32 dw = bounds.width - (cr.right - cr.left);		if (dw < 0) dw = 0;
	int32 dh = bounds.height - (cr.bottom - cr.top);	if (dh < 0) dh = 0;

	if (dw > 0 or dh > 0)
	{
		MRect p;
		mWindow->GetWindowPosition(p);
		p.x -= dw / 2;
		p.y -= dh / 2;
		p.width += dw;
		p.height += dh;
		mWindow->SetWindowPosition(p);
	}
}

MView* MWinDialogImpl::CreateControls(MView* inParent, int32 inX, int32 inY,
	xml::element* inTemplate)
{
	MView* result = nil;

	string name = inTemplate->name();

	if (inTemplate->name() == "button")
	{
		string id = inTemplate->get_attribute("id");
		string title = inTemplate->get_attribute("title");
		
		MRect bounds(inX, inY, 75, 23);
		MButton* button = new MButton(id, bounds, title);
		//mControls[id] = button;
		inParent->AddChild(button);
		//button->GetIdealSize(bounds.width, bounds.height);

		GetTextMetrics(title, L"PushButton;Button", BP_PUSHBUTTON, PBS_NORMAL, bounds.width, bounds.height);
		bounds.width += 20;
		if (bounds.width < 75)
			bounds.width = 75;

		bounds.height += 4;
		if (bounds.height < 23)
			bounds.height = 23;

		button->ResizeFrame(0, 0, bounds.width - 75, bounds.height - 23);

		result = button;
	}
	else if (inTemplate->name() == "combobox")
	{
		string id = inTemplate->get_attribute("id");
		
		MRect bounds(inX, inY, 75, 23);
		MCombobox* combo = new MCombobox(id, bounds, true);
		inParent->AddChild(combo);
		result = combo;
	}
	else if (inTemplate->name() == "caption")
	{
		string text = inTemplate->get_attribute("text");

		MRect bounds(inX, inY, 0, 0);
		GetTextMetrics(text, L"ContentArea;BodyText;Caption", TEXT_LABEL, 0, bounds.width, bounds.height);
//		bounds.width += 20;
//		if (bounds.width < 75)
//			bounds.width = 75;

		bounds.height += 4;
		if (bounds.height < 23)
			bounds.height = 23;
		
		MCaption* caption = new MCaption("caption", bounds, text);
		inParent->AddChild(caption);
		result = caption;
	}
	else if (name == "vbox" or name == "dialog")
	{
		int32 marginX = 0;
		int32 marginY = 0;

		if (not inTemplate->get_attribute("margin-x").empty())
			marginX = boost::lexical_cast<int32>(inTemplate->get_attribute("margin-x"));

		if (not inTemplate->get_attribute("margin-y").empty())
			marginY = boost::lexical_cast<int32>(inTemplate->get_attribute("margin-y"));
		
		int32 y = marginY;
		int32 width = 2 * marginX;
		
		if (name == "dialog")
			result = mWindow;
		else
		{
			result = new MView("vbox", MRect(inX, inY, 0, 0));
			inParent->AddChild(result);
		}
		
		vector<MView*> views;
		
		foreach (xml::element* b, inTemplate->children<xml::element>())
		{
			MView* v = CreateControls(result, marginX, y, b);
			views.push_back(v);
			
			MRect r;
			v->GetBounds(r);
			
			if (width < r.width + 2 * marginX)
				width = r.width + 2 * marginX;
			
			y += r.height + marginY;
		}
		
		int32 height = y;
		
		foreach (MView* v, views)
		{
			MRect b;
			v->GetBounds(b);
			if (b.width < width - 2 * marginX)
				v->ResizeFrame(0, 0, width - b.width - 2 * marginX, 0);
		}
		
		result->SetFrame(MRect(inX, inY, width, height));
	}
	else if (name == "hbox")
	{
		int32 marginX = 0;
		int32 marginY = 0;

		if (not inTemplate->get_attribute("margin-x").empty())
			marginX = boost::lexical_cast<int32>(inTemplate->get_attribute("margin-x"));

		if (not inTemplate->get_attribute("margin-y").empty())
			marginY = boost::lexical_cast<int32>(inTemplate->get_attribute("margin-y"));
		
		int32 x = marginX;
		int32 height = 2 * marginY;
		
		result = new MView("hbox", MRect(inX, inY, 0, 0));
		inParent->AddChild(result);
		
		vector<MView*> views;
		
		foreach (xml::element* b, inTemplate->children<xml::element>())
		{
			MView* v = CreateControls(result, x, marginY, b);
			views.push_back(v);
			
			MRect r;
			v->GetBounds(r);
			
			if (height < r.height + 2 * marginY)
				height = r.height + 2 * marginY;
			
			x += r.width + marginX;
		}
		
		int32 width = x;
		
		foreach (MView* v, views)
		{
			MRect b;
			v->GetBounds(b);
			if (b.height < height - 2 * marginY)
				v->ResizeFrame(0, 0, 0, height - b.height - 2 * marginY);
		}
		
		result->SetFrame(MRect(inX, inY, width, height));
	}

	string bindings = inTemplate->get_attribute("bind");
	result->SetBindings(
		ba::contains(bindings, "left"),
		ba::contains(bindings, "top"),
		ba::contains(bindings, "right"),
		ba::contains(bindings, "bottom"));
	
	return result;
}

void MWinDialogImpl::GetTextMetrics(const string& inText, const wchar_t* inClass,
	int inPartID, int inStateID, int32& outWidth, int32& outHeight)
{
	HTHEME hTheme = ::OpenThemeData(GetHandle(), inClass);
	if (hTheme != nil)
	{
		wstring text(c2w(inText));
		RECT r;

		THROW_IF_HRESULT_ERROR(::GetThemeTextExtent(hTheme, mDC, inPartID, inStateID, text.c_str(),
			text.length(), 0, nil, &r));

		outWidth = r.right - r.left;
		outHeight = r.bottom - r.top;

		::CloseThemeData(hTheme);
	}
}

// --------------------------------------------------------------------

MWindowImpl* MWindowImpl::CreateDialog(const string& inResource, MWindow* inWindow)
{
	return new MWinDialogImpl(inResource, inWindow);
}
