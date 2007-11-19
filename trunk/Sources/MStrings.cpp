#include "MJapieG.h"

#include <boost/filesystem/fstream.hpp>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlwriter.h>

#include "MStrings.h"
#include "MPatriciaTree.h"
#include "MGlobals.h"

using namespace std;

namespace {

typedef MPatriciaTree<string>		StringTable;

StringTable		gStringTable;
bool			gInited = false;

void InitStringTable()
{
	gInited = true;
	
    LIBXML_TEST_VERSION

	const char* lang = getenv("LANG");
	
	// the LANG string should contain a two letter code for the language

	if (lang != nil and
		strlen(lang) >= 2 and
		(strlen(lang) == 2 or lang[2] == '_') and
		strncmp(lang, "en", 2) != 0)
	{
		string l(lang, lang + 2);
		
		fs::ifstream f(gPrefsDir / (string("localised.") + l + ".xml"));
		if (f.is_open())
		{
			filebuf* b = f.rdbuf();
			
			uint32 size = b->pubseekoff(0, ios::end, ios::in);
			b->pubseekoff(0, ios::beg, ios::in);
			
			char* text = new char[size];
			
			b->sgetn(text, size);
			f.close();

			xmlDocPtr			xmlDoc = nil;
			
			xmlInitParser();
		
			try
			{
				string xml;
				
				xmlDoc = xmlParseMemory(text, size);
				if (xmlDoc == nil)
					THROW(("Failed to parse localised strings file"));
				
				xmlNodePtr strings = xmlDoc->children;
				if (strings == nil or strcmp((const char*)strings->name, "strings") != 0)
					THROW(("Invalid localised strings file"));
				
				for (xmlNodePtr sNode = strings->children; sNode != nil; sNode = sNode->next)
				{
					if (xmlNodeIsText(sNode) or strcmp((const char*)sNode->name, "string"))
						continue;
					
					string s1, s2;
					
					for (xmlNodePtr node = sNode->children; node != nil; node = node->next)
					{
						if (xmlNodeIsText(node) or strcmp((const char*)node->name, "loc") or node->children == nil)
							continue;
						
						const char* p = (const char*)xmlGetProp(node, BAD_CAST "lang");
						const char* v = (const char*)XML_GET_CONTENT(node->children);
						if (p == nil or v == nil)
							continue;
						
						if (strcmp(p, "en") == 0)
							s1 = v;
						else if (strncmp(p, lang, 2) == 0)
							s2 = v;
					}
					
					if (s1.length() > 0 and s2.length() > 0)
						gStringTable[s1] = s2;
				}
		
				xmlFreeDoc(xmlDoc);
			}
			catch (exception& e)
			{
				MError::DisplayError(e);
			}
			catch (...) {}
			
			delete[] text;
		}
	}
}
	
}

const char* GetLocalisedString(
				const char* inString)
{
	const char* result = inString;

	if (not gInited)
		InitStringTable();
	
	const string& l = gStringTable[inString];
	if (l.length())
		result = l.c_str();
	
	return result;
}

string GetLocalisedString(
	const string&	inString)
{
	return GetLocalisedString(inString.c_str());
}

string GetFormattedLocalisedStringWithArguments(
	const string&			inString,
	const vector<string>&	inArgs)
{
	string result = GetLocalisedString(inString.c_str());
	
	char s[] = "^1";
	
	for (vector<string>::const_iterator a = inArgs.begin(); a != inArgs.end(); ++a)
	{
		string::size_type p = result.find(s);
		if (p != string::npos)
			result.replace(p, 2, *a);
		++s[1];
	}
	
	return result;
}

