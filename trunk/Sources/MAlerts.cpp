//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/foreach.hpp>

#include <vector>

#include <zeep/xml/document.hpp>

#include "MResources.h"
#include "MAlerts.h"
#include "MStrings.h"
#include "MWindow.h"
#include "MError.h"
#include "MSound.h"

#define foreach BOOST_FOREACH

using namespace std;
namespace xml = zeep::xml;
namespace io = boost::iostreams;

GtkWidget* CreateAlertWithArgs(
	const char* 	inResourceName,
	vector<string>&	inArgs)
{
	GtkWidget* dlg = nil;
	
	string resource = string("Alerts/") + inResourceName + ".xml";
	
	mrsrc::rsrc rsrc(resource);
	if (not rsrc)
		THROW(("Could not load resource Alerts/%s.xml", inResourceName));
	
	// parse the XML data
	io::stream<io::array_source> data(rsrc.data(), rsrc.size());
	xml::document doc(data);
	
	// build an alert
	xml::element* root = doc.find_first("/alert");
	
	if (root->qname() != "alert")
		THROW(("Invalid resource for alert %s, first tag should be <alert>", inResourceName));
	
	string text;
	vector<pair<string,uint32> > btns;
	int32 defaultButton = -1;
	GtkMessageType type = GTK_MESSAGE_ERROR;
	
	if (root->get_attribute("type") == "warning")
		type = GTK_MESSAGE_WARNING;
	
	xml::element_set elements(root->children<xml::element>());
	
	foreach (xml::element* item, elements)
	{
		if (item->qname() == "message")
			text = _(item->content());
		else if (item->qname() == "buttons")
		{
			foreach (xml::element* button, item->children<xml::element>())
			{
				if (button->qname() == "button")
				{
					string label = _(button->get_attribute("title"));
					uint32 cmd = atoi(button->get_attribute("cmd").c_str());
					if (button->get_attribute("default") == "true")
						defaultButton = cmd;
					
					btns.push_back(make_pair(label, cmd));
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

