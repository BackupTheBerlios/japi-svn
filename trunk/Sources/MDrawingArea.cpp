/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 21:17:30
*/

#include "MJapieG.h"

#include <iostream>

#include "MDrawingArea.h"
#include "MDevice.h"
#include "MFont.h"

using namespace std;

MDrawingArea::MDrawingArea(
	uint32 inWidth, uint32 inHeight)
	: MView(gtk_drawing_area_new(), true)
	, mExposeEvent(this, &MDrawingArea::OnExposeEvent)
	, mPangoContext(pango_cairo_font_map_create_context(
		PANGO_CAIRO_FONT_MAP(pango_cairo_font_map_get_default())))
{
	mExposeEvent.Connect(GetGtkWidget(), "expose-event");

	pango_context_set_font_description(mPangoContext, kFixedFont);
	
	gtk_widget_set_size_request(GetGtkWidget(), inWidth, inHeight);
}

MDrawingArea::~MDrawingArea()
{
	g_object_unref(mPangoContext);
}

bool MDrawingArea::OnExposeEvent(
	GdkEventExpose*	inEvent)
{
	MDevice dev(this, MRect(inEvent->area));
	
	dev.EraseRect(inEvent->area);

	uint32 ascent = dev.GetAscent();
	uint32 descent = dev.GetDescent();
	uint32 lineheight = ascent + descent;

	dev.SetForeColor(kMarkedLineColor);
	dev.CreateAndUsePattern(kMarkedLineColor, kCurrentLineColor);

	MRect r1(0, 0, inEvent->area.width, lineheight);
	
	r1.x += lineheight;
	
	dev.FillRect(r1);
	
	MRect r2(r1);
	r2.x -= r2.height / 2;
	r2.width = r2.height;
	
	dev.FillEllipse(r2);

	// teken nu wat tekst

	float x = lineheight + 4;
	float y = 0;

	dev.SetForeColor(kBlack);

	for (uint32 lineNr = 0; lineNr < 10; ++lineNr)
	{
		dev.DrawString("Aap noot mies", x, y);
		y += lineheight;
	}
	
	return true;
}

