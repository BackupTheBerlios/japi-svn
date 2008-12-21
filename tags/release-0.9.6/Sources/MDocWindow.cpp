/*
	Copyright (c) 2006, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "MJapi.h"

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
	, mMenubar(this)
{
	GdkGeometry geom = {};
	geom.min_width = 300;
	geom.min_height = 100;
	gtk_window_set_geometry_hints(GTK_WINDOW(GetGtkWidget()), nil, &geom, GDK_HINT_MIN_SIZE);
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

void MDocWindow::DocumentChanged(
	MDocument*		inDocument)
{
	if (inDocument != nil)
	{
		// set title
		
		if (inDocument->IsSpecified())
			FileSpecChanged(inDocument->GetURL());
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
	const MUrl&		inFile)
{
	if (not inFile.IsLocal() or fs::exists(inFile.GetPath()))
	{
		MDocument* doc = mController->GetDocument();
		
		string title = inFile.str();
		
		if (doc != nil and doc->IsReadOnly())
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
