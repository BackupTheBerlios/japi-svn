//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include "MWinListViewImpl.h"
#include "MWinControlsImpl.h"
#include "MWinUtils.h"

using namespace std;

// --------------------------------------------------------------------

MWinListViewImpl::MWinListViewImpl(
	MListView*		inListView)
	: MListViewImpl(inListView)
	, eColumnResized(this, &MWinListViewImpl::ColumnResized)
{
	MRect bounds;
	mListView->GetBounds(bounds);
	bounds.height = 16;

	mListHeader = new MListHeader("header", bounds);
	mListView->AddChild(mListHeader);

	AddRoute(eColumnResized, mListHeader->eColumnResized);
}

MWinListViewImpl::~MWinListViewImpl()
{
	delete mListHeader;
}

void MWinListViewImpl::AppendColumn(
	const string&	inLabel,
	int				inWidth)
{
	mListHeader->AppendColumn(inLabel, inWidth);
	mColumnWidths.push_back(inWidth);
}

void MWinListViewImpl::ColumnResized(
	uint32			inColumnNr,
	uint32			inColumnWidth)
{
	if (inColumnNr < mColumnWidths.size())
		mColumnWidths[inColumnNr] = inColumnWidth;
}

MListViewImpl* MListViewImpl::Create(MListView* inListView)
{
	return new MWinListViewImpl(inListView);
}
