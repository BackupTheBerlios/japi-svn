#include "MJapieG.h"

#include <sstream>

#include "MDialog2.h"
#include "MResources.h"
#include "MPreferences.h"

using namespace std;

MDialog2* MDialog2::sFirst;

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
	, mParentWindow(nil)
	, mNext(nil)
	, mCloseImmediatelyOnOK(true)
{
	mNext = sFirst;
	sFirst = this;

	glade_xml_signal_connect_data(mGlade, "on_changed", 
		G_CALLBACK(&MDialog2::ChangedCallBack), this);
}

MDialog2::~MDialog2()
{
	if (sFirst == this)
		sFirst = mNext;
	else
	{
		MDialog2* dlog = sFirst;
		while (dlog->mNext != nil)
		{
			if (dlog->mNext == this)
			{
				dlog->mNext = mNext;
				break;
			}
			dlog = dlog->mNext;
		}
	}

	g_object_unref(mGlade);
}

void MDialog2::Init()
{
	gtk_widget_show_all(GetGtkWidget());

	SetChecked('wrap', true);
}

void MDialog2::Show(
	MWindow*		inParent)
{
	if (inParent != nil)
	{
		gtk_window_set_transient_for(
			GTK_WINDOW(GetGtkWidget()),
			GTK_WINDOW(inParent->GetGtkWidget()));
	}
	
	MWindow::Show();
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

GtkWidget* MDialog2::GetWidget(
	uint32				inID) const
{
	char name[5];
	GtkWidget* wdgt = glade_xml_get_widget(mGlade, IDToName(inID, name));
	if (wdgt == nil)
		THROW(("Widget '%s' does not exist", name));
	return wdgt;
}

void MDialog2::SetFocus(
	uint32				inID)
{
	gtk_widget_grab_focus(GetWidget(inID));
}

void MDialog2::GetText(
	uint32				inID,
	std::string&		outText) const
{
	GtkWidget* wdgt = GetWidget(inID);
	if (GTK_IS_COMBO_BOX(wdgt))
		outText = gtk_combo_box_get_active_text(GTK_COMBO_BOX(wdgt));
	else if (GTK_IS_ENTRY(wdgt))
		outText = gtk_entry_get_text(GTK_ENTRY(wdgt));
	else
		THROW(("item is not an entry"));
}

void MDialog2::SetText(
	uint32				inID,
	const std::string&	inText)
{
	GtkWidget* wdgt = GetWidget(inID);
	if (GTK_IS_COMBO_BOX(wdgt))
assert(false);//		gtk_combo_box_set_active_text(GTK_COMBO_BOX(wdgt), inText.c_str());
	else if (GTK_IS_ENTRY(wdgt))
		gtk_entry_set_text(GTK_ENTRY(wdgt), inText.c_str());
	else if (GTK_IS_LABEL(wdgt))
		gtk_label_set_text(GTK_LABEL(wdgt), inText.c_str());
	else if (GTK_IS_PROGRESS_BAR(wdgt))
	{
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(wdgt), inText.c_str());
		gtk_progress_bar_set_ellipsize(GTK_PROGRESS_BAR(wdgt),
			PANGO_ELLIPSIZE_MIDDLE);
	}
	else
		THROW(("item is not an entry"));
}

int32 MDialog2::GetValue(
	uint32				inID) const
{
	int32 result = 0;
	GtkWidget* wdgt = GetWidget(inID);
	
	if (GTK_IS_COMBO_BOX(wdgt))
		result = gtk_combo_box_get_active(GTK_COMBO_BOX(wdgt)) + 1;
	else
		THROW(("Cannot get value"));
		
	return result;
}

void MDialog2::SetValue(
	uint32				inID,
	int32				inValue)
{
	GtkWidget* wdgt = GetWidget(inID);

	if (GTK_IS_COMBO_BOX(wdgt))
		gtk_combo_box_set_active(GTK_COMBO_BOX(wdgt), inValue - 1);
	else
		THROW(("Cannot get value"));
}

// for comboboxes
void MDialog2::GetValues(
	uint32				inID,
	vector<string>& 	outValues) const
{
	assert(false);
}

