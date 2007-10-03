#include "Japie.h"

#include <deque>

#include <boost/thread.hpp>

#include "MMessageQueue.h"

using namespace std;

struct MMessageQueueImpl
{
	deque<MMessage*>	queue;
	boost::mutex		mutex;
};

MMessageQueue::MMessageQueue()
	: mImpl(new MMessageQueueImpl)
{
}

MMessageQueue::~MMessageQueue()
{
	boost::mutex::scoped_lock lock(mImpl->mutex);

	for (deque<MMessage*>::iterator m = mImpl->queue.begin(); m != mImpl->queue.end(); ++m)
		delete *m;
}

void MMessageQueue::PushMessage(
	MMessage*	inMessage)
{
	boost::mutex::scoped_lock lock(mImpl->mutex);
	
	mImpl->queue.push_back(inMessage);
}

MMessage* MMessageQueue::PopMessage()
{
	boost::mutex::scoped_lock lock(mImpl->mutex);
	
	MMessage* result = nil;
	
	if (mImpl->queue.size() > 0)
	{
		result = mImpl->queue.front();
		mImpl->queue.pop_front();
	}
	
	return result;
}

