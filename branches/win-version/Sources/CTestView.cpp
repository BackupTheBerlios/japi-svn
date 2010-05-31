#include "MJapi.h"
#include "CTestView.h"
#include "MDevice.h"
#include "MControls.h"
#include "MApplication.h"
#include "MCommands.h"

CTestView::CTestView(MRect inRect)
	: MView('test', inRect)
	, eAction(this, &CTestView::Action)
{
}

void CTestView::Draw(MRect inUpdate)
{
	MRect bounds;
	GetBounds(bounds);

	MColor c1("#efff7f"), c2("#ffffcc"), c3("#ffd281");
	MDevice dev(this, bounds, false);

	dev.EraseRect(bounds);

	MRect r(0, 0, 10, 10);

	dev.SetForeColor(c3);
	while (r.y < bounds.y + bounds.height and
		   r.x < bounds.x + bounds.width)
	{
		dev.FillRect(r);
		r.x += 10;
		r.y += 10;
	}

	dev.SetForeColor(kBlack);
	dev.DrawString("Hallo, wereld!", 10, 10);

	dev.SetForeColor(c3);
	dev.FillRect(MRect(10, 60, 50, 50));

	dev.SetForeColor(c2);
	dev.FillEllipse(MRect(100, 100, 50, 14));

	dev.CreateAndUsePattern(c1, c2);
	dev.FillEllipse(MRect(100, 128, 50, 14));

}

void CTestView::Action()
{
	Invalidate();
}
