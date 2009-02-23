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
#include "node.hpp"

class MProjectGroup;
class MProjectItem;

class MePubDocument : public MDocument
{
  public:

						MePubDocument();

	explicit			MePubDocument(
							const MFile&		inProjectFile);

//						MePubDocument(
//							const fs::path&		inParentDir,
//							const std::string&	inName);

	virtual				~MePubDocument();

	virtual bool		UpdateCommandStatus(
							uint32				inCommand,
							MMenu*				inMenu,
							uint32				inItemIndex,
							bool&				outEnabled,
							bool&				outChecked);

	virtual bool		ProcessCommand(
							uint32				inCommand,
							const MMenu*		inMenu,
							uint32				inItemIndex,
							uint32				inModifiers);

	virtual void		ReadFile(
							std::istream&		inFile);

	virtual void		WriteFile(
							std::ostream&		inFile);

	MProjectGroup*		GetFiles() const;
	MProjectGroup*		GetTOC() const;

	MEventOut<void(MProjectItem*)>				eInsertedFile;
	MEventOut<void(MProjectGroup*,int32)>		eRemovedFile;

	void				CreateNewGroup(
							const std::string&	inGroupName,
							MProjectGroup*		inGroup,
							int32				inIndex);

	MEventIn<void(const std::string&, MProjectGroup*, MProjectItem*&)>
											eCreateItem;
	
	MEventIn<void()>						eItemMoved;
	MEventIn<void()>						eItemRemoved;

	void				CreateItem(
							const std::string&	inFile,
							MProjectGroup*		inGroup,
							MProjectItem*&		outItem);

	void				ItemMoved();

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

	virtual void		SetModified(
							bool				inModified);

  private:
	
	void				ParseOPF(
							const fs::path&		inDirectory,
							xml::node&			inOPF,
							std::vector<std::string>&
												outProblems);

	xml::node_ptr		CreateOPF(
							const fs::path&		inOEBPS);

	void				ParseNCX(
							xml::node&			inNCX);

	void				ParseNavPoint(
							MProjectGroup*		inGroup,
							xml::node_ptr		inNavPoint);

	xml::node_ptr		CreateNCX();

	typedef std::map<std::string,uint32> MPlayOrder;

	void				CreateNavMap(
							MProjectGroup*		inGroup,
							xml::node_ptr		inNavPoint,
							uint32&				ioID,
							MPlayOrder&			ioPlayOrder);

	std::string			mRootFile;
	fs::path			mTOCFile;
	std::set<std::string>
						mLinear;
	MProjectGroup		mRoot, mTOC;
	std::string			mDocumentID, mDocumentIDScheme;
	AES_KEY				mKey;
	std::map<std::string,std::string>
						mDublinCore;
};

#endif
