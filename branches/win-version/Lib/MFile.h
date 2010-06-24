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

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

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
					
						MFile(
							const char*			inURI,
							bool				isAbsoluteURI = false);
					
	// GTK has the nasty habit of turning URI's into absolute URI's...
						MFile(
							const std::string&	inURI,
							bool				isAbsoluteURI = false);

	MFile&				operator=(
							const MFile&		rhs);

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

	std::string			GetHost() const;
	
	std::string			GetUser() const;
	
	uint16				GetPort() const;

	std::string			GetFileName() const;
	
	MFile				GetParent() const;
	
	MFileLoader*		Load();

	MFileSaver*			Save();

	bool				IsValid() const;

	bool				IsLocal() const;
	
	bool				Exists() const;

	std::time_t			GetModDate() const;

	bool				ReadOnly() const;

	int32				ReadAttribute(
							const char*			inName,
							void*				outData,
							uint32				inDataSize) const;

	int32				WriteAttribute(
							const char*			inName,
							const void*			inData,
							uint32				inDataSize) const;

  private:
	
	void				SetFileInfo(
							bool				inReadOnly,
							std::time_t			inModDate);

	friend fs::path RelativePath(const MFile&, const MFile&);
	friend class MFileLoader;
	friend class MFileSaver;

	MFileImp*			mImpl;
	bool				mReadOnly;
	std::time_t			mModDate;
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
						std::time_t			inModDate);

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
						std::time_t			inModDate);

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

namespace MFileDialogs
{

bool ChooseDirectory(
	MWindow*			inParent,
	fs::path&			outDirectory);

bool ChooseOneFile(
	MWindow*			inParent,
	MFile&				ioFile);

bool ChooseFiles(
	MWindow*			inParent,
	bool				inLocalOnly,
	std::vector<MFile>&	outFiles);

bool SaveFileAs(
	MWindow*			inParent,
	fs::path&			ioFile);

}

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

void URLDecode(
	std::string&		ioURL);

void URLEncode(
	std::string&		ioURL);

#endif
