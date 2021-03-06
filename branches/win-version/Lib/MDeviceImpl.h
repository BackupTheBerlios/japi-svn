//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MDEVICEIMPL_H
#define MDEVICEIMPL_H

#include "MDevice.h"

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

	virtual float			GetAscent()										{ return 10; }
	
	virtual float			GetDescent()									{ return 2; }
	
	virtual float			GetLeading()									{ return 0; }
	
	virtual int32			GetLineHeight()
							{
								return static_cast<int32>(
									std::ceil(GetAscent() + GetDescent() + GetLeading()));
							}

	virtual float			GetXWidth()										{ return 8; }

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
								float				inTabWidth)				{}
	
	virtual void			SetTextColors(
								uint32				inColorCount,
								uint32				inColorIndices[],
								uint32				inOffsets[],
								MColor				inColors[])				{}

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
	
	virtual float			GetTextWidth()									{ return 0; }
	
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
								MView*				inView,
								MRect				inRect,
								bool				inCreateOffscreen);
};

#endif
