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
		const char*		inName,
		const char*&	outData,
		uint32&			outSize);

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
							const MPath&		inFile);
	
	void				Write(
							const MPath&		inFile);

  private:
	struct MResourceFileImp*	mImpl;
};

#endif
