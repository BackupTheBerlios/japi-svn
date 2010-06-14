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
#include "MDialog.h"

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

	void			GetMargins(xml::element* inTemplate,
						int32& outLeftMargin, int32& outTopMargin,
						int32& outRightMargin, int32& outBottomMargin);

	MView*			CreateControls(xml::element* inTemplate, int32 inX, int32 inY);

	MView*			CreateButton(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreateCaption(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreateCheckbox(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreateCombobox(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreateEdittext(xml::element* inTemplate, int32 inX, int32 inY);
	MView*			CreatePopup(xml::element* inTemplate, int32 inX, int32 inY);
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

	mWindow->AddChild(content);
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
	string id = inTemplate->get_attribute("id");
	string title = inTemplate->get_attribute("title");
	
	uint32 idealWidth = GetTextWidth(title, VSCLASS_BUTTON, BP_PUSHBUTTON, PBS_NORMAL) + 10 * mDLUX;
	if (idealWidth < 50 * mDLUX)
		idealWidth = 50 * mDLUX;
	MRect bounds(inX, inY, idealWidth, 14 * mDLUY);

	MButton* button = new MButton(id, bounds, title);
	
	if (inTemplate->get_attribute("default") == "true")
		button->MakeDefault(true);

	AddRoute(button->eClicked, static_cast<MDialog*>(mWindow)->eButtonClicked);

	return button;
}

MView* MWinDialogImpl::CreateCaption(xml::element* inTemplate, int32 inX, int32 inY)
{
	string text = inTemplate->get_attribute("text");

	MRect bounds(inX, inY + 2 * mDLUY, 0, 8 * mDLUY);
	bounds.width = GetTextWidth(text, VSCLASS_STATIC, STAT_TEXT, 0);
	return new MCaption("caption", bounds, text);
}

MView* MWinDialogImpl::CreateCheckbox(xml::element* inTemplate, int32 inX, int32 inY)
{
	string id = inTemplate->get_attribute("id");
	string title = inTemplate->get_attribute("title");

	MRect bounds(inX, inY + 2 * mDLUY, 0, 10 * mDLUY);
	bounds.width = 14 * mDLUX +
		GetTextWidth(title, VSCLASS_BUTTON, BP_CHECKBOX, PBS_NORMAL);
	MCheckbox* checkbox = new MCheckbox(id, bounds, title);
	AddRoute(checkbox->eValueChanged,
		static_cast<MDialog*>(mWindow)->eCheckboxClicked);
	return checkbox;
}

MView* MWinDialogImpl::CreateCombobox(xml::element* inTemplate, int32 inX, int32 inY)
{
	string id = inTemplate->get_attribute("id");

	MRect bounds(inX, inY, 50 * mDLUX, 14 * mDLUY);
	MCombobox* combobox = new MCombobox(id, bounds);
	AddRoute(combobox->eValueChanged,
		static_cast<MDialog*>(mWindow)->eTextChanged);
	return combobox;
}

MView* MWinDialogImpl::CreateEdittext(xml::element* inTemplate, int32 inX, int32 inY)
{
	string id = inTemplate->get_attribute("id");

	MRect bounds(inX, inY, 50 * mDLUX, 14 * mDLUY);
	MEdittext* edittext = new MEdittext(id, bounds);
	AddRoute(edittext->eValueChanged,
		static_cast<MDialog*>(mWindow)->eTextChanged);
	return edittext;
}

MView* MWinDialogImpl::CreatePopup(xml::element* inTemplate, int32 inX, int32 inY)
{
	string id = inTemplate->get_attribute("id");

	MRect bounds(inX, inY, 0, 14 * mDLUY);
	
	vector<string> choices;
	foreach (xml::element* option, inTemplate->find("./option"))
	{
		string label = option->content();
		int32 width = GetTextWidth(label, VSCLASS_COMBOBOX, CP_DROPDOWNBUTTON, CBXSL_NORMAL);
		if (bounds.width < width)
			bounds.width = width;
		choices.push_back(label);
	}

	bounds.width += 14 * mDLUX;

	MPopup* popup = new MPopup(id, bounds);

	popup->SetChoices(choices);
	//AddRoute(popup->eValueChanged,
	//	static_cast<MDialog*>(mWindow)->eValueChanged);

	return popup;
}

MView* MWinDialogImpl::CreateSeparator(xml::element* inTemplate, int32 inX, int32 inY)
{
	MRect bounds(inX, inY, 2, 2);
	return new MSeparator("separator", bounds);
}

MView* MWinDialogImpl::CreateVBox(xml::element* inTemplate, int32 inX, int32 inY)
{
	MRect r(inX, inY, 0, 0);
	MView* result = new MVBox("vbox", r, 4 * mDLUY);
	
	foreach (xml::element* b, inTemplate->children<xml::element>())
		result->AddChild(CreateControls(b, 0, 0));
	
	result->GetViewSize(r.width, r.height);
	result->SetFrame(r);
	
	return result;
}

MView* MWinDialogImpl::CreateHBox(xml::element* inTemplate, int32 inX, int32 inY)
{
	MRect r(inX, inY, 0, 0);
	MView* result = new MHBox("hbox", r, 4 * mDLUX);
	
	foreach (xml::element* b, inTemplate->children<xml::element>())
		result->AddChild(CreateControls(b, 0, 0));
	
	result->GetViewSize(r.width, r.height);
	result->SetFrame(r);
	
	return result;
}

MView* MWinDialogImpl::CreateTable(xml::element* inTemplate, int32 inX, int32 inY)
{
//	MView* result = new MVBox("table-vbox", MRect(inX, inY, 0, 0), 4 * mDLUY);
//
//	foreach (xml::element* row, inTemplate->find("./row"))
//	{
//		MView* r = new MHBox("table-hbox", MRect(0, 0, 0, 0), 4 * mDLUX);
//		foreach (xml::element* col, row->children<xml::element>())
//			r->AddChild(CreateControls(col, 0, 0));
//		result->AddChild(r);
//	}
//	return result;

	vector<MView*> views;
	uint32 colCount = 0, rowCount = 0;
	
	foreach (xml::element* row, inTemplate->find("./row"))
	{
		int32 cn = 0;
		
		foreach (xml::element* col, row->children<xml::element>())
		{
			++cn;
			if (colCount < cn)
				colCount = cn;
			views.push_back(CreateControls(col, 0, 0));
		}
		
		++rowCount;
	}

	// fix me!
	while (views.size() < (rowCount * colCount))
		views.push_back(nil);
	
	MRect r(inX, inY, 0, 0);
	MTable* result = new MTable("table", r,
		&views[0], colCount, rowCount, 4 * mDLUX, 4 * mDLUY);
	
	result->GetViewSize(r.width, r.height);
	result->SetFrame(r);

	return result;
}

MView* MWinDialogImpl::CreateControls(xml::element* inTemplate, int32 inX, int32 inY)
{
	MView* result = nil;

	string name = inTemplate->name();

	if (name == "button")
		result = CreateButton(inTemplate, inX, inY);
	else if (name == "caption")
		result = CreateCaption(inTemplate, inX, inY);
	else if (name == "checkbox")
		result = CreateCheckbox(inTemplate, inX, inY);
	else if (name == "combobox")
		result = CreateCombobox(inTemplate, inX, inY);
	else if (name == "edittext")
		result = CreateEdittext(inTemplate, inX, inY);
	else if (name == "popup")
		result = CreatePopup(inTemplate, inX, inY);
	else if (name == "separator")
		result = CreateSeparator(inTemplate, inX, inY);
	else if (name == "table")
		result = CreateTable(inTemplate, inX, inY);
	else if (name == "vbox" or name == "dialog")
		result = CreateVBox(inTemplate, inX, inY);
	else if (name == "hbox")
		result = CreateHBox(inTemplate, inX, inY);
	
	if (not inTemplate->get_attribute("width").empty())
	{
		uint32 width = boost::lexical_cast<uint32>(inTemplate->get_attribute("width")) * mDLUX;
		
		MRect frame;
		result->GetFrame(frame);
		if (frame.width < width)
			result->ResizeFrame(0, 0, width - frame.width, 0);
	}

	int32 marginLeft, marginTop, marginRight, marginBottom;
	GetMargins(inTemplate, marginLeft, marginTop, marginRight, marginBottom);

	if (marginLeft != 0 or marginTop != 0)
		result->ResizeFrame(marginLeft, marginTop, 0, 0);

	if (marginRight != 0 or marginBottom != 0)
	{
		MRect frame;
		result->GetFrame(frame);
		frame.width += marginRight;
		frame.height += marginBottom;
		result->SetFrame(frame);

		result->SetViewSize(frame.width, frame.height);
	}

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
