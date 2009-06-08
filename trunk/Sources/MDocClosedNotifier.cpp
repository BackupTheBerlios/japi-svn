//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <iostream>

#include "MDocClosedNotifier.h"
#include "MError.h"

using namespace std;

struct MDocClosedNotifierImp
{
	int32			mRefCount;
	int				mFD;	
};

MDocClosedNotifier::MDocClosedNotifier(
	int			inFD)
	: mImpl(new MDocClosedNotifierImp)
{
	mImpl->mRefCount = 1;
	mImpl->mFD = inFD;
////PRINT(("%s: %d", __func__, mImpl->mFD));
}

MDocClosedNotifier::MDocClosedNotifier(
	const MDocClosedNotifier&	inRHS)
{
	mImpl = inRHS.mImpl;
	++mImpl->mRefCount;
//PRINT(("%s: %d", __func__, mImpl->mFD));
}

MDocClosedNotifier&	MDocClosedNotifier::operator=(
	const MDocClosedNotifier&	inRHS)
{
//PRINT(("%s: %d", __func__, mImpl->mFD));
	if (this != &inRHS)
	{
		if (--mImpl->mRefCount <= 0)
		{
//PRINT(("closing"));
			close(mImpl->mFD);
			delete mImpl;
		}
		
		mImpl = inRHS.mImpl;
		++mImpl->mRefCount;
	}
	
//PRINT(("%s: %d", __func__, mImpl->mFD));
	return *this;	
}

MDocClosedNotifier::~MDocClosedNotifier()
{
//PRINT(("%s: %d", __func__, mImpl->mFD));
	if (--mImpl->mRefCount <= 0)
	{
//PRINT(("closing"));
		close(mImpl->mFD);
		delete mImpl;
	}
}

int MDocClosedNotifier::GetFD() const
{
//PRINT(("%s: %d", __func__, mImpl->mFD));
	return mImpl->mFD;
}
