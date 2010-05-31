#ifndef CTESTVIEW_H
#define CTESTVIEW_H

#include "MView.h"

class CTestView : public MCanvas
{
public:
					CTestView(MRect inRect);

	virtual void	Draw(MRect inUpdate);
	virtual void	Click(int32 inX, int32 inY, uint32 inModifiers);
};

#endif