/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:18:30
*/

#include "MJapieG.h"

#include <iostream>

#include "MView.h"

using namespace std;

MView::MView(
	GtkWidget*		inWidget)
	: mGtkWidget(inWidget)
{
}

MView::~MView()
{
}

MWindow* MView::GetWindow() const
{
	return nil;
}

void MView::GetBounds(
	MRect&			outBounds) const
{
	outBounds = mGtkWidget->allocation;
}

void MView::SetBounds(
	const MRect&	inBounds)
{
}

bool MView::IsActive() const
{
}

void MView::Invalidate()
{
}

void MView::Invalidate(
	const MRect&	inRect)
{
}
