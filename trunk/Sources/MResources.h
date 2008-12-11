/*
	Copyright (c) 2007, Maarten L. Hekkelman
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

#ifndef MRESOURCES_H
#define MRESOURCES_H

#include <istream>
#include "MObjectFile.h"

/*
	Resources are data sources for the application.
	
	They are retrieved by name.
	
	The data returned depends on the current value of
	the LANG environmental variable.
	
*/

// LoadResource returns a pointer to the resource named inName
// If this resources is not found, an exception is thrown 

bool LoadResource(
		const std::string&	inName,
		const char*&		outData,
		uint32&				outSize);

class MResourceFile
{
  public:
						MResourceFile(
							MTargetCPU			inTarget);
						
						~MResourceFile();
	
	void				Add(
							const std::string&	inPath,
							const void*			inData,
							uint32				inSize);

	void				Add(
							const std::string&	inPath,
							const fs::path&		inFile);
	
	void				Write(
							const fs::path&		inFile);

  private:
	struct MResourceFileImp*	mImpl;
};

#endif
