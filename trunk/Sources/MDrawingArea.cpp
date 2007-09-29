/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 21:17:30
*/

#include "MJapieG.h"

#include <iostream>

#include "MDrawingArea.h"

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
	cout << "Drawinging in: "
		 << inEvent->area.x << ", " << inEvent->area.y
		 << " - "
		 << inEvent->area.width << ", " << inEvent->area.height
		 << endl;

	// This is where we draw on the window
	if (GetGtkWidget()->window)
	{
		GtkAllocation allocation = GetGtkWidget()->allocation;
		const int width = allocation.width;
		const int height = allocation.height;
		
		// coordinates for the center of the window
		int xc, yc;
		xc = width / 2;
		yc = height / 2;
		
		cairo_t* cr = gdk_cairo_create(GetGtkWidget()->window);
		
		cairo_set_line_width(cr, 10.0);
		
		// clip to the area indicated by the expose event so that we only redraw
		// the portion of the window that needs to be redrawn
		cairo_rectangle(cr, inEvent->area.x, inEvent->area.y,
		        inEvent->area.width, inEvent->area.height);
		cairo_clip(cr);
		
		// draw red lines out from the center of the window
		cairo_set_source_rgb(cr, 0.8, 0.0, 0.0);
		cairo_move_to(cr, 0, 0);
		cairo_line_to(cr, xc, yc);
		cairo_line_to(cr, 0, height);
		cairo_move_to(cr, xc, yc);
		cairo_line_to(cr, width, yc);
		cairo_stroke(cr);
		
		cairo_destroy(cr);
	}
	
	return true;
}

