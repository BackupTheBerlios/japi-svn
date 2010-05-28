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

	dev.EraseRect(bounds);

	dev.DrawString("Hallo, wereld!", 10, 5);

	MColor c1("#efff7f"), c2("#ffffcc"), c3("#ffd281");

	dev.SetForeColor(c3);
	dev.FillRect(MRect(10, 60, 50, 50));

	dev.SetForeColor(c2);
	dev.FillEllipse(MRect(100, 100, 50, 14));

	dev.CreateAndUsePattern(c1, c2);
	dev.FillEllipse(MRect(100, 128, 50, 14));

	MRect r(120, 120, 10, 10);

	dev.SetForeColor(c1);
	while (r & bounds)
	{
		dev.FillRect(r);
		r.x += 10;
		r.y += 10;
	}
}

void CTestView::Click(int32 inX, int32 inY, uint32 inModifiers)
{

}
