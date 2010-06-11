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

namespace {

uint32 str2id(const string& inID)
{
	uint32 id = 0;
	for (string::const_iterator ch = inID.begin(); ch != inID.end(); ++ch)
		id = id << 8 | uint8(*ch);
	return id;
}

}

class MWinDialogImpl : public MWinWindowImpl
{
  public:
					MWinDialogImpl(const string& inResource, MWindow* inWindow);
	virtual			~MWinDialogImpl();

	virtual void	Finish();

private:

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle, wstring& outClassName, HMENU& outMenu);
	virtual void	RegisterParams(UINT& outStyle, int& outWndExtra, HCURSOR& outCursor, HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground);

	void			GetMargins(xml::element* inTemplate,
						int32& outLeftMargin, int32& outTopMargin,
						int32& outRightMargin, int32& outBottomMargin);

	MView*			CreateControls(xml::element* inTemplate, int32 inX, int32 inY);

	MView*			CreateButton(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreateCaption(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreateCheckbox(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreateCombobox(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreateSeparator(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreateVBox(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreateHBox(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreateTable(xml::element* inTemplate, int32 inX, int32 inY);
//	MView*			CreateTable(xml::element* inTemplate, int32 inX, int32 inY);

	uint32			GetTextWidth(const string& inText,
						const wchar_t* inClass, int inPartID, int inStateID);

	string			mRsrc;
	HDC				mDC;
	float			mDLUX, mDLUY;
};

MWinDialogImpl::MWinDialogImpl(const string& inResource, MWindow* inWindow)
	: MWinWindowImpl(MWindowFlags(0), "", inWindow)
	, mRsrc(inResource)
	, mDC(nil)
	, mDLUX(1.75)
	, mDLUY(1.875)
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

	// now we have the handle, get the DC and theme font
	mDC = ::GetDC(GetHandle());

	HTHEME hTheme = ::OpenThemeData(GetHandle(), VSCLASS_TEXTSTYLE);

	TEXTMETRIC tm;
	if (hTheme != nil)
	{
		::GetThemeTextMetrics(hTheme, mDC, TEXT_BODYTEXT, TS_CONTROLLABEL_NORMAL, &tm);

		RECT r;
		THROW_IF_HRESULT_ERROR(::GetThemeTextExtent(hTheme, mDC, TEXT_BODYTEXT, TS_CONTROLLABEL_NORMAL, 
			L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, 0, nil, &r));

		mDLUY = tm.tmHeight / 8.0;
		mDLUX = (r.right - r.left) / (52 * 4.0);

		::CloseThemeData(hTheme);
	}
	else
	{
		::SelectObject(mDC, ::GetStockObject(DEFAULT_GUI_FONT));
		::GetTextMetrics(mDC, &tm);

		SIZE size;
		::GetTextExtentPoint32(mDC, L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &size);

		mDLUY = tm.tmHeight / 8.0;
		mDLUX = size.cx / (52 * 4.0);
	}

	// create the dialog controls, all stacked on top of each other
	MView* content = CreateControls(dialog, 0, 0);
	mWindow->AddChild(content);

	RECT cr;
	::GetClientRect(GetHandle(), &cr);

	content->GetBounds(bounds);

	int32 dw = bounds.width - (cr.right - cr.left);		if (dw < 0) dw = 0;
	int32 dh = bounds.height - (cr.bottom - cr.top);	if (dh < 0) dh = 0;

	if (dw > 0 or dh > 0)
	{
		MRect p;
		mWindow->GetWindowPosition(p);
//		p.x -= dw / 2;
//		p.y -= dh / 2;
		p.width += dw;
		p.height += dh;
		mWindow->SetWindowPosition(p);
	}
}

void MWinDialogImpl::GetMargins(xml::element* inTemplate,
	int32& outLeftMargin, int32& outTopMargin,
	int32& outRightMargin, int32& outBottomMargin)
{
	outLeftMargin = outTopMargin = outRightMargin = outBottomMargin = 0;
	
	string m = inTemplate->get_attribute("margin");
	if (not m.empty())
		outLeftMargin = outRightMargin =
		outTopMargin = outBottomMargin = boost::lexical_cast<int32>(m);

	m = inTemplate->get_attribute("margin-left-right");
	if (not m.empty())
		outLeftMargin = outRightMargin = boost::lexical_cast<int32>(m);

	m = inTemplate->get_attribute("margin-top-bottom");
	if (not m.empty())
		outTopMargin = outBottomMargin = boost::lexical_cast<int32>(m);

	m = inTemplate->get_attribute("margin-left");
	if (not m.empty())
		outLeftMargin = boost::lexical_cast<int32>(m);

	m = inTemplate->get_attribute("margin-top");
	if (not m.empty())
		outTopMargin = boost::lexical_cast<int32>(m);

	m = inTemplate->get_attribute("margin-right");
	if (not m.empty())
		outRightMargin = boost::lexical_cast<int32>(m);

	m = inTemplate->get_attribute("margin-bottom");
	if (not m.empty())
		outBottomMargin = boost::lexical_cast<int32>(m);

	outLeftMargin *= mDLUX;
	outRightMargin *= mDLUX;
	outTopMargin *= mDLUY;
	outBottomMargin *= mDLUY;
}

MView* MWinDialogImpl::CreateButton(xml::element* inTemplate, int32 inX, int32 inY)
{
	uint32 id = str2id(inTemplate->get_attribute("id"));
	string title = inTemplate->get_attribute("title");
	
	MRect bounds(inX, inY, 50 * mDLUX, 14 * mDLUY);
	MButton* button = new MButton(id, bounds, title);

	uint32 idealWidth = GetTextWidth(title, VSCLASS_BUTTON, BP_PUSHBUTTON, PBS_NORMAL) + 10 * mDLUX;
	if (idealWidth > bounds.width)
		button->ResizeFrame(0, 0, idealWidth - bounds.width, 0);
	
	if (inTemplate->get_attribute("default") == "true")
		button->MakeDefault(true);

	return button;
}

MView* MWinDialogImpl::CreateCaption(xml::element* inTemplate, int32 inX, int32 inY)
{
	string text = inTemplate->get_attribute("text");

	MRect bounds(inX, inY + 2 * mDLUY, 0, 8 * mDLUY);
	bounds.width = GetTextWidth(text, VSCLASS_STATIC, STAT_TEXT, 0);
	return new MCaption('capt', bounds, text);
}

MView* MWinDialogImpl::CreateCheckbox(xml::element* inTemplate, int32 inX, int32 inY)
{
	uint32 id = str2id(inTemplate->get_attribute("id"));
	string title = inTemplate->get_attribute("title");

	MRect bounds(inX, inY + 2 * mDLUY, 0, 10 * mDLUY);
	bounds.width = 10 * mDLUY +
		GetTextWidth(title, VSCLASS_STATIC, STAT_TEXT, 0);
	return new MCheckbox(id, bounds, title);
}

MView* MWinDialogImpl::CreateCombobox(xml::element* inTemplate, int32 inX, int32 inY)
{
	uint32 id = str2id(inTemplate->get_attribute("id"));

	MRect bounds(inX, inY, 50 * mDLUX, 14 * mDLUY);
	return new MCombobox(id, bounds, true);
}

MView* MWinDialogImpl::CreateSeparator(xml::element* inTemplate, int32 inX, int32 inY)
{
	MRect bounds(inX, inY, 2, 2);
	return new MSeparator('sepa', bounds);
}

MView* MWinDialogImpl::CreateVBox(xml::element* inTemplate, int32 inX, int32 inY)
{
	int32 y = 0;
	int32 width = 0;
	
	MView* result = new MView('vbox', MRect(inX, inY, 0, 0));
	
	vector<MView*> views;
	
	foreach (xml::element* b, inTemplate->children<xml::element>())
	{
		if (not views.empty())
			y += 4 * mDLUY;

		MView* v = CreateControls(b, 0, y);
		views.push_back(v);
		result->AddChild(v);
		
		MRect r;
		v->GetBounds(r);

		if (width < r.width)
			width = r.width;
		
		y += r.height;
	}
	
	int32 height = y;
	
	foreach (MView* v, views)
	{
		MRect b;
		v->GetBounds(b);
		if (b.width < width)
			v->ResizeFrame(0, 0, width - b.width, 0);
	}

	result->SetFrame(MRect(inX, inY, width, height));
	
	return result;
}

MView* MWinDialogImpl::CreateHBox(xml::element* inTemplate, int32 inX, int32 inY)
{
	int32 x = 0;
	int32 height = 0;
	
	MView* result = new MView('hbox', MRect(inX, inY, 0, 0));
	
	vector<MView*> views;
	
	foreach (xml::element* b, inTemplate->children<xml::element>())
	{
		if (not views.empty())
			x += 4 * mDLUX;

		MView* v = CreateControls(b, x, 0);
		views.push_back(v);
		result->AddChild(v);
		
		MRect r;
		v->GetBounds(r);

		if (height < r.height)
			height = r.height;
		
		x += r.width;
	}
	
	int32 width = x;
	
	foreach (MView* v, views)
	{
		MRect b;
		v->GetBounds(b);
		if (b.height < height)
			v->ResizeFrame(0, 0, 0, height - b.height);
	}

	result->SetFrame(MRect(inX, inY, width, height));
	
	return result;
}

MView* MWinDialogImpl::CreateTable(xml::element* inTemplate, int32 inX, int32 inY)
{
	MView* result = new MView('tabl', MRect(inX, inY, 0, 0));
	
	vector<vector<MView*> > rows;
	vector<int32> widths;

	int32 x = 0;
	int32 y = 0;
	
	foreach (xml::element* row, inTemplate->find("./row"))
	{
		x = 0;

		vector<MView*> cols;
		int32 height = 0;
		
		if (not rows.empty())
			y += 4 * mDLUY;
		
		foreach (xml::element* col, row->children<xml::element>())
		{
			if (not cols.empty())
				x += 4 * mDLUX;
			
			MView* view = CreateControls(col, x, y);
			cols.push_back(view);
			result->AddChild(view);
			
			MRect frame;
			view->GetFrame(frame);
			
			int32 width = frame.width + frame.x - x;
			x += width;
			
			if (cols.size() > widths.size())
				widths.push_back(width);
			else if (widths[cols.size() - 1] < width);
				widths[cols.size() - 1] = width;
			
			if (height < frame.height + frame.y - y)
				height = frame.height + frame.y - y;
		}

		foreach (MView* v, cols)
		{
			MRect b;
			v->GetBounds(b);
			if (b.height < height)
				v->ResizeFrame(0, 0, 0, height - b.height);
		}
		
		y += height;
		
		if (not cols.empty())
			rows.push_back(cols);
	}
	
	result->SetFrame(MRect(inX, inY, x, y));
	
	y = 0;
	foreach (vector<MView*>& row, rows)
	{
		x = 0;
		int32 n = 0, height;
		
		foreach (MView* col, row)
		{
			MRect frame;
			col->GetFrame(frame);
			
			col->ResizeFrame(x - frame.x, y - frame.y,
				widths[n] - frame.width, 0);
			
			x += widths[n];
			++n;
			height = frame.height;
		}
		
		y += height;
	}

	return result;
}

MView* MWinDialogImpl::CreateControls(xml::element* inTemplate, int32 inX, int32 inY)
{
	MView* result = nil;

	string name = inTemplate->name();

	int32 marginLeft, marginTop, marginRight, marginBottom;
	GetMargins(inTemplate, marginLeft, marginTop, marginRight, marginBottom);

	if (name == "button")
		result = CreateButton(inTemplate, inX + marginLeft, inY + marginTop);
	else if (name == "caption")
		result = CreateCaption(inTemplate, inX + marginLeft, inY + marginTop);
	else if (name == "checkbox")
		result = CreateCheckbox(inTemplate, inX + marginLeft, inY + marginTop);
	else if (name == "combobox")
		result = CreateCombobox(inTemplate, inX + marginLeft, inY + marginTop);
	else if (name == "separator")
		result = CreateSeparator(inTemplate, inX + marginLeft, inY + marginTop);
	else if (name == "table")
		result = CreateTable(inTemplate, inX + marginLeft, inY + marginTop);
	else if (name == "vbox" or name == "dialog")
		result = CreateVBox(inTemplate, inX + marginLeft, inY + marginTop);
	else if (name == "hbox")
		result = CreateHBox(inTemplate, inX + marginLeft, inY + marginTop);
	
	if (not inTemplate->get_attribute("width").empty())
	{
		uint32 width = boost::lexical_cast<uint32>(inTemplate->get_attribute("width")) * mDLUX;
		
		MRect bounds;
		result->GetBounds(bounds);
		if (width > bounds.width)
			result->ResizeFrame(0, 0, width - bounds.width, 0);
	}
	
	MRect frame;
	result->GetFrame(frame);
	frame.width += marginLeft + marginRight;
	frame.height += marginTop + marginBottom;
	result->SetFrame(frame);

	string bindings = inTemplate->get_attribute("bind");
	result->SetBindings(
		ba::contains(bindings, "left"),
		ba::contains(bindings, "top"),
		ba::contains(bindings, "right"),
		ba::contains(bindings, "bottom"));
	
	return result;
}

uint32 MWinDialogImpl::GetTextWidth(const string& inText,
	const wchar_t* inClass, int inPartID, int inStateID)
{
	uint32 result = 0;
	wstring text(c2w(inText));
	
	HTHEME hTheme = ::OpenThemeData(GetHandle(), inClass);

	if (hTheme != nil)
	{
		RECT r;
		THROW_IF_HRESULT_ERROR(::GetThemeTextExtent(hTheme, mDC,
			inPartID, inStateID, text.c_str(), text.length(), 0, nil, &r));
		result = r.right - r.left;
		::CloseThemeData(hTheme);
	}
	else
	{
		SIZE size;
		::GetTextExtentPoint32(mDC, text.c_str(), text.length(), &size);
		result = size.cx;
	}
	
	return result;
}

// --------------------------------------------------------------------

MWindowImpl* MWindowImpl::CreateDialog(const string& inResource, MWindow* inWindow)
{
	return new MWinDialogImpl(inResource, inWindow);
}
