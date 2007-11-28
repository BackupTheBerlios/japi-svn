#include "MJapieG.h"

#include "MResources.h"
#include "MAlerts.h"

using namespace std;

int32 DisplayAlert(
		const char*		inResourceName)
{
	int32 result = -1;
	GtkBuilder* builder = gtk_builder_new();
	GError* err = nil;
	
	try
	{
		const char* xml;
		uint32 size;
		
		if (not LoadResource(inResourceName, xml, size))
			THROW(("Could not load resource %s", inResourceName));
		
		if (gtk_builder_add_from_string(builder, xml, size, &err) == 0)
			THROW(("Error building alert: %s", err->message));
		
		GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(builder, "alert"));
		
		result = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);	
	}
	catch (exception& e)
	{
		MError::DisplayError(e);
	}
	
	g_object_unref(builder);

	if (err != nil)
		g_error_free(err);
	
	return result;
}
