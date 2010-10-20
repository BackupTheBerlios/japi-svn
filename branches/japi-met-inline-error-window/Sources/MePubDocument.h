//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEPUBDOCUMENT_H
#define MEPUBDOCUMENT_H

#include <map>
#include <set>

#include <boost/array.hpp>
#include <openssl/aes.h>

#include "MDocument.h"
#include "MProjectItem.h"
#include <zeep/xml/node.hpp>

class MProjectGroup;
class MProjectItem;
class MTextDocument;
class MMessageList;

class MePubDocument : public MDocument
{
  public:
						MePubDocument();

	explicit			MePubDocument(
							const MFile&		inProjectFile);

	virtual				~MePubDocument();

	static MePubDocument*
						GetFirstEPubDocument();
	
	MePubDocument*		GetNextEPubDocument();

	void				AddDocument(
							MTextDocument*		inDoc);

	void				ImportOEB(
							const MFile&		inOEB);

	void				InitializeNew();

	MProjectGroup*		GetFiles() const;
	MProjectGroup*		GetTOC() const;

	MEventIn<void(const std::string&, MProjectGroup*, MProjectItem*&)>
											eCreateItem;
	
	MEventIn<void()>						eItemMoved;
	MEventIn<void()>						eItemRemoved;
	MEventIn<void(MProjectItem*,const std::string&,
		const std::string&)>				eItemRenamed;
	
	MEventOut<void(MProjectItem*)>			eFileItemInserted;

	void				CreateItem(
							const std::string&	inFile,
							MProjectGroup*		inGroup,
							MProjectItem*&		outItem);

	void				ItemMoved();

	void				ItemRenamed(
							MProjectItem*		inItem,
							const std::string&	inOldName,
							const std::string&	inNewName);

	std::string			GetDocumentID() const;
	
	std::string			GetDocumentIDScheme() const;
	
	void				SetDocumentID(
							const std::string&	inID);

	void				SetDocumentIDScheme(
							const std::string&	inScheme);

	void				GenerateNewDocumentID();
	
	std::string			GetDublinCoreValue(
							const std::string&	inName) const;

	void				SetDublinCoreValue(
							const std::string&	inName,
							const std::string&	inValue);

	std::string			GetFileData(
							const fs::path&		inFile);

	void				SetFileData(
							const fs::path&		inFile,
							const std::string&	inText);

	MFile				GetFileForSrc(
							const std::string&	inSrc);

	virtual void		SetModified(
							bool				inModified);

  private:

	virtual void		ReadFile(
							std::istream&		inFile);

	virtual void		WriteFile(
							std::ostream&		inFile);
	
	void				ParseOPF(
							const fs::path&		inDirectory,
							zeep::xml::element*	inOPF,
							MMessageList&		outProblems);

	zeep::xml::element*	CreateOPF(
							const fs::path&		inOEBPS);

	void				ParseNCX(
							zeep::xml::element*	inNCX);

	void				ParseNavPoint(
							MProjectGroup*		inGroup,
							zeep::xml::element&	inNavPoint);

	zeep::xml::element*	CreateNCX();

	typedef std::map<std::string,uint32> MPlayOrder;

	void				CreateNavMap(
							MProjectGroup*		inGroup,
							zeep::xml::element*	inNavPoint,
							uint32&				ioID,
							MPlayOrder&			ioPlayOrder);

	fs::path			mRootFile, mTOCFile;
	std::set<std::string>
						mLinear;
	MProjectGroup		mRoot, mTOC;
	std::string			mDocumentID, mDocumentIDScheme;
	AES_KEY				mKey;
	std::map<std::string,std::string>
						mDublinCore;
};

#endif
