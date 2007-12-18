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

#include "MJapieG.h"

#include <sstream>

#include "MFile.h"
#include "MDocument.h"
#include "MDiffWindow.h"
#include "MGlobals.h"
#include "MUtils.h"
#include "MMenu.h"
#include "MDevice.h"
#include "MError.h"
#include "MDocWindow.h"
#include "MStrings.h"

using namespace std;

namespace
{
enum {
	kListViewID = 'tree',
	
	kChooseFile1Command	= 'fil1',
	kChooseFile2Command	= 'fil2',
	
	kMergeToFile1Command = 'mrg1',
	kMergeToFile2Command = 'mrg2'
};

}

// ------------------------------------------------------------------
//

MDiffWindow::MDiffWindow(
	GladeXML*			inGlade,
	GtkWidget*			inRoot)
	: MDialog(inGlade, inRoot)
	, eDocument1Closed(this, &MDiffWindow::Document1Closed)
	, eDocument2Closed(this, &MDiffWindow::Document2Closed)
	, mSelected(this, &MDiffWindow::DiffSelected)
	, mDoc1(nil)
	, mDoc2(nil)
{
}

MDiffWindow::~MDiffWindow()
{
}

void MDiffWindow::Init()
{
	GtkWidget* treeView = GetWidget(kListViewID);
	THROW_IF_NIL((treeView));

	GtkListStore* store = gtk_list_store_new(1, G_TYPE_STRING);

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeView), (GTK_TREE_MODEL(store)));
	
	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes (
		_("Difference"), renderer, "text", 0, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);
	
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
	mSelected.Connect(G_OBJECT(selection), "changed");
}

void MDiffWindow::ValueChanged(
	uint32			inID)
{
	ProcessCommand(inID, nil, 0);
}

bool MDiffWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex)
{
	bool result = true;
	
	switch (inCommand)
	{
		case kChooseFile1Command:
			ChooseFile(1);
			break;
		
		case kChooseFile2Command:
			ChooseFile(2);
			break;
		
		case kMergeToFile1Command:
			MergeToFile(1);
			break;
		
		case kMergeToFile2Command:
			MergeToFile(2);
			break;
		
		default:
			result = MDialog::ProcessCommand(inCommand, inMenu, inItemIndex);
			break;
	}
	
	return result;
}

bool MDiffWindow::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = true;
	
	switch (inCommand)
	{
		default:
			result = MDialog::UpdateCommandStatus(inCommand, inMenu, inItemIndex,
						outEnabled, outChecked);
			break;
	}
	
	return result;
}

