#ifndef MDOCUMENT_H
#define MDOCUMENT_H

#include "MURL.h"

class MDocument
{
  public:
						MDocument();
	explicit			MDocument(
							const MURL&		inURL);

	virtual				~MDocument();
	
	MURL				GetURL()					{ return mURL; }
	
	static MDocument*	GetDocumentForURL(
							const MURL&		inURL,
							bool			inCreateIfNeeded);
	
	static MDocument*	GetFirstDocument()			{ return sFirst; }
	MDocument*			GetNextDocument()			{ return mNext; }

  protected:

	virtual void		Initialize();

	MURL				mURL;
	MDocument*			mNext;
	static MDocument*	sFirst;
};

#endif
