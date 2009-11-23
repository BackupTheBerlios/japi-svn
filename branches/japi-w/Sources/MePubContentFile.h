//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEPUBCONTENTFILE_H
#define MEPUBCONTENTFILE_H

#include "MFile.h"
#include "MFileImp.h"

class MePubDocument;

struct MePubContentFile : public MFileImp
{
						MePubContentFile(
							MePubDocument*	inEPub,
							const fs::path&	inPath)
							: mEPub(inEPub)
							, mPath(inPath)
						{
						}

	virtual bool		Equivalent(const MFileImp* rhs) const
						{
							const MePubContentFile* erhs = dynamic_cast<const MePubContentFile*>(rhs);
							return erhs != nil and
								mEPub == erhs->mEPub and
								mPath == erhs->mPath;
						}

	virtual std::string	GetURI() const
						{
							return mEPub->GetFile().GetURI() + '/' + mPath.string();
						}
						
	virtual fs::path	GetPath() const
						{
							return mPath;
						}
						
	virtual std::string	GetScheme() const
						{
							return "epub";
						}
						
	virtual std::string	GetFileName() const
						{
							return mPath.leaf();
						}
						
	virtual bool		IsLocal() const
						{
							return false;
						}
						
	virtual MFileImp*	GetParent() const
						{
							return new MePubContentFile(mEPub, mPath.branch_path());
						}
						
	virtual MFileImp*	GetChild(const fs::path& inSubPath) const
						{
							return new MePubContentFile(mEPub, mPath / inSubPath);
						}
	
	class MePubFileLoader : public MFileLoader
	{
	  public:
						MePubFileLoader(MFile& inFile, const std::string& inData, double inModDate)
							: MFileLoader(inFile), data(inData), modDate(inModDate) {}
	
		void			DoLoad()
						{
							std::stringstream s(data);
							eReadFile(s);
							SetFileInfo(false, modDate);
							eFileLoaded();
							delete this;
						}
	
		std::string		data;
		double			modDate;
	};
	
	class MePubFileSaver : public MFileSaver
	{
	  public:
						MePubFileSaver(MFile& inFile, MePubDocument* inEPub, fs::path inPath)
							: MFileSaver(inFile), mEPub(inEPub), mPath(inPath) {}

	void				DoSave()
						{
							std::stringstream s;
							eWriteFile(s);
							mEPub->SetFileData(mPath, s.str());
							SetFileInfo(false, mEPub->GetFile().GetModDate());
							eFileWritten();
							delete this;
						}

		MePubDocument*		mEPub;
		fs::path			mPath;
	};

	virtual MFileLoader* Load(MFile& inFile)
						{
							return new MePubFileLoader(inFile, mEPub->GetFileData(mPath), mEPub->GetFile().GetModDate());
						}
						
	virtual MFileSaver*	Save(MFile& inFile)
						{
							return new MePubFileSaver(inFile, mEPub, mPath);
						}
						

	MePubDocument*		mEPub;
	fs::path			mPath;
};

#endif