//void MDiffWindow::DrawItem(CGContextRef inContext, HIRect inFrame, uint32 inRow,
//	bool inSelected, const void* inData, uint32 inDataLength)
//{
//	MDiffInfo diff = mScript[inRow];
//	
//	string s;
//	int n0 = 0, n1 = 0, n2 = 0, n3 = 0;
//	
//	if (diff.mA1 == diff.mA2)
//	{
//		if (diff.mB1 < diff.mB2 - 1)
//		{
//			s = "Extra lines in file ^0: ^1-^2";
//			n0 = 2;
//			n1 = diff.mB1 + 1;
//			n2 = diff.mB2;
//		}
//		else
//		{
//			s = "Extra line in file ^0: ^1";
//			n0 = 2;
//			n1 = diff.mB1 + 1;
//		}
//	}
//	else if (diff.mB1 == diff.mB2)
//	{
//		if (diff.mA1 < diff.mA2 - 1)
//		{
//			s = "Extra lines in file ^0: ^1-^2";
//			n0 = 1;
//			n1 = diff.mA1 + 1;
//			n2 = diff.mA2;
//		}
//		else
//		{
//			s = "Extra line in file ^0: ^1";
//			n0 = 1;
//			n1 = diff.mA1 + 1;
//		}
//	}
//	else
//	{
//		if (diff.mA1 < diff.mA2 - 1 && diff.mB1 < diff.mB2 - 1)
//		{
//			s = "Nonmatching lines. File 1: ^0-^1, File 2: ^2-^3";
//			n0 = diff.mA1 + 1;
//			n1 = diff.mA2;
//			n2 = diff.mB1 + 1;
//			n3 = diff.mB2;
//		}
//		else if (diff.mB1 < diff.mB2 - 1)
//		{
//			s = "Nonmatching lines. File 1: ^0, File 2: ^1-^2";
//			n0 = diff.mA1 + 1;
//			n1 = diff.mB1 + 1;
//			n2 = diff.mB2;
//		}
//		else if (diff.mA1 < diff.mA2 - 1)
//		{
//			s = "Nonmatching lines. File 1: ^0-^1, File 2: ^2";
//			n0 = diff.mA1 + 1;
//			n1 = diff.mA2;
//			n2 = diff.mB1 + 1;
//		}
//		else
//		{
//			s = "Nonmatching lines. File 1: ^0, File 2: ^1";
//			n0 = diff.mA1 + 1;
//			n1 = diff.mB1 + 1;
//		}
//	}
//	
//	string::size_type p;
//	
//	s = GetLocalisedString(s.c_str());
//	
//	if ((p = s.find("^0")) != string::npos)
//		s.replace(p, 2, NumToString(n0));
//	if ((p = s.find("^1")) != string::npos)
//		s.replace(p, 2, NumToString(n1));
//	if ((p = s.find("^2")) != string::npos)
//		s.replace(p, 2, NumToString(n2));
//	if ((p = s.find("^3")) != string::npos)
//		s.replace(p, 2, NumToString(n3));
//	
//	MDevice dev(inContext, inFrame);
//
//	if (inSelected)
//		dev.SetBackColor(gHiliteColor);
//	else if ((inRow % 2) == 0)
//		dev.SetBackColor(gOddRowColor);
//	else
//		dev.SetBackColor(kWhite);
//	
//	dev.EraseRect(inFrame);
//	
//	float x, y;
//	x = inFrame.origin.x + 4;
//	y = inFrame.origin.y + dev.GetAscent() + 1;
//
//	dev.DrawString(s, x, y);
//}
//
//// ----------------------------------------------------------------------------
//// MDiffWindow::DrawDirItem
//
//void MDiffWindow::DrawDirItem(CGContextRef inContext, HIRect inFrame, uint32 inRow,
//	bool inSelected, const void* inData, uint32 inDataLength)
//{
//	MDirDiffItem diff = mDScript[inRow];
//	
//	string s;
//	
//	if (diff.status == 0)
//		s = diff.name;
//	else
//	{
//		s = GetLocalisedString("File '^0' only in dir ^1");
//		s.replace(s.find("^0"), 2, diff.name);
//		s.replace(s.find("^1"), 2, NumToString(diff.status));
//	}
//	
//	MDevice dev(inContext, inFrame);
//
//	if (inSelected)
//		dev.SetBackColor(gHiliteColor);
//	else if ((inRow % 2) == 0)
//		dev.SetBackColor(gOddRowColor);
//	else
//		dev.SetBackColor(kWhite);
//	
//	dev.EraseRect(inFrame);
//	
//	float x, y;
//	x = inFrame.origin.x + 4;
//	y = inFrame.origin.y + dev.GetAscent() + 1;
//
//	dev.DrawString(s, x, y);
//}

// ----------------------------------------------------------------------------
// MDiffWindow::ChooseFile

void MDiffWindow::ChooseFile(int inFileNr)
{
	MUrl url;
	
	if (inFileNr == 1)
	{
		if (mDoc1 != nil)
			url = mDoc1->GetURL();
		else
			url = MUrl(mDir1);
	}
	else if (inFileNr == 2)
	{
		if (mDoc2 != nil)
			url = mDoc2->GetURL();
		else
			url = MUrl(mDir2);
	}
	
	if (ChooseOneFile(url))
	{
		if (url.IsLocal() and is_directory(url.GetPath()))
			SetDirectory(inFileNr, url.GetPath());
		else
		{
			MDocument* doc = gApp->OpenOneDocument(url);
			
			if (doc != nil)
				SetDocument(inFileNr, doc);
		}
	}
}

// ----------------------------------------------------------------------------
// DocumentClosed

void MDiffWindow::Document1Closed()
{
	ClearList();
	mDoc1 = nil;
	SetButtonTitle(1, _("File 1"));
}

void MDiffWindow::Document2Closed()
{
	ClearList();
	mDoc2 = nil;
	SetButtonTitle(2, _("File 2"));
}

// ----------------------------------------------------------------------------
// SetDocument

