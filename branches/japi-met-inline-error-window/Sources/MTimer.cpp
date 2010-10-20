//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include "MTimer.h"

struct MTimeOutImp
{
					MTimeOutImp(
						MTimeOut*	inTimeOut);

					~MTimeOutImp();

	void			Start(
						double		inDurationInSeconds);
	void			Stop();

	MTimeOut*		mTimeOut;
	guint			mTag;

	static gboolean TimedOutCB(
						gpointer	inData);
};

MTimeOutImp::MTimeOutImp(
	MTimeOut*	inTimeOut)
	: mTimeOut(inTimeOut)
	, mTag(0)
{
}

MTimeOutImp::~MTimeOutImp()
{
	Stop();
}

void MTimeOutImp::Start(
	double		inDurationInSeconds)
{
	Stop();
	
	mTag = g_timeout_add(
		static_cast<guint>(inDurationInSeconds * 1000),
		&MTimeOutImp::TimedOutCB, this);
}

void MTimeOutImp::Stop()
{
	if (mTag > 0)
		g_source_remove(mTag);
	mTag = 0;
}

gboolean MTimeOutImp::TimedOutCB(
	gpointer	inData)
{
	MTimeOutImp* timeOutImp = reinterpret_cast<MTimeOutImp*>(inData);
	
	timeOutImp->mTag = 0;

	timeOutImp->mTimeOut->eTimedOut();
	
	return false;
}

//---------------------------------------------------------------------

MTimeOut::MTimeOut()
{
	mImpl = new MTimeOutImp(this);
}
		
MTimeOut::~MTimeOut()
{
	delete mImpl;
}

void MTimeOut::Start(
	double		inDurationInSeconds)
{
	mImpl->Start(inDurationInSeconds);
}

void MTimeOut::Stop()
{
	mImpl->Stop();
}
