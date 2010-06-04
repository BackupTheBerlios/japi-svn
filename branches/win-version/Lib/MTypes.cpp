//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"
#include "MTypes.h"

void MRect::InsetBy(
	int32				inDeltaX,
	int32				inDeltaY)
{
	if (inDeltaX < 0 or 2 * inDeltaX <= width)
	{
		x += inDeltaX;
		width -= inDeltaX * 2;
	}
	else
	{
		x += width / 2;
		width = 0;
	}

	if (inDeltaY < 0 or 2 * inDeltaY <= height)
	{
		y += inDeltaY;
		height -= inDeltaY * 2;
	}
	else
	{
		y += height / 2;
		height = 0;
	}
}

MRect MRect::operator&(
	const MRect&		inRhs)
{
	MRect result(*this);
	result &= inRhs;
	return result;
}

MRect& MRect::operator&=(
	const MRect&		inRhs)
{
	int32 nx = x;
	if (nx < inRhs.x)
		nx = inRhs.x;

	int32 ny = y;
	if (ny < inRhs.y)
		ny = inRhs.y;
	
	int32 nx2 = x + width;
	if (nx2 > inRhs.x + inRhs.width)
		nx2 = inRhs.x + inRhs.width;
	
	int32 ny2 = y + height;
	if (ny2 > inRhs.y + inRhs.height)
		ny2 = inRhs.y + inRhs.height;
	
	x = nx;
	y = ny;

	width = nx2 - nx;
	if (width < 0)
		width = 0;

	height = ny2 - ny;
	if (height < 0)
		height = 0;

	return *this;
}

inline MRect& MRect::operator|=(
	const MRect&		inRhs)
{
	int32 nx = x;
	if (nx > inRhs.x)
		nx = inRhs.x;

	int32 ny = y;
	if (ny > inRhs.y)
		ny = inRhs.y;
	
	int32 nx2 = x + width;
	if (nx2 < inRhs.x + inRhs.width)
		nx2 = inRhs.x + inRhs.width;
	
	int32 ny2 = y + height;
	if (ny2 < inRhs.y + inRhs.height)
		ny2 = inRhs.y + inRhs.height;
	
	x = nx;
	y = ny;

	width = nx2 - nx;
	if (width < 0)
		width = 0;

	height = ny2 - ny;
	if (height < 0)
		height = 0;

	return *this;
}

MRect MRect::operator|(
	const MRect&		inRhs)
{
	MRect result(*this);
	result |= inRhs;
	return result;
}

MRect::operator bool() const
{
	return width > 0 and height > 0;
}
