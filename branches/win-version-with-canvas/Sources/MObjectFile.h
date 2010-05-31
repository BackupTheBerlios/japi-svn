//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOBJECTFILE_H
#define MOBJECTFILE_H

#include <vector>

#include "MProject.h"
#include "MFile.h"

uint32 AddNameToNameTable(
	std::string&		ioNameTable,
	const std::string&	inName);

struct MObjectFileImp
{
	fs::path			mFile;
	uint32			mTextSize;
	uint32			mDataSize;
					
	virtual			~MObjectFileImp() {}
	
	virtual void	Read(
						const fs::path&		inFile) = 0;

	virtual void	Write(
						const fs::path&		inFile) = 0;

  protected:

	friend class MObjectFile;

	struct MGlobal
	{
		std::string	name;
		std::string	data;
	};

	typedef std::vector<MGlobal>	MGlobals;
	
	MGlobals		mGlobals;
};

class MObjectFile
{
  public:
				MObjectFile(
					MTargetCPU			inTarget);

				MObjectFile(
					const fs::path&		inFile);

				~MObjectFile();

	uint32		GetTextSize() const;
	uint32		GetDataSize() const;

	void		AddGlobal(
					const std::string&	inName,
					const void*			inData,
					uint32				inSize);

	void		Write(
					const fs::path&		inFile);

  private:
	struct MObjectFileImp*	mImpl;
};

#endif