void MDiffWindow::SetDocument(int inDocNr, MDocument* inDocument)
{
	if (inDocNr == 1)
	{
		if (mDoc1 != nil)
			RemoveRoute(eDocument1Closed, mDoc1->eDocumentClosed);
		
		mDoc1 = inDocument;

		if (mDoc1 != nil)
		{
			AddRoute(eDocument1Closed, mDoc1->eDocumentClosed);
			SetButtonTitle(1, inDocument->GetURL().GetFileName());
		}
		else
			SetButtonTitle(1, _("File 1"));
	}
	else
	{
		if (mDoc2 != nil)
			RemoveRoute(eDocument2Closed, mDoc2->eDocumentClosed);
		
		mDoc2 = inDocument;

		if (mDoc2 != nil)
		{
			AddRoute(eDocument2Closed, mDoc2->eDocumentClosed);
			SetButtonTitle(2, inDocument->GetURL().GetFileName());
		}
		else
			SetButtonTitle(2, _("File 2"));
	}
	
	if (mDoc1 != nil and mDoc2 != nil)
	{
		RecalculateDiffs();
		ArrangeWindows();
	}

	Select();
}

// ----------------------------------------------------------------------------
// SetDirectory

void MDiffWindow::SetDirectory(int inDirNr, const MPath& inDir)
{
//	if (mDoc1 != nil)
//		SetDocument(1, nil);
//	
//	if (mDoc2 != nil)
//		SetDocument(2, nil);
//	
//	mListView->RemoveAll();
//	
//	if (inDirNr == 1)
//	{
//		mDir1 = inDir;
//		SetButtonTitle(1, mDir1.leaf());
//	}
//	else
//	{
//		mDir2 = inDir;
//		SetButtonTitle(2, mDir2.leaf());
//	}
//
//	mListView->RemoveAll();
//	mScript.clear();
//	mDScript.clear();
//	
//	if (is_directory(mDir1) and is_directory(mDir2))
//		RecalculateDiffsForDirs();

	Select();
}

// ----------------------------------------------------------------------------
// RecalculateDiffs

void MDiffWindow::RecalculateDiffs()
{
	GtkWidget* treeView = GetWidget(kListViewID);
	GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)));
	
	int32 selected = GetSelectedRow();

	ClearList();
	
	if (mDoc1 == nil or mDoc2 == nil)
		return;
	
	vector<uint32> ha;	mDoc1->HashLines(ha);
	vector<uint32> hb;	mDoc2->HashLines(hb);
	
	MDiff diff(ha, hb);
	
	diff.Report(mScript);
	
	for (MDiffScript::iterator diff = mScript.begin(); diff != mScript.end(); ++diff)
	{
		string s;
		int n0 = 0, n1 = 0, n2 = 0, n3 = 0;
		
		if (diff->mA1 == diff->mA2)
		{
			if (diff->mB1 < diff->mB2 - 1)
			{
				s = _("Extra lines in file ^0: ^1-^2");
				n0 = 2;
				n1 = diff->mB1 + 1;
				n2 = diff->mB2;
			}
			else
			{
				s = _("Extra line in file ^0: ^1");
				n0 = 2;
				n1 = diff->mB1 + 1;
			}
		}
		else if (diff->mB1 == diff->mB2)
		{
			if (diff->mA1 < diff->mA2 - 1)
			{
				s = _("Extra lines in file ^0: ^1-^2");
				n0 = 1;
				n1 = diff->mA1 + 1;
				n2 = diff->mA2;
			}
			else
			{
				s = _("Extra line in file ^0: ^1");
				n0 = 1;
				n1 = diff->mA1 + 1;
			}
		}
		else
		{
			if (diff->mA1 < diff->mA2 - 1 and diff->mB1 < diff->mB2 - 1)
			{
				s = _("Nonmatching lines. File 1: ^0-^1, File 2: ^2-^3");
				n0 = diff->mA1 + 1;
				n1 = diff->mA2;
				n2 = diff->mB1 + 1;
				n3 = diff->mB2;
			}
			else if (diff->mB1 < diff->mB2 - 1)
			{
				s = _("Nonmatching lines. File 1: ^0, File 2: ^1-^2");
				n0 = diff->mA1 + 1;
				n1 = diff->mB1 + 1;
				n2 = diff->mB2;
			}
			else if (diff->mA1 < diff->mA2 - 1)
			{
				s = _("Nonmatching lines. File 1: ^0-^1, File 2: ^2");
				n0 = diff->mA1 + 1;
				n1 = diff->mA2;
				n2 = diff->mB1 + 1;
			}
			else
			{
				s = _("Nonmatching lines. File 1: ^0, File 2: ^1");
				n0 = diff->mA1 + 1;
				n1 = diff->mB1 + 1;
			}
		}
		
		string::size_type p;
		
		if ((p = s.find("^0")) != string::npos)
			s.replace(p, 2, NumToString(n0));
		if ((p = s.find("^1")) != string::npos)
			s.replace(p, 2, NumToString(n1));
		if ((p = s.find("^2")) != string::npos)
			s.replace(p, 2, NumToString(n2));
		if ((p = s.find("^3")) != string::npos)
			s.replace(p, 2, NumToString(n3));
		
		GtkTreeIter iter;
		gtk_list_store_append(GTK_LIST_STORE(store), &iter);  /* Acquire an iterator */
		gtk_list_store_set(GTK_LIST_STORE(store), &iter, 0, s.c_str(), -1);
	}
	
	if (selected >= 0)
		SelectRow(selected);
}

