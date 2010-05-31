#ifndef CTESTVIEW_H
#define CTESTVIEW_H

#include "MView.h"

class CTestView : public MView
{
public:
					CTestView(MRect inRect);

	virtual void	Draw(MRect inUpdate);
	virtual void	Action();

	MEventIn<void()>
					eAction;
};

#endif