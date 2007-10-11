/*
	Copyright (c) 2006, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MFILE
#define MFILE

#include <fcntl.h>
#include <cassert>

//#include <boost/filesystem/path.hpp>
//#include <boost/filesystem/operations.hpp>
//#include <boost/filesystem/convenience.hpp>

#include "MTypes.h"
#include "MURL.h"

ssize_t read_attribute(const MURL& inPath, const char* inName, void* outData, size_t inDataSize);
void write_attribute(const MURL& inPath, const char* inName, const void* inData, size_t inDataSize);

namespace MFileSystem
{
	double	GetModificationDate(
				const MURL&		inURL);
}

class MFile
{
  public:
					MFile(const MURL& inFileSpec, int inFD = -1);
	virtual			~MFile();

	void			Open(int inPermissions);
	void			Close();
	bool			IsOpen()						{ return mIsOpen; }
	
	void			GetFileSpec(MURL& outFileSpec) const;
	const MURL&		GetFileSpec() const				{ return mFileSpec; }

	int32			Read(void* inBuffer, int32 inByteCount);
	int32			Write(const void* inBuffer, int32 inByteCount);
	int64			Seek(int64 inOffset, int inMode);
	int64			GetSize() const;
	void			SetSize(int64 inSize);
	
	double			GetModificationDate() const;
	
	int				GetFD() const					{ assert(mIsOpen); return mFD; }
	
  private:
	MURL			mFileSpec;
	int				mFD;
	bool			mIsOpen;
};

class MSafeSaver
{
  public:
					MSafeSaver(const MURL& inFile);
	virtual			~MSafeSaver();
	
	void			Commit(MURL& outFileSpec);
	
	MFile*			GetTempFile();
	
  private:
	MURL			mTempFileSpec;
	MURL			mDestFileSpec;
	std::auto_ptr<MFile>
					mTempFile;
};

enum {
	kFileIter_Deep				= 1 << 0,
	kFileIter_TEXTFilesOnly		= 1 << 1,
	kFileIter_ReturnDirectories	= 1 << 2
};

class MFileIterator
{
  public:
					MFileIterator(const MURL& inDirectory, uint32 inFlags);
					~MFileIterator();

	void			SetFilter(const std::string& inFilter);
	
	bool			Next(MURL& outFile);

  private:

	struct MFileIteratorImp*
					mImpl;
};

bool FileNameMatches(const char* inPattern, const MURL& inFile);
bool FileNameMatches(const char* inPattern, const std::string& inFile);

MURL relative_path(const MURL& inFromDir, const MURL& inFile);

#endif