// ----------------------------------------------------------------------------
// RecalculateDiffsForDirs

void MDiffWindow::RecalculateDiffsForDirs()
{
//	mDScript.clear();
//	mListView->RemoveAll();
//	
//	vector<MPath> a, b;
//	MPath p;
//	
//	MFileIterator iter_a(mDir1, 0);
//	while (iter_a.Next(p))
//		a.push_back(p);
//	
//	MFileIterator iter_b(mDir2, 0);
//	while (iter_b.Next(p))
//		b.push_back(p);
//	
//	sort(a.begin(), a.end());
//	sort(b.begin(), b.end());
//	
//	vector<MPath>::iterator ai, bi;
//	ai = a.begin();
//	bi = b.begin();
//	
//	while (ai != a.end() and bi != b.end())
//	{
//		if (ai->leaf() == bi->leaf())
//		{
//			if (FilesDiffer(*ai, *bi))
//				AddDirDiff(ai->leaf(), 0);
//
//			++ai;
//			++bi;
//		}
//		else
//		{
//			if (ai->leaf() < bi->leaf())
//			{
//				AddDirDiff(ai->leaf(), 1);
//				++ai;
//			}
//			else
//			{
//				AddDirDiff(bi->leaf(), 2);
//				++bi;
//			}
//		}
//	}
//	
//	while (ai != a.end())
//	{
//		AddDirDiff(ai->leaf(), 1);
//		++ai;
//	}
//	
//	while (bi != b.end())
//	{
//		AddDirDiff(bi->leaf(), 2);
//		++bi;
//	}
//
//	SetCallBack(mListView->cbDrawItem, this, &MDiffWindow::DrawDirItem);
//	SetCallBack(mListView->cbRowSelected, this, &MDiffWindow::FileSelected);
}

// ----------------------------------------------------------------------------
// MDiffWindow::AddDirDiff

void MDiffWindow::AddDirDiff(const std::string& inName, uint32 inStatus)
{
//	MDirDiffItem dItem = { inName, inStatus };
//	mDScript.push_back(dItem);
//	mListView->InsertItem(0, "", 0);
}

// ----------------------------------------------------------------------------
//	MDiffWindow::FilesDiffer

bool MDiffWindow::FilesDiffer(const MUrl& inA, const MUrl& inB) const
{
	vector<uint32> a, b;
	
	MDocument* doc;
	
	if ((doc = MDocument::GetDocumentForURL(inA, false)) != nil)
		doc->HashLines(a);
	else
	{
		auto_ptr<MDocument> d(new MDocument(&inA));
		d->HashLines(a);
	}
	
	if ((doc = MDocument::GetDocumentForURL(inB, false)) != nil)
		doc->HashLines(b);
	else
	{
		auto_ptr<MDocument> d(new MDocument(&inB));
		d->HashLines(b);
	}
	
	if (a.back() != b.back())
	{
		if (a.back() != 0) a.push_back(0);
		if (b.back() != 0) b.push_back(0);
	}
	
	return a != b;
}

// ----------------------------------------------------------------------------
// MDiffWindow::SetButtonTitle

void MDiffWindow::SetButtonTitle(int inButtonNr, const std::string& inTitle)
{
	if (inButtonNr == 1)
		SetText(kChooseFile1Command, inTitle);
	else if (inButtonNr == 2)
		SetText(kChooseFile2Command, inTitle);
}

// ----------------------------------------------------------------------------
// MDiffWindow::DiffSelected

