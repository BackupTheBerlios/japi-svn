//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <sstream>

#include "MFile.h"
#include "MTextDocument.h"
#include "MDiffWindow.h"
#include "MGlobals.h"
#include "MUtils.h"
#include "MMenu.h"
#include "MDevice.h"
#include "MError.h"
#include "MDocWindow.h"
#include "MStrings.h"
#include "MJapiApp.h"

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
	MTextDocument*		inDocument)
	: MDialog("diff-window")
	, eDocumentClosed(this, &MDiffWindow::DocumentClosed)
	, mSelected(this, &MDiffWindow::DiffSelected)
	, mDoc1(nil)
	, mDoc2(nil)
	, mDir1Inited(false)
	, mDir2Inited(false)
	, mInvokeRow(this, &MDiffWindow::InvokeRow)
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
	mInvokeRow.Connect(G_OBJECT(treeView), "row-activated");
	
	if (inDocument != nil /*and inDocument->IsSpecified()*/)
		SetDocument(1, inDocument);
}

MDiffWindow::~MDiffWindow()
{
}

void MDiffWindow::ValueChanged(
	uint32			inID)
{
	ProcessCommand(inID, nil, 0, 0);
}

bool MDiffWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
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
			result = MDialog::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
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

// ----------------------------------------------------------------------------
// MDiffWindow::ChooseFile

void MDiffWindow::ChooseFile(int inFileNr)
{
	string currentFolder = gApp->GetCurrentFolder();
	MFile url;
	
	if (inFileNr == 1)
	{
		if (mDoc1 != nil)
			url = mDoc1->GetFile();
		else if (mDir1Inited)
			url = mDir1;
	}
	else if (inFileNr == 2)
	{
		if (mDoc2 != nil)
			url = mDoc2->GetFile();
		else if (mDir2Inited)
			url = mDir2;
	}

	try
	{
		uint32 modifiers = GetModifiers();

		if (modifiers != 0)
		{
			fs::path dir = url.GetPath();
			
			if (ChooseDirectory(dir) and fs::is_directory(dir))
				SetDirectory(inFileNr, dir);
			else
				gApp->SetCurrentFolder(currentFolder);
		}
		else if (ChooseOneFile(url))
		{
			MTextDocument* doc = dynamic_cast<MTextDocument*>(
				gApp->OpenOneDocument(url));
			
			if (doc != nil)
				SetDocument(inFileNr, doc);
		}
		else
			gApp->SetCurrentFolder(currentFolder);
	}
	catch (...)
	{
		gApp->SetCurrentFolder(currentFolder);
		throw;
	}
}

// ----------------------------------------------------------------------------
// DocumentClosed

void MDiffWindow::DocumentClosed(
	MDocument*		inDocument)
{
	ClearList();

	if (inDocument == mDoc1)
	{
		mDoc1 = nil;
		SetButtonTitle(1, _("File 1"));
	}
	else if (inDocument == mDoc2)
	{
		mDoc2 = nil;
		SetButtonTitle(2, _("File 2"));
	}
}

// ----------------------------------------------------------------------------
// SetDocument

void MDiffWindow::SetDocument(int inDocNr, MTextDocument* inDocument)
{
	if (inDocNr == 1)
	{
		if (mDoc1 != nil)
			RemoveRoute(eDocumentClosed, mDoc1->eDocumentClosed);
		
		mDoc1 = inDocument;

		if (mDoc1 != nil)
		{
			AddRoute(eDocumentClosed, mDoc1->eDocumentClosed);
			SetButtonTitle(1, inDocument->GetFile().GetFileName());
		}
		else
			SetButtonTitle(1, _("File 1"));
	}
	else
	{
		if (mDoc2 != nil)
			RemoveRoute(eDocumentClosed, mDoc2->eDocumentClosed);
		
		mDoc2 = inDocument;

		if (mDoc2 != nil)
		{
			AddRoute(eDocumentClosed, mDoc2->eDocumentClosed);
			SetButtonTitle(2, inDocument->GetFile().GetFileName());
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

void MDiffWindow::SetDirectory(
	int				inDirNr,
	const fs::path&	inDir)
{
	if (mDoc1 != nil)
		SetDocument(1, nil);
	
	if (mDoc2 != nil)
		SetDocument(2, nil);
	
	ClearList();
	
	if (inDirNr == 1)
	{
		mDir1 = inDir;
		mDir1Inited = true;
		SetButtonTitle(1, mDir1.leaf());
	}
	else
	{
		mDir2 = inDir;
		mDir2Inited = true;
		SetButtonTitle(2, mDir2.leaf());
	}
	
	if (fs::exists(mDir1) and is_directory(mDir1) and
		fs::exists(mDir2) and is_directory(mDir2))
	{
		RecalculateDiffsForDirs();
	}

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
	mDScript.clear();

	ClearList();

	vector<fs::path> a, b;
	fs::path p;
	
	MFileIterator iter_a(mDir1, 0);
	while (iter_a.Next(p))
		a.push_back(p);
	
	MFileIterator iter_b(mDir2, 0);
	while (iter_b.Next(p))
		b.push_back(p);
	
	sort(a.begin(), a.end());
	sort(b.begin(), b.end());
	
	vector<fs::path>::iterator ai, bi;
	ai = a.begin();
	bi = b.begin();
	
	while (ai != a.end() and bi != b.end())
	{
		if (ai->leaf() == bi->leaf())
		{
			if (FilesDiffer(MFile(*ai), MFile(*bi)))
				AddDirDiff(ai->leaf(), 0);

			++ai;
			++bi;
		}
		else
		{
			if (ai->leaf() < bi->leaf())
			{
				AddDirDiff(ai->leaf(), 1);
				++ai;
			}
			else
			{
				AddDirDiff(bi->leaf(), 2);
				++bi;
			}
		}
	}
	
	while (ai != a.end())
	{
		AddDirDiff(ai->leaf(), 1);
		++ai;
	}
	
	while (bi != b.end())
	{
		AddDirDiff(bi->leaf(), 2);
		++bi;
	}
}

// ----------------------------------------------------------------------------
// MDiffWindow::AddDirDiff

void MDiffWindow::AddDirDiff(
	const string&	inName,
	uint32			inStatus)
{
	MDirDiffItem dItem = { inName, inStatus };
	mDScript.push_back(dItem);

	string s;
	
	if (inStatus == 0)
		s = inName;
	else
	{
		s = _("File '^0' only in dir ^1");
		s.replace(s.find("^0"), 2, inName);
		s.replace(s.find("^1"), 2, NumToString(inStatus));
	}
	
	GtkWidget* treeView = GetWidget(kListViewID);
	GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)));
	
	GtkTreeIter iter;
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);  /* Acquire an iterator */
	gtk_list_store_set(GTK_LIST_STORE(store), &iter, 0, s.c_str(), -1);
}

