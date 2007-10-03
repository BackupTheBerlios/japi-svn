#ifndef MMESSAGEQUEUE_H
#define MMESSAGEQUEUE_H

class MMessage
{
  public:
				MMessage() {}
	virtual		~MMessage() {}

  private:
				MMessage(const MMessage&);
	MMessage&	operator=(const MMessage&);
};

class MMessageQueue
{
  public:
				MMessageQueue();
	virtual		~MMessageQueue();
	
	void		PushMessage(
					MMessage*	inMessage);
	
	MMessage*	PopMessage();

  private:
	struct MMessageQueueImpl*	mImpl;
};

#endif