void MDiffWindow::DiffSelected()
{
	int32 selected = GetSelectedRow();

	if (selected >= 0 and selected < (int32)mScript.size())
	{
		MDiffInfo diff = mScript[selected];
		
		mDoc1->Select(mDoc1->LineStart(diff.mA1), mDoc1->LineStart(diff.mA2), kScrollForDiff);
		mDoc2->Select(mDoc2->LineStart(diff.mB1), mDoc2->LineStart(diff.mB2), kScrollForDiff);
	}
}

// ----------------------------------------------------------------------------
// MDiffWindow::FileSelected

void MDiffWindow::FileSelected(uint32 inDiffNr)
{
//	MDirDiffItem diff = mDScript[inDiffNr];
//	
//	switch (diff.status)
//	{
//		case 0:
//		{
//			auto_ptr<MDiffWindow> w(new MDiffWindow);
//			
//			w->Initialize();
//			w->Show();
//			
//			w->SetDocument(1, MApplication::Instance().OpenOneDocument(mDir1 / diff.name));
//			w->SetDocument(2, MApplication::Instance().OpenOneDocument(mDir2 / diff.name));
//			
//			w.release();
//			break;
//		}
//		
//		case 1:
//			MApplication::Instance().OpenOneDocument(mDir1 / diff.name);
//			break;
//		
//		case 2:
//			MApplication::Instance().OpenOneDocument(mDir2 / diff.name);
//			break;
//	}
}

// ----------------------------------------------------------------------------
// MDiffWindow::MergeToFile

void MDiffWindow::MergeToFile(int inFileNr)
{
	int32 selected = GetSelectedRow();
	
	if (selected < 0)
		return;
	
	MDiffInfo diff = mScript[selected];
	
	mDoc1->Select(mDoc1->LineStart(diff.mA1), mDoc1->LineStart(diff.mA2), kScrollForDiff);
	mDoc2->Select(mDoc2->LineStart(diff.mB1), mDoc2->LineStart(diff.mB2), kScrollForDiff);
	
	MDocument* srcDoc = mDoc2;
	MDocument* dstDoc = mDoc1;
	
	if (inFileNr == 2)
		swap(srcDoc, dstDoc);
	
	string txt;
	srcDoc->GetSelectedText(txt);
	
	dstDoc->StartAction("Merge");
	dstDoc->ReplaceSelectedText(txt);
	dstDoc->FinishAction();
	
	int32 d;
	
	if (inFileNr == 1)
		d = (diff.mB2 - diff.mB1) - (diff.mA2 - diff.mA1);
	else
		d = (diff.mA2 - diff.mA1) - (diff.mB2 - diff.mB1);

	MDiffScript::iterator di = mScript.begin() + selected;
	di = mScript.erase(di);

	for (; di != mScript.end(); ++di)
	{
		if (inFileNr == 1)
		{
			di->mA1 += d;
			di->mA2 += d;
		}
		else
		{
			di->mB1 += d;
			di->mB2 += d;
		}
	}
	
	RemoveRow(selected);
}

// ----------------------------------------------------------------------------
// MDiffWindow::FocusChanged

void MDiffWindow::FocusChanged(
	uint32		inID)
{
	if (mDoc1 != nil and mDoc2 != nil)
		RecalculateDiffs();
}

// ----------------------------------------------------------------------------
// MDiffWindow::ArrangeWindows

