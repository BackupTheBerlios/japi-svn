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

#include "MJapieG.h"

#if defined(__APPLE__) and defined(__MACH__)
#	include "MObjectFileImp_macho.h"

struct MNativeOjectFileImp : public MMachoObjectFileImp {};

#else
#	include "MObjectFileImp_elf.h"
#endif

#include <boost/filesystem/fstream.hpp>

#include "MObjectFile.h"

using namespace std;

namespace fs = boost::filesystem;

uint32 AddNameToNameTable(
	string&			ioNameTable,
	const string&	inName)
{
	uint32 result = 0;
	
	const char* p = ioNameTable.c_str();
	const char* e = p + ioNameTable.length();
	
	while (p < e)
	{
		if (inName == p)
		{
			result = p - ioNameTable.c_str();
			break;
		}

		p += strlen(p) + 1;
	}
	
	if (p >= e)
	{
		result = ioNameTable.length();
		ioNameTable.append(inName);
		ioNameTable.append("\0", 1);
	}
	
	return result;
}

MObjectFile::MObjectFile(
	MTargetCPU			inTarget)
	: mImpl(nil)
{
#if defined(__APPLE__) and defined(__MACH__)
	mImpl = new MNativeOjectFileImp();
#else
	mImpl = CreateELFObjectFileImp(inTarget);
#endif
}

MObjectFile::MObjectFile(
	const MPath&		inFile)
	: mImpl(nil)
{
	try
	{
#if defined(__APPLE__) and defined(__MACH__)
		mImpl = new MNativeOjectFileImp();
#else
		mImpl = CreateELFObjectFileImp(inFile);
#endif
		
		mImpl->mTextSize = 0;
		mImpl->mDataSize = 0;

		mImpl->Read(inFile);
	}
	catch (std::exception& e)
	{
		MError::DisplayError(e);
		mImpl->mTextSize = 0;
		mImpl->mDataSize = 0;
	}
}

MObjectFile::~MObjectFile()
{
	delete mImpl;
}

uint32 MObjectFile::GetTextSize() const
{
	return mImpl->mTextSize;
}

uint32 MObjectFile::GetDataSize() const
{
	return mImpl->mDataSize;
}

void MObjectFile::AddGlobal(
	const string&	inName,
	const void*		inData,
	uint32			inSize)
{
	MObjectFileImp::MGlobal g;
	g.name = inName;
	g.data.assign(reinterpret_cast<const char*>(inData), inSize);
	
	mImpl->mGlobals.push_back(g);
}

void MObjectFile::Write(
	const MPath&		inFile)
{
	mImpl->Write(inFile);
}

