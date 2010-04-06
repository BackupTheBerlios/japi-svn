//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <cstring>

#include "MAlerts.h"
#include "MError.h"

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
	const fs::path&		inFile)
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
		DisplayError(e);
		if (mImpl != nil)
		{
			mImpl->mTextSize = 0;
			mImpl->mDataSize = 0;
		}
	}
}

MObjectFile::~MObjectFile()
{
	delete mImpl;
}

uint32 MObjectFile::GetTextSize() const
{
	uint32 textSize = 0;
	if (mImpl != nil)
		textSize = mImpl->mTextSize;
	return textSize;
}

uint32 MObjectFile::GetDataSize() const
{
	uint32 dataSize = 0;
	if (mImpl != nil)
		dataSize = mImpl->mDataSize;
	return dataSize;
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
	const fs::path&		inFile)
{
	THROW_IF_NIL((mImpl));
	mImpl->Write(inFile);
}

