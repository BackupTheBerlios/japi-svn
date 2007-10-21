#include "MJapieG.h"

#include <iostream>

#include "MFont.h"

using namespace std;

namespace {

class MFontMap
{
  public:
					MFontMap();
					~MFontMap();

	PangoFontDescription*
					GetFont(
						MStandardFontEnum	inFont);
  private:
	PangoFontMap*	mPangoFontMap;
};

MFontMap	gFontMap;

MFontMap::MFontMap()
	: mPangoFontMap(pango_cairo_font_map_get_default())
{
/*
	PangoFontFamily** families = nil;
	int cnt;
	
	pango_font_map_list_families(mPangoFontMap, &families, &cnt);

	if (families != nil)
	{
		for (uint32 fn = 0; fn < cnt; ++fn)
		{
			cout << pango_font_family_get_name(families[fn]) << endl;
		}
		g_free(families);
	}
*/
}

MFontMap::~MFontMap()
{
}

PangoFontDescription* MFontMap::GetFont(
	MStandardFontEnum	inFontNr)
{
	PangoFontDescription* d = nil;

	switch (inFontNr)
	{
		case eFixedFont:
			d = pango_font_description_from_string("Monospace 9");
			break;
		
		case eSansFont:
			d = pango_font_description_from_string("Sans 10");
			break;
	}

//	pango_font_description_set_size(d, 10 * PANGO_SCALE);

	return d;
}

}

MFont kFixedFont(eFixedFont);
MFont kSansFont(eSansFont);

MFont::MFont(
	const MFont&		inRHS)
	: mDesc(pango_font_description_copy(inRHS.mDesc))
{
}

MFont::MFont(
	const PangoFontDescription*
						inRHS)
	: mDesc(pango_font_description_copy(inRHS))
{
}

MFont::MFont(
	MStandardFontEnum	inFontNr)
	: mDesc(gFontMap.GetFont(inFontNr))
{
}

MFont::~MFont()
{
	pango_font_description_free(mDesc);
}

MFont& MFont::operator=(
	const MFont&			inRHS)
{
	if (this != &inRHS)
	{
		pango_font_description_free(mDesc);
		mDesc = pango_font_description_copy(inRHS.mDesc);
	}
	
	return *this;
}

MFont& MFont::operator=(
	const PangoFontDescription*
							inRHS)
{
	if (mDesc != inRHS)
	{
		pango_font_description_free(mDesc);
		mDesc = pango_font_description_copy(inRHS);
	}
	
	return *this;
}
