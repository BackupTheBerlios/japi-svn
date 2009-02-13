#ifndef MFILELOADER_H
#define MFILELOADER_H

#include <string>
#include "MP2PEvents.h"

class MFileLoader
{
  public:
					MFileLoader(
						const char*		inURI);

	virtual			~MFileLoader();
	
	MEventOut<void(float)>				eProgress;
	MEventOut<void(std::string)>		eError;
	MEventOut<void(const char*,uint32)>	eLoaded;

  private:
	struct MFileLoaderImpl*				mImpl;
};

#endif
