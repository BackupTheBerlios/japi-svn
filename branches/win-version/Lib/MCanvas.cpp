//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include "MCanvasImpl.h"

using namespace std;

MCanvas::MCanvas(
	const string&	inID,
	MRect			inBounds)
	: MView(inID, inBounds)
	, mImpl(MCanvasImpl::Create(this))
{
}

MCanvas::~MCanvas()
{
	delete mImpl;
}

void MCanvas::ResizeFrame(
	int32			inXDelta,
	int32			inYDelta,
	int32			inWidthDelta,
	int32			inHeightDelta)
{
	mImpl->ResizeFrame(inXDelta, inYDelta, inWidthDelta, inHeightDelta);
}

void MCanvas::AddedToWindow()
{
	mImpl->AddedToWindow();
}

