/*
	Copyright (c) 2007, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MDEVICE_H
#define MDEVICE_H

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
					MDevice();

					MDevice(
						MView*				inView,
						MRect				inRect);

					~MDevice();

	void			Save();
	
	void			Restore();

	void			SetFont(
						const std::string&	inFont);

	void			SetForeColor(
						MColor				inColor);

	MColor			GetForeColor() const;

	void			SetBackColor(
						MColor				inColor);

	MColor			GetBackColor() const;
	
	void			EraseRect(
						MRect				inRect);

	void			FillRect(
						MRect				inRect);

	void			StrokeRect(
						MRect				inRect,
						uint32				inLineWidth = 1);

	void			FillEllipse(
						MRect				inRect);
	
	void			CreateAndUsePattern(
						MColor				inColor1,
						MColor				inColor2);
	
	void			DrawListItemBackground(
						MRect				inRect,
						bool				inSelected,
						bool				inActive,
						bool				inOdd,
						bool				inRoundEdges);

	uint32			GetAscent() const;
	
	uint32			GetDescent() const;
	
	uint32			GetLeading() const;
	
	uint32			GetLineHeight() const;

	void			DrawString(
						const std::string&	inText,
						float				inX,
						float				inY,
						uint32				inTruncateWidth = 0,
						MAlignment			inAlign = eAlignNone);

	uint32			GetStringWidth(
						const std::string&	inText);

	// Text Layout options
	
	void			SetText(
						const std::string&	inText);
	
	void			SetTabStops(
						uint32				inTabWidth);
	
	void			SetTextColors(
						uint32				inColorCount,
						uint32				inColors[],
						uint32				inOffsets[]);

	void			SetTextSelection(
						uint32				inStart,
						uint32				inLength,
						MColor				inSelectionColor);
	
	void			IndexToPosition(
						uint32				inIndex,
						bool				inTrailing,
						int32&				outPosition);

	bool			PositionToIndex(
						int32				inPosition,
						uint32&				outIndex,
						bool&				outTrailing);
	
	uint32			GetTextWidth();
	
	void			DrawText(
						float				inX,
						float				inY);

	void			DrawCaret(
						float				inX,
						float				inY,
						uint32				inOffset);
	
	bool			BreakLine(
						uint32				inWidth,
						uint32&				outBreak);
	
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
