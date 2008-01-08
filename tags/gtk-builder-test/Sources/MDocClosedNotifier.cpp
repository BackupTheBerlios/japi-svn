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

int MDocClosedNotifier::GetFD() const
{
	return mImpl->mFD;
}
