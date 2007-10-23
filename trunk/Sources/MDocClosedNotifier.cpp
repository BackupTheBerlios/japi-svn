#include "MJapieG.h"

#include <iostream>

#include "MDocClosedNotifier.h"

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
}

MDocClosedNotifier::MDocClosedNotifier(
	const MDocClosedNotifier&	inRHS)
{
	mImpl = inRHS.mImpl;
	++mImpl->mRefCount;
}

MDocClosedNotifier&	MDocClosedNotifier::operator=(
	const MDocClosedNotifier&	inRHS)
{
	if (this != &inRHS)
	{
		if (--mImpl->mRefCount <= 0)
		{
			close(mImpl->mFD);
			delete mImpl;
		}
		
		mImpl = inRHS.mImpl;
		++mImpl->mRefCount;
	}
	
	return *this;	
}

MDocClosedNotifier::~MDocClosedNotifier()
{
	if (--mImpl->mRefCount <= 0)
	{
		close(mImpl->mFD);
		delete mImpl;
	}
}
