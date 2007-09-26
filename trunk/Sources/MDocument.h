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
	
	bool				IsSpecified() const			{ return mSpecified; }
	MURL				GetURL() const				{ return mURL; }
	bool				IsDirty() const				{ return mDirty; }
	
	static MDocument*	GetDocumentForURL(
							const MURL&		inURL,
							bool			inCreateIfNeeded);
	
	static MDocument*	GetFirstDocument()			{ return sFirst; }
	MDocument*			GetNextDocument()			{ return mNext; }

  protected:

	virtual void		Initialize();

	MURL				mURL;
	bool				mSpecified;
	bool				mDirty;
	MDocument*			mNext;
	static MDocument*	sFirst;
};

#endif
