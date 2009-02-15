#ifndef MFILELOADER_H
#define MFILELOADER_H

#include <string>
#include "MCallbacks.h"

class MFileLoader
{
  public:
					MFileLoader(
						GFile*					inFile);

	virtual			~MFileLoader();
	
	MCallback<void(float, const std::string&)>	eProgress;
	MCallback<void(const std::string&)>			eError;
	MCallback<void(const char*,uint32)>			eLoaded;
	
	double			GetModDate() const;
	bool			ReadOnly() const;

  private:
	struct MFileLoaderImpl*				mImpl;
};

#endif