void MDialog2::SetValues(
	uint32				inID,
	const vector<string>&
						inValues)
{
	GtkWidget* wdgt = GetWidget(inID);

	char name[5];
	if (not GTK_IS_COMBO_BOX(wdgt))
		THROW(("Item %s is not a combo box", IDToName(inID, name)));

	GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(wdgt));
	int32 count = gtk_tree_model_iter_n_children(model, nil);

	while (count-- > 0)
		gtk_combo_box_remove_text(GTK_COMBO_BOX(wdgt), count);

	for (vector<string>::const_iterator s = inValues.begin(); s != inValues.end(); ++s)
		gtk_combo_box_append_text(GTK_COMBO_BOX(wdgt), s->c_str());

	gtk_combo_box_set_active(GTK_COMBO_BOX(wdgt), 0);
}

bool MDialog2::IsChecked(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	if (not GTK_IS_TOGGLE_BUTTON(wdgt))
		THROW(("Widget '%d' is not of the correct type", inID));
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wdgt));
}

void MDialog2::SetChecked(
	uint32				inID,
	bool				inOn)
{
	GtkWidget* wdgt = GetWidget(inID);
	if (not GTK_IS_TOGGLE_BUTTON(wdgt))
		THROW(("Widget '%d' is not of the correct type", inID));
	return gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wdgt), inOn);
}	

bool MDialog2::IsVisible(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	return GTK_WIDGET_VISIBLE(wdgt);
}

void MDialog2::SetVisible(
	uint32				inID,
	bool				inVisible)
{
	GtkWidget* wdgt = GetWidget(inID);
	if (inVisible)
		gtk_widget_show(wdgt);
	else
		gtk_widget_hide(wdgt);
}

bool MDialog2::IsEnabled(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	return GTK_WIDGET_IS_SENSITIVE(wdgt);
}

void MDialog2::SetEnabled(
	uint32				inID,
	bool				inEnabled)
{
	GtkWidget* wdgt = GetWidget(inID);
	gtk_widget_set_sensitive(wdgt, inEnabled);
}

bool MDialog2::IsExpanded(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	assert(GTK_IS_EXPANDER(wdgt));
	return gtk_expander_get_expanded(GTK_EXPANDER(wdgt));
}

void MDialog2::SetExpanded(
	uint32				inID,
	bool				inExpanded)
{
	GtkWidget* wdgt = GetWidget(inID);
	assert(GTK_IS_EXPANDER(wdgt));
	gtk_expander_set_expanded(GTK_EXPANDER(wdgt), inExpanded);
}

void MDialog2::ValueChanged(
	uint32				inID)
{
//	char name[5];
//	cout << "Value Changed for " << IDToName(inID, name) << endl;
}

void MDialog2::ChangedCallBack(
	GtkWidget*			inWidget,
	gpointer			inUserData)
{
	MDialog2* self = reinterpret_cast<MDialog2*>(inUserData);
	
	const char* name = glade_get_widget_name(inWidget);
	if (name != nil)
	{
		uint32 id;
		for (uint32 i = 0; i < 4 and name[i]; ++i)
			id = (id << 8) | name[i];
		
		try
		{
			self->ValueChanged(id);
		}
		catch (exception& e)
		{
			MError::DisplayError(e);
		}
		catch (...) {}
	}
}

void MDialog2::SavePosition(const char* inName)
{
	int x, y;
	gtk_window_get_position(GTK_WINDOW(GetGtkWidget()), &x, &y);
	
	stringstream s;
	s << x << ' ' << y;
	
	Preferences::SetString(inName, s.str());
}

void MDialog2::RestorePosition(const char* inName)
{
	string s = Preferences::GetString(inName, "");
	if (s.length() > 0)
	{
		int x, y;
		
		stringstream ss(s);
		ss >> x >> y;
		
		gtk_window_move(GTK_WINDOW(GetGtkWidget()), x, y);
	}
}

void MDialog2::SetCloseImmediatelyFlag(
	bool inCloseImmediately)
{
	mCloseImmediatelyOnOK = inCloseImmediately;
}
