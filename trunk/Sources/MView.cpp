/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:18:30
*/

#include "MJapieG.h"

#include <iostream>
#include <cassert>

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

void MView::ResizeTo(
	uint32			inWidth,
	uint32			inHeight)
{
	gtk_widget_set_size_request(GetGtkWidget(), inWidth, inHeight);
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

void MView::Add(
	MView*			inSubView)
{
	assert(GTK_IS_CONTAINER(GetGtkWidget()));
	gtk_container_add(GTK_CONTAINER(GetGtkWidget()), inSubView->GetGtkWidget());
}

