#ifndef MDEVICE_H
#define MDEVICE_H

#include "MTypes.h"
#include "MColor.h"

class MView;
class MTextLayout;

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

	void			FillEllipse(
						MRect				inRect);
	
	void			CreateAndUsePattern(
						MColor				inColor1,
						MColor				inColor2);
	
	uint32			GetAscent() const;
	
	uint32			GetDescent() const;
	
	uint32			GetLeading() const;
	
	uint32			GetLineHeight() const;

	void			DrawString(
						const std::string&	inText,
						float				inX,
						float				inY);
	
	uint32			GetStringWidth(
						const std::string&	inText);

	// Text Layout options
	
	void			SetText(
						const std::string&	inText);
	
	void			SetTabStops(
						float				inTabWidth);
	
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
	
	void			DrawText(
						float				inX,
						float				inY);

	void			DrawCaret(
						uint32				inOffset);
	
  private:

					MDevice(const MDevice&);
	MDevice&		operator=(const MDevice&);

	struct MDeviceImp*	mImpl;
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
