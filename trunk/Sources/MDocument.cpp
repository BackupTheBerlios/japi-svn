#include "MJapieG.h"


#include "MDocument.h"

using namespace std;

MDocument* MDocument::sFirst;

MDocument::MDocument()
{	
}

MDocument::MDocument(
	const MURL&		inURL)
	: mURL(inURL)
{
}

MDocument::~MDocument()
{
}

void MDocument::Initialize()
{
}

MDocument* MDocument::GetDocumentForURL(
	const MURL&		inURL,
	bool			inCreateIfNeeded)
{
	MDocument* doc = sFirst;
	while (doc != nil)
	{
		if (doc->GetURL() == inURL)
			break;
		doc = doc->mNext;
	}
	
	if (doc == nil and inCreateIfNeeded)
		doc = new MDocument(inURL);
	
	return doc;
}

