//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MDEVICE_H
#define MDEVICE_H

#include <vector>

#include "MTypes.h"
#include "MColor.h"

class MView;
class MTextLayout;

enum MAlignment {
	eAlignNone,
	eAlignLeft,
	eAlignCenter,
	eAlignRight	
};

class MDevice
{
  public:
  					// create a dummy device
					MDevice();

					// a regular device, used for drawing in a view
					// if inCreateOffscreen is true, drawing is done
					// in a GdkPixmap instead.
					MDevice(
						MView*				inView,
						MRect				inRect,
						bool				inCreateOffscreen = false);
					
					~MDevice();

	void			Save();
	
	void			Restore();

	bool			IsPrinting() const;
	
	int32			GetPageNr() const;

	MRect			GetBounds() const;

	void			SetFont(
						const std::string&	inFont);

	void			SetForeColor(
						MColor				inColor);

	MColor			GetForeColor() const;

	void			SetBackColor(
						MColor				inColor);

	MColor			GetBackColor() const;
	
	void			ClipRect(
						MRect				inRect);
	
	//void			ClipRegion(
	//					MRegion				inRegion);

	void			EraseRect(
						MRect				inRect);

	void			FillRect(
						MRect				inRect);

	void			StrokeRect(
						MRect				inRect,
						uint32				inLineWidth = 1);

	void			FillEllipse(
						MRect				inRect);

	//void			DrawImage(
	//					cairo_surface_t*	inImage,
	//					float				inX,
	//					float				inY,
	//					float				inShear);
	
	void			CreateAndUsePattern(
						MColor				inColor1,
						MColor				inColor2);
	
	float			GetAscent() const;
	
	float			GetDescent() const;
	
	float			GetLeading() const;
	
	int32			GetLineHeight() const;

	float			GetXWidth() const;

	void			DrawString(
						const std::string&	inText,
						float				inX,
						float				inY,
						uint32				inTruncateWidth = 0,
						MAlignment			inAlign = eAlignNone);

	// Text Layout options
	void			SetText(
						const std::string&	inText);
	
	float			GetTextWidth() const;

	void			SetTabStops(
						float				inTabWidth);
	
	void			SetTextColors(
						uint32				inColorCount,
						uint32				inColorIndices[],
						uint32				inOffsets[],
						MColor				inColors[]);

	void			SetTextSelection(
						uint32				inStart,
						uint32				inLength,
						MColor				inSelectionColor);
	
	void			IndexToPosition(
						uint32				inIndex,
						bool				inTrailing,
						int32&				outPosition);

//	bool			PositionToIndex(
//						int32				inPosition,
//						uint32&				outIndex,
//						bool&				outTrailing);

	bool			PositionToIndex(
						int32				inPosition,
						uint32&				outIndex);
	
	void			DrawText(
						float				inX,
						float				inY);

	void			DrawCaret(
						float				inX,
						float				inY,
						uint32				inOffset);
	
	void			BreakLines(
						uint32				inWidth,
						std::vector<uint32>&
											outBreaks);

	void			MakeTransparent(
						float				inOpacity);

	//GdkPixmap*		GetPixmap() const;

	void			SetDrawWhiteSpace(
						bool				inDrawWhiteSpace);
	
	static void		GetSysSelectionColor(
						MColor&				outColor);

  private:

					MDevice(const MDevice&);
	MDevice&		operator=(const MDevice&);

	class MDeviceImp*	mImpl;
};

class MDeviceContextSaver
{
  public:
				MDeviceContextSaver(
					MDevice&		inDevice)
					: mDevice(inDevice)
				{
					mDevice.Save();
				}
				
				~MDeviceContextSaver()
				{
					mDevice.Restore();
				}

  private:
	MDevice&	mDevice;
};

#endif
