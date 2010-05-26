#include "MJapi.h"
#include "CTestView.h"
#include "MDevice.h"
#include "MControls.h"

CTestView::CTestView(MRect inRect)
	: MView('test', inRect)
{
}

void CTestView::Draw(MRect inUpdate)
{
	MRect bounds;
	GetBounds(bounds);

	MDevice dev(this, bounds, false);

	dev.DrawString("Hallo, wereld!", 10, 5);

	MColor c1("#efff7f"), c2("#ffffcc"), c3("#ffd281");

//			dev.FillRect(MRect(10, 10, 50, 50));

	dev.SetForeColor(c3);
	dev.FillRect(MRect(10, 60, 50, 50));

	dev.SetForeColor(c2);
	dev.FillEllipse(MRect(100, 100, 50, 14));

	dev.CreateAndUsePattern(c1, c2);
	dev.FillEllipse(MRect(100, 128, 50, 14));

	MRect r = bounds;
	r.x += bounds.width - 10;
	r.y += bounds.height - 10;
	r.width = 10;
	r.height = 10;

	if (r)
	{
		dev.SetForeColor(c1);
		dev.FillRect(r);
	}
}

void CTestView::Click(int32 inX, int32 inY, uint32 inModifiers)
{

}
