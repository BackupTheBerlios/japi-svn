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
#include "MApplication.h"

using namespace std;

// ------------------------------------------------------------------
//

MDocWindow::MDocWindow(const string& inTitle, const MRect& inBounds,
		MWindowFlags inFlags, const string& inMenu)
	: MWindow(inTitle, inBounds, inFlags, inMenu)
	, eModifiedChanged(this, &MDocWindow::ModifiedChanged)
	, eFileSpecChanged(this, &MDocWindow::FileSpecChanged)
	, eDocumentChanged(this, &MDocWindow::DocumentChanged)
	, mController(this)
{
	SetFocus(&mController);
}

MDocWindow::~MDocWindow()
{
}

void MDocWindow::SetDocument(
	MDocument*		inDocument)
{
	mController.SetDocument(inDocument);
}

bool MDocWindow::DoClose()
{
	return mController.TryCloseController(kSaveChangesClosingDocument);
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
	bool handled = true;

	switch (inCommand)
	{
		//case cmd_Close:
		//	outEnabled = true;
		//	break;

		default:
			handled = MWindow::UpdateCommandStatus(
				inCommand, inMenu, inItemIndex, outEnabled, outChecked);
			break;
	}

	return handled;
}

bool MDocWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool handled = true;

	switch (inCommand)
	{
		//case cmd_Close:
		//	
		//	break;

		default:
			handled = MWindow::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
			break;
	}

	return handled;
}

MDocument* MDocWindow::GetDocument()
{
	return mController.GetDocument();
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
	MDocument* doc = mController.GetDocument();

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

