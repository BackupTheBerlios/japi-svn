//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MFILE
#define MFILE

#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

#include "MCallbacks.h"

namespace fs = boost::filesystem;

class MFileLoader;
class MFileSaver;

// --------------------------------------------------------------------
// MFile, something like a path or URI.

struct MFileImp;

class MFile
{
  public:
						MFile();
						
						~MFile();
						
						MFile(
							MFileImp*			impl);
						
						MFile(
							const MFile&		rhs);
					
	explicit			MFile(
							const fs::path&		inPath);
					
	explicit			MFile(
							const std::string&	inURI);
					
	explicit			MFile(
							const char*			inURI);
					
	explicit			MFile(
							GFile*				inFile);
	
	MFile&				operator=(
							const MFile&		rhs);

	MFile&				operator=(
							GFile*				rhs);

	MFile&				operator=(
							const fs::path&		rhs);

	MFile&				operator=(
							const std::string&	rhs);

	bool				operator==(
							const MFile&		rhs) const;

	MFile&				operator/=(
							const char*			inSubPath);

	MFile&				operator/=(
							const fs::path&		inSubPath);

	fs::path			GetPath() const;

	std::string			GetURI() const;
		
	std::string			GetScheme() const;

	std::string			GetFileName() const;
	
	MFile				GetParent() const;
	
						operator GFile* () const;

	MFileLoader*		Load();

	MFileSaver*			Save();

	bool				IsValid() const;

	bool				IsLocal() const;
	
	bool				Exists() const;

	double				GetModDate() const;

	bool				ReadOnly() const;

	ssize_t				ReadAttribute(
							const char*			inName,
							void*				outData,
							size_t				inDataSize) const;

	size_t				WriteAttribute(
							const char*			inName,
							const void*			inData,
							size_t				inDataSize) const;

  private:
	
	void				SetFileInfo(
							bool				inReadOnly,
							double				inModDate);

	friend fs::path RelativePath(const MFile&, const MFile&);
	friend class MFileLoader;
	friend class MFileSaver;

	MFileImp*			mImpl;
	bool				mReadOnly;
	double				mModDate;
};

MFile operator/(const MFile& lhs, const fs::path& rhs);

fs::path RelativePath(const MFile& lhs, const MFile& rhs);

std::ostream& operator<<(std::ostream& lhs, const MFile& rhs);

// --------------------------------------------------------------------
// MFileLoader, used to load the contents of a file.

class MFileLoader
{
  public:

	MCallback<void(float, const std::string&)>	eProgress;
	MCallback<void(const std::string&)>			eError;
	MCallback<void(std::istream&)>				eReadFile;
	MCallback<void()>							eFileLoaded;

	virtual void	DoLoad() = 0;
	
	virtual void	Cancel();

  protected:
					MFileLoader(
						MFile&				inFile);

	virtual			~MFileLoader();

	MFile&			mFile;

	void			SetFileInfo(
						bool				inReadOnly,
						double				inModDate);

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
	MCallback<bool()>							eAskOverwriteNewer;
	MCallback<void(void)>						eFileWritten;

	virtual void	DoSave() = 0;

	virtual void	Cancel();

  protected:
					MFileSaver(
						MFile&				inFile);

	virtual			~MFileSaver();
	
	MFile&			mFile;

	void			SetFileInfo(
						bool				inReadOnly,
						double				inModDate);

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

void NormalizePath(
	std::string&		ioPath);

void NormalizePath(
	fs::path&			ioPath);

#endif
