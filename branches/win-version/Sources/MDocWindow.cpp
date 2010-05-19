//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include <sstream>
#include <cassert>

#include "MDocWindow.h"
#include "MDocWindow.h"
#include "MStrings.h"

using namespace std;

// ------------------------------------------------------------------
//

MDocWindow::MDocWindow(
	const char*		inResource)
	: MWindow(inResource)
	, eModifiedChanged(this, &MDocWindow::ModifiedChanged)
	, eFileSpecChanged(this, &MDocWindow::FileSpecChanged)
	, eDocumentChanged(this, &MDocWindow::DocumentChanged)
	, mController(nil)
	//, mMenubar(this)
{
	//GdkGeometry geom = {};
	//geom.min_width = 300;
	//geom.min_height = 100;
	//gtk_window_set_geometry_hints(GTK_WINDOW(GetGtkWidget()), nil, &geom, GDK_HINT_MIN_SIZE);
}

MDocWindow::~MDocWindow()
{
	delete mController;
}

void MDocWindow::Initialize(
	MDocument*		inDocument)
{
	if (mController == nil)
		mController = new MController(this);
	
	mController->SetWindow(this);
	mController->SetDocument(inDocument);
}

bool MDocWindow::DoClose()
{
	return mController->TryCloseController(kSaveChangesClosingDocument);
}

MDocWindow* MDocWindow::FindWindowForDocument(MDocument* inDocument)
{
	MWindow* w = MWindow::GetFirstWindow();

	while (w != nil)
	{
		MDocWindow* d = dynamic_cast<MDocWindow*>(w);

		if (d != nil and d->GetDocument() == inDocument)
			break;

		w = w->GetNextWindow();
	}

	return static_cast<MDocWindow*>(w);
}

string MDocWindow::GetUntitledTitle()
{
	static int sDocNr = 0;
	stringstream result;
	
	result << _("Untitled");
	
	if (++sDocNr > 1)
		result << ' ' << sDocNr;
	
	return result.str();
}

bool MDocWindow::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	return MWindow::UpdateCommandStatus(
		inCommand, inMenu, inItemIndex, outEnabled, outChecked);
}

bool MDocWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	return MWindow::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
}

MDocument* MDocWindow::GetDocument()
{
	return mController->GetDocument();
}

void MDocWindow::SaveState()
{
}

void MDocWindow::DocumentChanged(
	MDocument*		inDocument)
{
	if (inDocument != nil)
	{
		// set title
		
		if (inDocument->IsSpecified())
			FileSpecChanged(inDocument, inDocument->GetFile());
		else
			SetTitle(GetUntitledTitle());

		ModifiedChanged(inDocument->IsModified());
	}
	else
		Close();
}

void MDocWindow::ModifiedChanged(
	bool			inModified)
{
	SetModifiedMarkInTitle(inModified);
}

void MDocWindow::FileSpecChanged(
	MDocument*		inDocument,
	const MFile&		inFile)
{
	MDocument* doc = mController->GetDocument();

	if (doc != nil)
	{
		string title = doc->GetWindowTitle();;

		if (doc->IsReadOnly())
			title += _(" [Read Only]");
		
		SetTitle(title);
	}
	else
		SetTitle(GetUntitledTitle());
}

void MDocWindow::AddRoutes(
	MDocument*		inDocument)
{
	AddRoute(inDocument->eModifiedChanged, eModifiedChanged);
	AddRoute(inDocument->eFileSpecChanged, eFileSpecChanged);
}

void MDocWindow::RemoveRoutes(
	MDocument*		inDocument)
{
	RemoveRoute(inDocument->eModifiedChanged, eModifiedChanged);
	RemoveRoute(inDocument->eFileSpecChanged, eFileSpecChanged);
}

void MDocWindow::BeFocus()
{
	mController->TakeFocus();
}
