//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <vector>

#include "MResources.h"
#include "MAlerts.h"
#include "MUtils.h"
#include "MStrings.h"
#include "MWindow.h"
#include "MError.h"
#include "MSound.h"

using namespace std;

GtkWidget* CreateAlertWithArgs(
	const char* 	inResourceName,
	vector<string>&	inArgs)
{
	xmlDocPtr xmlDoc = nil;
	GtkWidget* dlg = nil;
	
	string rsrc = string("Alerts/") + inResourceName + ".xml";
	
	try
	{
		xmlInitParser();
	
		const char* xml;
		uint32 size;
		
		if (not LoadResource(rsrc, xml, size))
			THROW(("Could not load resource %s", inResourceName));
		
		xmlDoc = xmlParseMemory(xml, size);
		if (xmlDoc == nil or xmlDoc->children == nil)
			THROW(("Failed to parse project file"));
		
		// build an alert
		
		string text;
		vector<pair<string,uint32> > btns;
		int32 defaultButton = -1;
		GtkMessageType type = GTK_MESSAGE_ERROR;
		
		XMLNode node(xmlDoc->children);
		
		if (node.name() == "alert")
		{
			if (node.property("type") == "warning")
				type = GTK_MESSAGE_WARNING;
			
			for (XMLNode::iterator item = node.begin(); item != node.end(); ++item)
			{
				if (item->name() == "message")
					text = _(item->text());
				else if (item->name() == "buttons")
				{
					for (XMLNode::iterator button = item->begin(); button != item->end(); ++button)
					{
						if (button->name() == "button")
						{
							string label = _(button->property("title"));
							uint32 cmd = atoi(button->property("cmd").c_str());
							if (button->property("default") == "true")
								defaultButton = cmd;
							
							btns.push_back(make_pair(label, cmd));
						}
					}
				}
			}
		}
	
		// replace parameters
		char s[] = "^0";
		
		for (vector<string>::const_iterator a = inArgs.begin(); a != inArgs.end(); ++a)
		{
			string::size_type p = text.find(s);
			if (p != string::npos)
				text.replace(p, 2, *a);
			++s[1];
		}
		
		dlg = gtk_message_dialog_new(nil, GTK_DIALOG_MODAL,
			type, GTK_BUTTONS_NONE, "%s", text.c_str());
		
		THROW_IF_NIL(dlg);
		
		for (vector<pair<string,uint32> >::iterator b = btns.begin(); b != btns.end(); ++b)
			gtk_dialog_add_button(GTK_DIALOG(dlg), b->first.c_str(), b->second);
	
		if (defaultButton >= 0)
			gtk_dialog_set_default_response(GTK_DIALOG(dlg), defaultButton);
	
		if (xmlDoc != nil)
			xmlFreeDoc(xmlDoc);
		
		xmlCleanupParser();
	}
	catch (...)
	{
		if (xmlDoc != nil)
			xmlFreeDoc(xmlDoc);
		
		xmlCleanupParser();
		
		throw;
	}
	
	return dlg;
}

int32 DisplayAlertWithArgs(
		const char*		inResourceName,
		vector<string>&	inArgs)
{
	int32 result = -1;

	try
	{
		GtkWidget* dlg = CreateAlertWithArgs(inResourceName, inArgs);

		if (MWindow::GetFirstWindow() != nil)
		{
			gtk_window_set_transient_for(
				GTK_WINDOW(dlg),
				GTK_WINDOW(MWindow::GetFirstWindow()->GetGtkWidget()));
		}

		result = gtk_dialog_run(GTK_DIALOG(dlg));
		
		gtk_widget_destroy(dlg);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

void DisplayError(
	const exception&	inErr)
{
	{
		StOKToThrow ok;
		PlaySound("error");
	}
	
	try
	{
		DisplayAlert("exception-alert", inErr.what());
	}
	catch (...)
	{
		GtkWidget* dlg = gtk_message_dialog_new(nil, GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, "%s", inErr.what());
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
	}
}

void DisplayError(
	const string&		inErr)
{
	{
		StOKToThrow ok;
		PlaySound("error");
	}
	
	try
	{
		DisplayAlert("error-alert", inErr.c_str());
	}
	catch (...)
	{
		GtkWidget* dlg = gtk_message_dialog_new(nil, GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, "%s", inErr.c_str());
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
	}
}

