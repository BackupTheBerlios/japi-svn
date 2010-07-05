//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCANVAS_H
#define MCANVAS_H

#include "MView.h"

class MCanvasImpl;

class MCanvas : public MView
{
  public:
					MCanvas(
						const std::string&
										inID,
						MRect			inBounds);
	virtual			~MCanvas();
	
	virtual void	ResizeFrame(
						int32			inXDelta,
						int32			inYDelta,
						int32			inWidthDelta,
						int32			inHeightDelta);

	virtual void	AddedToWindow();

	MCanvasImpl*	GetImpl() const							{ return mImpl; }

  protected:

	MCanvasImpl*	mImpl;
};

#endif

