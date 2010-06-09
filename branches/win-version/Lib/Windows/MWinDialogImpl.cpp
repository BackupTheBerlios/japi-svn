//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include "zeep/xml/document.hpp"
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem/fstream.hpp>

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

using namespace std;
using namespace zeep;
namespace io = boost::iostreams;

class MWinDialogImpl : public MWinWindowImpl
{
  public:
					MWinDialogImpl(const string& inResource, MWindow* inWindow);
	virtual			~MWinDialogImpl();

	virtual void	Create();

private:

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle, wstring& outClassName, HMENU& outMenu);
	virtual void	RegisterParams(UINT& outStyle, HCURSOR& outCursor, HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground);

	string			mRsrc;
};

MWinDialogImpl::MWinDialogImpl(const string& inResource, MWindow* inWindow)
	: MWinWindowImpl(MWindowFlags(0), "", inWindow)
	, mRsrc(inResource)
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
	outExStyle = 0;
	outMenu = nil;
}

void MWinDialogImpl::RegisterParams(UINT& outStyle, HCURSOR& outCursor,
	HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground)
{
	MWinWindowImpl::RegisterParams(outStyle, outCursor, outIcon, outSmallIcon, outBackground);
	
	HINSTANCE inst = MWinApplicationImpl::GetInstance()->GetHInstance();
	
	outStyle = 0;// CS_HREDRAW | CS_VREDRAW;
	//outIcon = ::LoadIcon(inst, MAKEINTRESOURCE(ID_DEF_DOC_ICON));
	//outSmallIcon = ::LoadIcon(inst, MAKEINTRESOURCE(ID_DEF_DOC_ICON));
	outCursor = ::LoadCursor(NULL, IDC_ARROW);
	outBackground = (HBRUSH)(COLOR_BTNFACE + 1);
}

void MWinDialogImpl::Create()
{
	MRect bounds(CW_USEDEFAULT, CW_USEDEFAULT, 100, 100);

	MWinProcMixin::CreateHandle(nil, bounds, L"My First Dialog");

	mrsrc::rsrc rsrc(string("Dialogs/") + mRsrc + ".ui");
		
	if (not rsrc)
		THROW(("Dialog resource not found: %s", mRsrc.c_str()));

	io::stream<io::array_source> data(rsrc.data(), rsrc.size());
	xml::document doc(data);

	xml::element* e = doc.find_first("/interface/object[@class='GtkDialog']/property[@name='title']");
	if (e != nil)
		SetTitle(e->content());

	// create a plan for the layout
}

// --------------------------------------------------------------------

MWindowImpl* MWindowImpl::CreateDialog(const string& inResource, MWindow* inWindow)
{
	MWinDialogImpl* result = new MWinDialogImpl(inResource, inWindow);
	result->Create();
	return result;
}