// ----------------------------------------------------------------------------
//	MDiffWindow::FilesDiffer

bool MDiffWindow::FilesDiffer(
	const MFile&		inA,
	const MFile&		inB) const
{
	vector<uint32> a, b;
	
	MTextDocument* doc;
	
	if ((doc = dynamic_cast<MTextDocument*>(MDocument::GetDocumentForFile(inA))) != nil)
		doc->HashLines(a);
	else
	{
		unique_ptr<MTextDocument> d(MDocument::Create<MTextDocument>(inA));
		d->HashLines(a);
	}
	
	if ((doc = dynamic_cast<MTextDocument*>(MDocument::GetDocumentForFile(inB))) != nil)
		doc->HashLines(b);
	else
	{
		unique_ptr<MTextDocument> d(MDocument::Create<MTextDocument>(inB));
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
// MDiffWindow::DiffInvoked

void MDiffWindow::DiffInvoked(
	int32		inRow)
{
	if (inRow >= 0 and inRow < (int32)mDScript.size())
	{
		MDirDiffItem diff = mDScript[inRow];
		
		switch (diff.status)
		{
			case 0:
			{
				unique_ptr<MDiffWindow> w(new MDiffWindow);
				
				w->SetDocument(1,
					dynamic_cast<MTextDocument*>(gApp->OpenOneDocument(MFile(mDir1 / diff.name))));
				w->SetDocument(2,
					dynamic_cast<MTextDocument*>(gApp->OpenOneDocument(MFile(mDir2 / diff.name))));
				
				w->Select();
				
				w.release();
				break;
			}
			
			case 1:
				gApp->OpenOneDocument(MFile(mDir1 / diff.name));
				break;
			
			case 2:
				gApp->OpenOneDocument(MFile(mDir2 / diff.name));
				break;
		}
	}
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
	
	MTextDocument* srcDoc = mDoc2;
	MTextDocument* dstDoc = mDoc1;
	
	if (inFileNr == 2)
		swap(srcDoc, dstDoc);
	
	string txt;
	srcDoc->GetSelectedText(txt);
	
	dstDoc->StartAction("Merge");
	dstDoc->ReplaceSelectedText(txt, false, true);
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

	MDocWindow* w;
	MRect wRect;

	GetMaxPosition(wRect);

	const int32
		kDiffWindowHeight = (175 * wRect.height) / 1024,
		kGapWidth = 10;
	
	wRect.InsetBy(kGapWidth, kGapWidth);
	wRect.height -= 2 * kGapWidth;
	
	MRect diffWindowBounds, targetWindow1Bounds, targetWindow2Bounds;

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
	
	w = gApp->DisplayDocument(mDoc1);
	THROW_IF_NIL(w);
	w->SetWindowPosition(targetWindow1Bounds, true);
	
	w = gApp->DisplayDocument(mDoc2);
	THROW_IF_NIL(w);
	w->SetWindowPosition(targetWindow2Bounds, true);
	
	SetWindowPosition(diffWindowBounds, true);
}

// ----------------------------------------------------------------------------
// MDiffWindow::ClearList

void MDiffWindow::ClearList()
{
	GtkWidget* treeView = GetWidget(kListViewID);
	GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)));
	gtk_list_store_clear(store);

	mScript.clear();
	mDScript.clear();
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
// MDiffWindow::InvokeRow

void MDiffWindow::InvokeRow(
	GtkTreePath*		inPath,
	GtkTreeViewColumn*	inColumn)
{
	int32 item = -1;

	char* path = gtk_tree_path_to_string(inPath);
	if (path != nil)
	{
		item = atoi(path);
		g_free(path);
	}
	
	try
	{
		DiffInvoked(item);
	}
	catch (...) {}
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