void MDiffWindow::ArrangeWindows()
{
	THROW_IF_NIL(mDoc1);
	
	MDocWindow* w = MDocWindow::DisplayDocument(mDoc1);
	THROW_IF_NIL(w);
	
	MRect wRect;
	w->GetMaxPosition(wRect);

	// gebruik normale afmetingen
	
	const int32
//		kMaxWidth = 1024, kMaxHeight = 768,
		kDiffWindowHeight = 175,
		kGapWidth = 10;
	
//	if (wRect.width > kMaxWidth)
//	{
//		int32 dx = (wRect.width - kMaxWidth) / 2;
//		wRect.InsetBy(dx, 0);
//	}
//	
//	if (wRect.height > kMaxHeight)
//	{
//		int32 dy = (wRect.height - kMaxHeight) / 2;
//		wRect.InsetBy(0, dy);
//	}
	
	wRect.InsetBy(20 + kGapWidth, 20 + kGapWidth);
	
	MRect diffWindowBounds, targetWindow1Bounds, targetWindow2Bounds, temp;

	diffWindowBounds = wRect;
	diffWindowBounds.height = kDiffWindowHeight;
	diffWindowBounds.y += (wRect.height - kDiffWindowHeight);
	diffWindowBounds.InsetBy(kGapWidth, kGapWidth);
	
	wRect.height -= kDiffWindowHeight + kGapWidth;
	
	targetWindow1Bounds = wRect;
	targetWindow1Bounds.width /= 2;
	targetWindow1Bounds.InsetBy(kGapWidth, kGapWidth);
	
	targetWindow2Bounds = wRect;
	targetWindow2Bounds.x += wRect.width / 2;
	targetWindow2Bounds.width -= wRect.width / 2;
	targetWindow2Bounds.InsetBy(kGapWidth, kGapWidth);
	
	w = MDocWindow::DisplayDocument(mDoc1);
	w->SetWindowPosition(targetWindow1Bounds);
	
	w = MDocWindow::DisplayDocument(mDoc2);
	w->SetWindowPosition(targetWindow2Bounds);
	
	SetWindowPosition(diffWindowBounds);

//	::CGRectDivide(wRect, &diffWindowBounds, &temp, kDiffWindowHeight, CGRectMaxYEdge);
//	::CGRectDivide(temp, &targetWindow1Bounds, &targetWindow2Bounds, wRect.size.width / 2, CGRectMinXEdge);
//	
//	diffWindowBounds = ::CGRectInset(diffWindowBounds, kGapWidth / 2, kGapWidth / 2);
//	targetWindow1Bounds = ::CGRectInset(targetWindow1Bounds, kGapWidth / 2, kGapWidth / 2);
//	targetWindow2Bounds = ::CGRectInset(targetWindow2Bounds, kGapWidth / 2, kGapWidth / 2);
//	
//	THROW_IF_OSERROR(::TransitionWindowWithOptions(
//		GetSysWindow(), kWindowSlideTransitionEffect, kWindowResizeTransitionAction,
//		&diffWindowBounds, true, nil));
//	
//	w = MDocWindow::DisplayDocument(mDoc1);
//	
//	THROW_IF_OSERROR(::TransitionWindowWithOptions(
//		w->GetSysWindow(), kWindowSlideTransitionEffect, kWindowResizeTransitionAction,
//		&targetWindow1Bounds, true, nil));
//	
//	w = MDocWindow::DisplayDocument(mDoc2);
//	
//	THROW_IF_OSERROR(::TransitionWindowWithOptions(
//		w->GetSysWindow(), kWindowSlideTransitionEffect, kWindowResizeTransitionAction,
//		&targetWindow2Bounds, true, nil));
}

// ----------------------------------------------------------------------------
// MDiffWindow::ClearList

void MDiffWindow::ClearList()
{
	GtkWidget* treeView = GetWidget(kListViewID);
	GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)));
	gtk_list_store_clear(store);

	mScript.clear();
}

// ----------------------------------------------------------------------------
// MDiffWindow::GetSelectedRow

int32 MDiffWindow::GetSelectedRow() const
{
	int32 result = -1;
	
	GtkWidget* treeView = GetWidget(kListViewID);
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));

	GList* selected = gtk_tree_selection_get_selected_rows(selection, nil);
	if (selected != nil)
	{
		char* path = gtk_tree_path_to_string((GtkTreePath*)(selected->data));
		result = atoi(path);
		g_list_foreach(selected, (GFunc)gtk_tree_path_free, nil);
		g_list_free(selected);
	}
	
	return result;
}
	
// ----------------------------------------------------------------------------
// MDiffWindow::SelectRow

void MDiffWindow::SelectRow(
	int32			inRow)
{
	GtkWidget* treeView = GetWidget(kListViewID);
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
	
	GtkTreePath* path = gtk_tree_path_new_from_indices(inRow, -1);
	THROW_IF_NIL(path);
	
	gtk_tree_selection_select_path(selection, path);
	gtk_tree_path_free(path);
}

// ----------------------------------------------------------------------------
// MDiffWindow::RemoveRow

void MDiffWindow::RemoveRow(
	int32			inRow)
{
	GtkWidget* treeView = GetWidget(kListViewID);
	GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)));
	
	GtkTreePath* path = gtk_tree_path_new_from_indices(inRow, -1);
	THROW_IF_NIL(path);
	
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
	gtk_tree_path_free(path);
	
	if (gtk_list_store_remove(GTK_LIST_STORE(store), &iter))
	{
		GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));

		path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
		THROW_IF_NIL(path);
		gtk_tree_selection_select_path(selection, path);
		gtk_tree_path_free(path);
	}
}

