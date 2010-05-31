//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCANVASIMPL_H
#define MCANVASIMPL_H

class MCanvas;

class MCanvasImpl
{
public:
					MCanvasImpl(MCanvas* inCanvas)
						: mCanvas(inCanvas) {}
	virtual			~MCanvasImpl() {}

	static MCanvasImpl*		Create(MCanvas* inCanvas);

protected:
	MCanvas*		mCanvas;
};

#endif