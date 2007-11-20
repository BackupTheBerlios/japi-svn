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
