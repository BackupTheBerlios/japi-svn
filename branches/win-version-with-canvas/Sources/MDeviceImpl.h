//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MDEVICEIMPL_H
#define MDEVICEIMPL_H

#include "MTypes.h"

// --------------------------------------------------------------------
// base class for MDeviceImp

class MDeviceImp
{
  public:
							MDeviceImp()									{}
	virtual					~MDeviceImp()									{}

	virtual void			Save()											{}
	virtual void			Restore()										{}

	virtual bool			IsPrinting(
								int32&				outPage) const			{ return false; }

	virtual MRect			GetBounds() const								{ return MRect(0, 0, 100, 100); }

	virtual void			SetOrigin(
								int32				inX,
								int32				inY)					{}

	virtual void			SetFont(
								const std::string&	inFont)					{}

	virtual void			SetForeColor(
								MColor				inColor)				{}

	virtual MColor			GetForeColor() const							{ return kBlack;}

	virtual void			SetBackColor(
								MColor				inColor)				{}

	virtual MColor			GetBackColor() const							{ return kWhite; }
	
	virtual void			ClipRect(
								MRect				inRect)					{}

	//virtual void			ClipRegion(
	//							MRegion				inRegion)				{}

	virtual void			EraseRect(
								MRect				inRect)					{}

	virtual void			FillRect(
								MRect				inRect)					{}

	virtual void			StrokeRect(
								MRect				inRect,
								uint32				inLineWidth = 1)		{}

	virtual void			FillEllipse(
								MRect				inRect)					{}

	//virtual void			DrawImage(
	//							cairo_surface_t*	inImage,
	//							float				inX,
	//							float				inY,
	//							float				inShear)				{}
	
	virtual void			CreateAndUsePattern(
								MColor				inColor1,
								MColor				inColor2)				{}
	
	//PangoFontMetrics*		GetMetrics();

	virtual uint32			GetAscent()										{ return 10; }
	
	virtual uint32			GetDescent()									{ return 2; }
	
	virtual uint32			GetLeading()									{ return 0; }
	
	virtual uint32			GetLineHeight()									{ return GetAscent() + GetDescent() + GetLeading(); }

	virtual void			DrawString(
								const std::string&	inText,
								float				inX,
								float				inY,
								uint32				inTruncateWidth = 0,
								MAlignment			inAlign = eAlignNone)	{}

	virtual uint32			GetStringWidth(
								const std::string&	inText)					{ return 0; }

	// Text Layout options
	
	virtual void			SetText(
								const std::string&	inText)					{}
	
	virtual void			SetTabStops(
								uint32				inTabWidth)				{}
	
	virtual void			SetTextColors(
								uint32				inColorCount,
								uint32				inColors[],
								uint32				inOffsets[])			{}

	virtual void			SetTextSelection(
								uint32				inStart,
								uint32				inLength,
								MColor				inSelectionColor)		{}
	
	virtual void			IndexToPosition(
								uint32				inIndex,
								bool				inTrailing,
								int32&				outPosition)			{}

	virtual bool			PositionToIndex(
								int32				inPosition,
								uint32&				outIndex)				{ return false; }
	
	virtual uint32			GetTextWidth()									{ return 0; }
	
	virtual void			DrawText(
								float				inX,
								float				inY)					{}

	virtual void			DrawCaret(
								float				inX,
								float				inY,
								uint32				inOffset)				{}
	
	virtual void			BreakLines(
								uint32				inWidth,
								std::vector<uint32>&
													outBreaks)				{}

	virtual void			MakeTransparent(
								float				inOpacity)				{}

	//virtual GdkPixmap*		GetPixmap() const							{ return nil; }

	virtual void			SetDrawWhiteSpace(
								bool				inDrawWhiteSpace)		{}

	static MDeviceImp*		Create();
	static MDeviceImp*		Create(
								MCanvas*			inCanvas,
								MRect				inRect,
								bool				inCreateOffscreen);
};

#endif