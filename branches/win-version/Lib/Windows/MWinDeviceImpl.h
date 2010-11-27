//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINDEVICEIMPL_H
#define MWINDEVICEIMPL_H

#include <stack>
#include "MDeviceImpl.h"

// --------------------------------------------------------------------

class MWinDeviceImpl : public MDeviceImp
{
  public:
							MWinDeviceImpl();

							MWinDeviceImpl(
								MView*		inView,
								MRect		inBounds,
								bool		inOffscreen);

	static ID2D1Factory*	GetD2D1Factory();
	static IDWriteFactory*	GetDWFactory();
	static std::wstring		GetLocale();

	virtual					~MWinDeviceImpl();

	virtual void			Save();
	
	virtual void			Restore();

	virtual MRect			GetBounds() const						{ return MRect(0, 0, 100, 100); }

	virtual void			SetFont(
								const std::string&	inFont);

	virtual void			SetForeColor(
								MColor				inColor);

	virtual MColor			GetForeColor() const;

	virtual void			SetBackColor(
								MColor				inColor);

	virtual MColor			GetBackColor() const;
	
	virtual void			ClipRect(
								MRect				inRect);

	//virtual void			ClipRegion(
	//							MRegion				inRegion);

	virtual void			EraseRect(
								MRect				inRect);

	virtual void			FillRect(
								MRect				inRect);

	virtual void			StrokeRect(
								MRect				inRect,
								uint32				inLineWidth = 1);

	virtual void			FillEllipse(
								MRect				inRect);

	//virtual void			DrawImage(
	//							cairo_surface_t*	inImage,
	//							float				inX,
	//							float				inY,
	//							float				inShear);
	
	virtual void			CreateAndUsePattern(
								MColor				inColor1,
								MColor				inColor2);
	
	virtual float			GetAscent();
	
	virtual float			GetDescent();

	virtual int32			GetLineHeight();

	virtual float			GetXWidth();

	virtual void			DrawString(
								const std::string&	inText,
								float				inX,
								float				inY,
								uint32				inTruncateWidth = 0,
								MAlignment			inAlign = eAlignNone);

	// Text Layout options
	virtual void			SetText(
								const std::string&	inText);
	
	virtual void			SetTabStops(
								float				inTabWidth);
	
	virtual void			SetTextColors(
								uint32				inColorCount,
								uint32				inColorIndices[],
								uint32				inOffsets[],
								MColor				inColors[]);

	virtual void			SetTextSelection(
								uint32				inStart,
								uint32				inLength,
								MColor				inSelectionColor);
	
	virtual void			IndexToPosition(
								uint32				inIndex,
								bool				inTrailing,
								int32&				outPosition);

	virtual bool			PositionToIndex(
								int32				inPosition,
								uint32&				outIndex);
	
	virtual float			GetTextWidth();
	
	virtual void			DrawText(
								float				inX,
								float				inY);

	virtual void			DrawCaret(
								float				inX,
								float				inY,
								uint32				inOffset);
	
	virtual void			BreakLines(
								uint32				inWidth,
								std::vector<uint32>&
													outBreaks);

	virtual void			MakeTransparent(
								float				inOpacity) {}

	//virtual GdkPixmap*		GetPixmap() const		{ return nil; }

	virtual void			SetDrawWhiteSpace(
								bool				inDrawWhiteSpace) {}

  protected:

	void					CreateTextFormat();
	void					LookupFont(
								const std::wstring&	inFamily);

	uint32					MapBack(
								uint32				inOffset);

	MView*					mView;
//	HDC						mDC;

	ID2D1RenderTarget*		mRenderTarget;
//	ID2D1Layer*				mClipLayer;
	std::stack<MRect>		mClipping;
	IDWriteTextFormat*		mTextFormat;
	IDWriteTextLayout*		mTextLayout;
	ID2D1Brush*				mForeBrush;
	ID2D1Brush*				mBackBrush;

	std::wstring			mFontFamily;
	float					mFontSize;
	IDWriteFont*			mFont;

	// converted text (from UTF8 to UTF16)
	std::wstring			mText;
	std::vector<uint16>		mTextIndex;		// from string to wstring
	MColor					mSelectionColor;
	uint32					mSelectionStart, mSelectionLength;

	float					mDpiScaleX, mDpiScaleY;
	std::stack<ID2D1DrawingStateBlock*>
							mState;
};

#endif
