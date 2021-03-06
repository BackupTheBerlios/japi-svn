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

#ifndef MOBJECTFILE_H
#define MOBJECTFILE_H

#include <vector>

#include "MProjectTarget.h"
#include "MFile.h"

uint32 AddNameToNameTable(
	std::string&		ioNameTable,
	const std::string&	inName);

struct MObjectFileImp
{
	MPath			mFile;
	uint32			mTextSize;
	uint32			mDataSize;
					
	virtual			~MObjectFileImp() {}
	
	virtual void	Read(
						const MPath&		inFile) = 0;

	virtual void	Write(
						const MPath&		inFile) = 0;

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
					const MPath&		inFile);

				~MObjectFile();

	uint32		GetTextSize() const;
	uint32		GetDataSize() const;

	void		AddGlobal(
					const std::string&	inName,
					const void*			inData,
					uint32				inSize);

	void		Write(
					const MPath&		inFile);

  private:
	struct MObjectFileImp*	mImpl;
};

#endif
