/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 21:25:10
*/

#include "MJapieG.h"

#include "MViewPort.h"
#include "MScrollBar.h"

using namespace std;

MViewPort::MViewPort(
	MScrollBar*		inHScrollBar,
	MScrollBar*		inVScrollBar)
	: mHScrollBar(inHScrollBar)
	, mVScrollBar(inVScrollBar)
{
	GtkAdjustment* hAdj = nil;
	if (inHScrollBar)
		hAdj = inHScrollBar->GetAdjustment();
	
	GtkAdjustment* vAdj = nil;
	if (inVScrollBar)
		vAdj = inVScrollBar->GetAdjustment();
	
	SetWidget(gtk_viewport_new(hAdj, vAdj), false);
		
	mConfigureEvent.Connect(GetGtkWidget(), "configure-event");
}

void MViewPort::SetShadowType(
	GtkShadowType	inShadowType)
{
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(GetGtkWidget()), inShadowType);
}

bool MViewPort::OnConfigureEvent(
	GdkEventConfigure*	inEvent)
{
	eBoundsChanged();
	return false;
}
