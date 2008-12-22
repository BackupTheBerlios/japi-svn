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

#include "MTypes.h"

namespace fs = boost::filesystem;

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

class MUrl;

ssize_t read_attribute(
	const fs::path&		inPath,
	const char*			inName,
	void*				outData,
	size_t				inDataSize);

void write_attribute(
	const fs::path&		inPath,
	const char*			inName,
	const void*			inData,
	size_t				inDataSize);

bool ChooseDirectory(
	fs::path&				outDirectory);

bool ChooseOneFile(
	MUrl&				ioFile);

bool ChooseFiles(
	bool				inLocalOnly,
	std::vector<MUrl>&	outFiles);

bool FileNameMatches(
	const char*			inPattern,
	const fs::path&		inFile);

bool FileNameMatches(
	const char*			inPattern,
	const std::string&	inFile);

fs::path relative_path(
	const fs::path&		inFromDir,
	const fs::path&		inFile);

#endif
