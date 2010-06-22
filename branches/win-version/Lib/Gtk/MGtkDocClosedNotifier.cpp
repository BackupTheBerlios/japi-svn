//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include <iostream>
#include <cassert>

#include "MDocClosedNotifier.h"
#include "MError.h"

using namespace std;

	
	//if (inRead)
	//{
	//	mPreparedForStdOut = true;
	//	mDataFD = inNotifier.GetFD();

	//	int flags = fcntl(mDataFD, F_GETFL, 0);
	//	if (fcntl(mDataFD, F_SETFL, flags | O_NONBLOCK))
	//		cerr << _("Failed to set fd non blocking: ") << strerror(errno) << endl;
	//}

	//if (mDataFD >= 0)
	//{
	//	char buffer[10240];
	//	int r = read(mDataFD, buffer, sizeof(buffer));
	//	if (r == 0 or (r < 0 and errno != EAGAIN))
	//		mDataFD = -1;
	//	else if (r > 0)
	//		StdOut(buffer, r);
	//}


MDocClosedNotifierImpl::MDocClosedNotifierImpl()
	: mRefCount(1)
{
}

MDocClosedNotifierImpl::~MDocClosedNotifierImpl()
{
	assert(mRefCount == 0);
}

void MDocClosedNotifierImpl::AddRef()
{
	++mRefCount;
}

void MDocClosedNotifierImpl::Release()
{
	if (--mRefCount <= 0)
		delete this;
}

// --------------------------------------------------------------------

MDocClosedNotifier::MDocClosedNotifier(
	MDocClosedNotifierImpl*	inImpl)
	: mImpl(inImpl)
{
}

MDocClosedNotifier::MDocClosedNotifier(
	const MDocClosedNotifier&	inRHS)
{
	mImpl = inRHS.mImpl;
	mImpl->AddRef();
}

MDocClosedNotifier&	MDocClosedNotifier::operator=(
	const MDocClosedNotifier&	inRHS)
{
	if (mImpl != inRHS.mImpl)
	{
		mImpl->Release();
		mImpl = inRHS.mImpl;
		mImpl->AddRef();
	}
	
	return *this;	
}

MDocClosedNotifier::~MDocClosedNotifier()
{
	mImpl->Release();
}

bool MDocClosedNotifier::ReadSome(
	string&			outText)
{
	return mImpl->ReadSome(outText);
}
