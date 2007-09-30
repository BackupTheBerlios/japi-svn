#include "MJapieG.h"

#include <cmath>

#include "MDevice.h"
#include "MView.h"

using namespace std;

struct MDeviceImp
{
				MDeviceImp(
					MView*		inView,
					MRect		inRect);
	
				~MDeviceImp();
	
	MView*		mView;
	MRect		mRect;
	MColor		mForeColor;
	MColor		mBackColor;
//	MWindow*	mWindow;
	cairo_t*	mContext;
	uint32		mPatternData[8][8];
};

MDeviceImp::MDeviceImp(
	MView*		inView,
	MRect		inRect)
	: mView(inView)
	, mRect(inRect)
	, mForeColor(kBlack)
	, mBackColor(kWhite)
{
	GtkWidget* widget = mView->GetGtkWidget();
	
	mContext = gdk_cairo_create(widget->window);
	
	cairo_rectangle(mContext, inRect.x, inRect.y, inRect.width, inRect.height);
	cairo_clip(mContext);
}

MDeviceImp::~MDeviceImp()
{
	cairo_destroy(mContext);
}

// -------------------------------------------------------------------

MDevice::MDevice(
	MView*		inView,
	MRect		inRect)
	: mImpl(new MDeviceImp(inView, inRect))
{
}

MDevice::~MDevice()
{
	delete mImpl;
}

void MDevice::SetForeColor(
	MColor		inColor)
{
	mImpl->mForeColor = inColor;

	cairo_set_source_rgb(mImpl->mContext, mImpl->mForeColor.red,
		mImpl->mForeColor.green, mImpl->mForeColor.blue);
}

MColor MDevice::GetForeColor() const
{
	return mImpl->mForeColor;
}

void MDevice::SetBackColor(
	MColor		inColor)
{
	mImpl->mBackColor = inColor;
}

MColor MDevice::GetBackColor() const
{
	return mImpl->mBackColor;
}

void MDevice::EraseRect(
	MRect		inRect)
{
	cairo_save(mImpl->mContext);
	
	cairo_rectangle(mImpl->mContext, inRect.x, inRect.y, inRect.width, inRect.height);
	cairo_set_source_rgb(mImpl->mContext, mImpl->mBackColor.red,
		mImpl->mBackColor.green, mImpl->mBackColor.blue);
	cairo_fill(mImpl->mContext);
	
	cairo_restore(mImpl->mContext);
}

void MDevice::FillRect(
	MRect		inRect)
{
	cairo_rectangle(mImpl->mContext, inRect.x, inRect.y, inRect.width, inRect.height);
	cairo_fill(mImpl->mContext);
}

void MDevice::FillEllipse(
	MRect		inRect)
{
	cairo_translate(mImpl->mContext, inRect.x + inRect.width / 2., inRect.y + inRect.height / 2.);
	cairo_scale(mImpl->mContext, inRect.width / 2., inRect.height / 2.);
	cairo_arc(mImpl->mContext, 0., 0., 1., 0., 2 * M_PI);
	cairo_fill(mImpl->mContext);
}

void MDevice::CreateAndUsePattern(
	MColor		inColor1,
	MColor		inColor2)
{
	uint32 c1 = 0, c2 = 0;
	
	c1 |= (uint32(inColor1.red * 255) & 0xFF) << 16;
	c1 |= (uint32(inColor1.green * 255) & 0xFF) << 8;
	c1 |= (uint32(inColor1.blue * 255) & 0xFF) << 0;
	
	c2 |= (uint32(inColor2.red * 255) & 0xFF) << 16;
	c2 |= (uint32(inColor2.green * 255) & 0xFF) << 8;
	c2 |= (uint32(inColor2.blue * 255) & 0xFF) << 0;
	
	for (uint32 y = 0; y < 8; ++y)
	{
		for (uint32 x = 0; x < 4; ++x)
			mImpl->mPatternData[y][x] = c1;
		for (uint32 x = 4; x < 8; ++x)
			mImpl->mPatternData[y][x] = c2;
	}
	
	cairo_surface_t* s = cairo_image_surface_create_for_data(
		reinterpret_cast<uint8*>(mImpl->mPatternData), CAIRO_FORMAT_RGB24, 8, 8, 0);

	if (s != nil)
	{
		cairo_pattern_t* p = cairo_pattern_create_for_surface(s);
		cairo_pattern_set_extend(p, CAIRO_EXTEND_REPEAT);
		
		if (p != nil)
		{
			cairo_matrix_t m;
			cairo_matrix_init_rotate(&m, 2.356);
			cairo_pattern_set_matrix(p, &m);
			
			cairo_set_source(mImpl->mContext, p);
			
			FillRect(MRect(40, 40, 20, 20));
			
			cairo_pattern_destroy(p);
		}
		
		cairo_surface_destroy(s);
	}
}

uint32 MDevice::GetAscent() const
{
}

uint32 MDevice::GetDescent() const
{
}

uint32 MDevice::GetLeading() const
{
}

void MDevice::DrawString(
	const std::string&	inText,
	float inX,
	float inY)
{
}



