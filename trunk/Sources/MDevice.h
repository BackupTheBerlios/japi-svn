#ifndef MDEVICE_H
#define MDEVICE_H

#include "MTypes.h"
#include "MColor.h"

class MView;
class MTextLayout;

class MDevice
{
  public:
					MDevice(
						MView*			inView,
						MRect			inRect);

					MDevice(
						MTextLayout*	inTextLayout);

					~MDevice();

	void			SetForeColor(
						MColor			inColor);

	MColor			GetForeColor() const;

	void			SetBackColor(
						MColor			inColor);

	MColor			GetBackColor() const;
	
	void			EraseRect(
						MRect			inRect);

	void			FillRect(
						MRect			inRect);

	void			FillEllipse(
						MRect			inRect);
	
	void			CreateAndUsePattern(
						MColor			inColor1,
						MColor			inColor2);
	
	uint32			GetAscent() const;
	
	uint32			GetDescent() const;
	
	uint32			GetLeading() const;

	void			DrawString(
						const std::string&	
										inText,
						float			inX,
						float			inY);
	
//	void			UseTextLayout(ATSUTextLayout inTextLayout);
//	void			DrawText(float inX, float inY, uint32 inTextStart, uint32 inTextLength);
//	void			DrawTextHighLight(float inX, float inY,
//						uint32 inSelectionStart, uint32 inSelectionLength,
//						bool inFillLeft, bool inFillRight, bool inActive);

  private:

					MDevice(const MDevice&);
	MDevice&		operator=(const MDevice&);

	struct MDeviceImp*	mImpl;
};

#endif
