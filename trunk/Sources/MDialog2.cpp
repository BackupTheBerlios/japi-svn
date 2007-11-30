#include "MJapieG.h"

#include "MDialog2.h"
#include "MResources.h"

void MDialog2::CreateGladeAndWidgets(
	const char*		inResource,
	GladeXML*&		outGlade,
	GtkWidget*&		outWidget)
{
	const char* xml;
	uint32 size;
	
	if (not LoadResource(inResource, xml, size))
		THROW(("Could not load dialog resource %s", inResource));
	
	outGlade = glade_xml_new_from_buffer(xml, size, nil, nil);
	if (outGlade == nil)
		THROW(("Failed to create glade from resource"));
	
	outWidget = glade_xml_get_widget(outGlade, "dialog");
	if (outWidget == nil)
		THROW(("Failed to extract root widget from glade ('dialog')"));
}

MDialog2::MDialog2(
	GladeXML*		inGlade,
	GtkWidget*		inRoot)
	: MWindow(inRoot)
	, mGlade(inGlade)
{
}

MDialog2::~MDialog2()
{
	g_object_unref(mGlade);
}

void MDialog2::Init()
{
	gtk_widget_show_all(GetGtkWidget());

	SetChecked('wrap', true);
}

const char* MDialog2::IDToName(
	uint32			inID,
	char			inName[5]) const
{
	inName[4] = 0;
	inName[3] = inID & 0x000000ff; inID >>= 8;
	inName[2] = inID & 0x000000ff; inID >>= 8;
	inName[1] = inID & 0x000000ff; inID >>= 8;
	inName[0] = inID & 0x000000ff;
	
	return inName;
}

bool MDialog2::IsChecked(
	uint32				inID) const
{
	char name[5];
	GtkWidget* wdgt = glade_xml_get_widget(mGlade, IDToName(inID, name));
	if (wdgt == nil or not GTK_IS_TOGGLE_BUTTON(wdgt))
		THROW(("Widget '%s' does not exists or is of the incorrect type", name));
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wdgt));
}

void MDialog2::SetChecked(
	uint32				inID,
	bool				inOn)
{
	char name[5];
	GtkWidget* wdgt = glade_xml_get_widget(mGlade, IDToName(inID, name));
	if (wdgt == nil or not GTK_IS_TOGGLE_BUTTON(wdgt))
		THROW(("Widget '%s' does not exists or is of the incorrect type", name));
	return gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wdgt), inOn);
}	
