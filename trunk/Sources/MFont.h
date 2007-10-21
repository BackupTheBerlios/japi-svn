#ifndef MFONT_H
#define MFONT_H

enum MStandardFontEnum {
	eFixedFont,
	eSansFont
};

class MFont
{
  public:
			MFont(
				const MFont&			inRHS);

			MFont(
				MStandardFontEnum		inFont);
			
			MFont(
				const PangoFontDescription*
										inDesc);

			~MFont();
	
	MFont&	operator=(
				const MFont&			inRHS);

	MFont&	operator=(
				const PangoFontDescription*
										inRHS);

			operator PangoFontDescription*() const	{ return mDesc; }

	uint32	GetAscent() const;
	uint32	GetDescent() const;

  private:
	PangoFontDescription*				mDesc;
};

extern MFont
	kFixedFont,
	kSansFont;

#endif
