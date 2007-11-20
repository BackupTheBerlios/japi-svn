#ifndef MRESOURCES_H
#define MRESOURCES_H

/*
	Resources are data sources for the application.
	
	They are retrieved by name.
	
	The data returned depends on the current value of
	the LANG environmental variable.
	
*/

// LoadResource returns a pointer to the resource named inName
// If this resources is not found, an exception is thrown 

const void* LoadResource(
		const char*		inName);

class MResourceFile
{
  public:
						MResourceFile();
						
						~MResourceFile();
	
	void				Add(
							const char*		inLocale,
							const char*		inName,
							const void*		inData,
							uint32			inSize);

	void				Add(
							const char*		inLocale,
							const char*		inName,
							const MPath&	inFile);
	
	void				Write(
							const MPath&	inFile);

  private:
	struct MResourceFileImp*	mImpl;
};

#endif
