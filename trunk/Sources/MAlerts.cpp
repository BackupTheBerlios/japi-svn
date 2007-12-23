/*
	Copyright (c) 2007, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "MJapieG.h"

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <vector>
#include <boost/bind.hpp>

#include "MResources.h"
#include "MAlerts.h"
#include "MUtils.h"
#include "MStrings.h"

using namespace std;

int32 DisplayAlertWithArgs(
		const char*		inResourceName,
		vector<string>&	inArgs)
{
	int32 result = -1;
	xmlDocPtr xmlDoc = nil;
	
	xmlInitParser();

	gdk_threads_enter();
	
	try
	{
		const char* xml;
		uint32 size;
		
		if (not LoadResource(inResourceName, xml, size))
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
		
		GtkWidget* dlg = gtk_message_dialog_new(nil, GTK_DIALOG_MODAL,
			type, GTK_BUTTONS_NONE, text.c_str());
		
		for (vector<pair<string,uint32> >::iterator b = btns.begin(); b != btns.end(); ++b)
			gtk_dialog_add_button(GTK_DIALOG(dlg), b->first.c_str(), b->second);

		if (defaultButton >= 0)
			gtk_dialog_set_default_response(GTK_DIALOG(dlg), defaultButton);

		result = gtk_dialog_run(GTK_DIALOG(dlg));
		
		gtk_widget_destroy(dlg);
	}
	catch (exception& e)
	{
		MError::DisplayError(e);
	}
	
	if (xmlDoc != nil)
		xmlFreeDoc(xmlDoc);
	
	xmlCleanupParser();
	
	gdk_threads_leave();

	return result;
}
