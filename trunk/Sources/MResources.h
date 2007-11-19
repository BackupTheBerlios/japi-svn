#ifndef MRESOURCES_H
#define MRESOURCES_H

/*
	Resources are data sources for the application.
	
	They are retrieved by name.
	
	The data returned depends on the current value of
	the LANG environmental variable.
	
*/

bool LoadResource(
		const char*		inName,
		const void*&	outData,
		uint32&			outSize);

void CreateResourceFile(
		const MPath&	inResourceDirectory,
		const MPath&	inResourceFile);

#endif
