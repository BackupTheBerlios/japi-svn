//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MFILE
#define MFILE

#include <fcntl.h>
#include <cassert>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

#include "MCallbacks.h"

#include "MTypes.h"

namespace fs = boost::filesystem;

class MFileLoader;
class MFileSaver;

// --------------------------------------------------------------------
// MFile, something like a path or URI. 

class MFile
{
  public:
					MFile();
					
					MFile(
						const MFile&		rhs);
					
	explicit		MFile(
						const fs::path&		inPath);
					
	explicit		MFile(
						const std::string&	inURI);
					
	explicit		MFile(
						const char*			inURI);
					
	explicit		MFile(
						GFile*				inFile);
	
	MFile&			operator=(
						const MFile&		rhs);

	MFile&			operator=(
						GFile*				rhs);

	MFile&			operator=(
						const fs::path&		rhs);

	MFile&			operator=(
						const std::string&	rhs);

	bool			operator==(
						const MFile&		rhs) const;

	bool			operator!=(
						const MFile&		rhs) const;

	MFile&			operator/=(
						const char*			inSubPath);

	MFile&			operator/=(
						const fs::path&		inSubPath);

	fs::path		GetPath() const;

	std::string		GetURI() const;
	
	std::string		GetScheme() const;

	std::string		GetFileName() const;
	
	MFile			GetParent() const;
	
					operator GFile* () const;

	MFileLoader*	Load() const;

	MFileSaver*		Save() const;

	bool			IsValid() const;

	bool			IsLocal() const;
	
	bool			Exists() const;

	double			GetModDate() const;

	bool			ReadOnly() const;

	void			ReadAttribute(
						const char*			inName,
						std::string&		outData);

	void			WriteAttribute(
						const char*			inName,
						const std::string&	inData);

  private:
	friend fs::path RelativePath(const MFile&, const MFile&);

	GFile*			mFile;
	bool			mLoaded;
	bool			mReadOnly;
	double			mModDate;
};

MFile operator/(const MFile& lhs, const fs::path& rhs);

fs::path RelativePath(const MFile& lhs, const MFile& rhs);

std::ostream& operator<<(std::ostream& lhs, const MFile& rhs);

// --------------------------------------------------------------------
// MFileLoader, used to load the contents of a file.
// This works strictly asynchronous. 

class MFileLoader
{
  public:

	MCallback<void(float, const std::string&)>	eProgress;
	MCallback<void(const std::string&)>			eError;
	MCallback<void(std::istream&)>				eReadFile;

	virtual void	DoLoad() = 0;

  protected:
					MFileLoader(
						const MFile&		inFile);

	virtual			~MFileLoader();

	const MFile&	mFile;

  private:
					MFileLoader(
						const MFileLoader&	rhs);

	MFileLoader&	operator=(
						const MFileLoader&	rhs);
};

// --------------------------------------------------------------------
// MFileSaver, used to save data to a file.

class MFileSaver
{
  public:

	MCallback<void(float, const std::string&)>	eProgress;
	MCallback<void(const std::string&)>			eError;
	MCallback<void(std::ostream&)>				eWriteFile;

	virtual void	DoSave() = 0;

  protected:
					MFileSaver(
						const MFile&		inFile);

	virtual			~MFileSaver();
	
	const MFile&	mFile;

  private:
					MFileSaver(
						const MFileSaver&	rhs);

	MFileSaver&	operator=(
						const MFileSaver&	rhs);
};

// --------------------------------------------------------------------
//  

enum {
	kFileIter_Deep				= 1 << 0,
	kFileIter_ReturnDirectories	= 1 << 1
};

class MFileIterator
{
  public:
					MFileIterator(
						const fs::path&		inDirectory,
						uint32				inFlags);

					~MFileIterator();

	void			SetFilter(
						const std::string&	inFilter);
	
	bool			Next(fs::path& outFile);

  private:

					MFileIterator(
						const MFileIterator&);
	MFileIterator&	operator=(
						const MFileIterator&);

	struct MFileIteratorImp*
					mImpl;
};

//bool ChooseDirectory(
//	MFile&				outDirectory);

bool ChooseDirectory(
	fs::path&			outDirectory);

bool ChooseOneFile(
	MFile&				ioFile);

bool ChooseFiles(
	bool				inLocalOnly,
	std::vector<MFile>&	outFiles);

bool FileNameMatches(
	const char*			inPattern,
	const fs::path&		inFile);

bool FileNameMatches(
	const char*			inPattern,
	const MFile&		inFile);

bool FileNameMatches(
	const char*			inPattern,
	const std::string&	inFile);

fs::path relative_path(
	const fs::path&		inFromDir,
	const fs::path&		inFile);

#endif
