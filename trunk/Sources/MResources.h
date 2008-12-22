//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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
