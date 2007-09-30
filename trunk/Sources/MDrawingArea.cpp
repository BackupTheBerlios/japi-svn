/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 21:17:30
*/

#include "MJapieG.h"

#include <iostream>

#include "MDrawingArea.h"
#include "MDevice.h"

using namespace std;

MDrawingArea::MDrawingArea(
	uint32 inWidth, uint32 inHeight)
	: MView(gtk_drawing_area_new())
	, mExposeEvent(this, &MDrawingArea::OnExposeEvent)
{
	mExposeEvent.Connect(GetGtkWidget(), "expose-event");
	
	gtk_widget_set_size_request(GetGtkWidget(), inWidth, inHeight);
}

MDrawingArea::~MDrawingArea()
{
}

bool MDrawingArea::OnExposeEvent(
	GdkEventExpose*	inEvent)
{
	MDevice dev(this, MRect(inEvent->area));
	
	dev.EraseRect(inEvent->area);

	dev.SetForeColor(kMarkedLineColor);
	dev.CreateAndUsePattern(kMarkedLineColor, kCurrentLineColor);

	MRect r1(0, 0, inEvent->area.width, 12);
	
	r1.x += 12;
	
	dev.FillRect(r1);
	
	MRect r2(r1);
	r2.x -= r2.height / 2;
	r2.width = r2.height;
	
	dev.FillEllipse(r2);
	
	return true;
}

