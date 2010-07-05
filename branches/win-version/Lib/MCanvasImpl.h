//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCANVASIMPL_H
#define MCANVASIMPL_H

#include "MCanvas.h"

class MCanvasImpl
{
  public:
					MCanvasImpl(
						MCanvas*		inCanvas)
						: mCanvas(inCanvas)			{}
	
	virtual void	ResizeFrame(
						int32			inXDelta,
						int32			inYDelta,
						int32			inWidthDelta,
						int32			inHeightDelta) = 0;

	virtual void	AddedToWindow() = 0;

	static MCanvasImpl*
					Create(
						MCanvas*		inCanvas);

  protected:

	MCanvas*		mCanvas;

  private:
					MCanvasImpl(
						const MCanvasImpl&);
	MCanvasImpl&	operator=(const MCanvasImpl&);
};

#endif

