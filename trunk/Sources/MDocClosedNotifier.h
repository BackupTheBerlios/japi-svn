#ifndef MDOCCLOSEDNOTIFIER_H
#define MDOCCLOSEDNOTIFIER_H

class MDocClosedNotifier
{
  public:
						MDocClosedNotifier(
							int							inFD);
						
						MDocClosedNotifier(
							const MDocClosedNotifier&	inRHS);
	
	MDocClosedNotifier&	operator=(
							const MDocClosedNotifier&	inRHS);

						~MDocClosedNotifier();

  private:

	struct MDocClosedNotifierImp*						mImpl;
};

#endif
