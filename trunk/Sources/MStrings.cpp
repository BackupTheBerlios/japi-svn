#include "MJapieG.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlwriter.h>

#include "MStrings.h"
#include "MPatriciaTree.h"

namespace {

typedef MPatriciaTree<const char*>		StringTable;

auto_ptr<StringTable>	gStringTable;

void InitStringTable()
{
	gStringTable.reset(new StringTable);
	
	
}
	
}

const char* GetLocalisedString(
				const char* inString)
{
	
}

std::string GetFormattedLocalisedStringWithArguments(
				const std::string&			inString,
				const std::vector<string>&	inArgs);

