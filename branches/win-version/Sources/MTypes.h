//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// MTypes.h

#ifndef MTYPES_H
#define MTYPES_H

#define nil NULL

typedef signed char			int8;
typedef unsigned char		uint8;
typedef signed short		int16;
typedef unsigned short		uint16;
typedef signed int			int32;
typedef unsigned int		uint32;
typedef signed long long	int64;
typedef unsigned long long	uint64;

typedef uint32				unicode;

struct MRect
{
	int32		x, y;
	int32		width, height;
	
				MRect();

				MRect(
					const MRect&		inRHS);

				MRect(
					int32				inX,
					int32				inY,
					int32				inWidth,
					int32				inHeight);

	bool		Intersects(
					const MRect&		inRHS) const;
	
	bool		ContainsPoint(
					int32				inX,
					int32				inY) const;
	
	void		InsetBy(
					int32				inDeltaX,
					int32				inDeltaY);
};

//struct MRegion
//{
//				MRegion()
//					: mGdkRegion(gdk_region_new()) {}
//	
//				MRegion(
//					const MRegion&	inRegion)
//					: mGdkRegion(gdk_region_copy(inRegion.mGdkRegion))
//				{
//				}
//
//	MRegion&	operator=(
//					const MRegion&	inRegion)
//				{
//					gdk_region_destroy(mGdkRegion);
//					mGdkRegion = gdk_region_copy(inRegion.mGdkRegion);
//					return *this;
//				}
//	
//				~MRegion()
//				{
//					gdk_region_destroy(mGdkRegion);
//				}
//				
//	MRegion&	operator+=(const MRect& r)
//				{
//					MRect& c = const_cast<MRect&>(r);
//					gdk_region_union_with_rect(mGdkRegion, c);
//					return *this;
//				}
//
//	bool		ContainsPoint(
//					int32				inX,
//					int32				inY) const
//				{
//					return gdk_region_point_in(mGdkRegion, inX, inY);
//				}
//	
//	void		OffsetBy(
//					int32				inX,
//					int32				inY)
//				{
//					gdk_region_offset(mGdkRegion, inX, inY);
//				}
//	
//				operator const GdkRegion* () const
//				{
//					return mGdkRegion;
//				}
//
//  private:
//	GdkRegion*	mGdkRegion;	
//};

enum MDirection
{
	kDirectionForward = 1,
	kDirectionBackward = -1
};

enum MScrollMessage
{
	kScrollNone,
	kScrollToStart,
	kScrollToEnd,
	kScrollToCaret,
	kScrollToSelection,
	kScrollCenterSelection,
	kScrollLineUp,
	kScrollLineDown,
	kScrollPageUp,
	kScrollPageDown,
	kScrollForKiss,
	kScrollReturnAfterKiss,
	kScrollForDiff,
	kScrollToPC
};

extern const char kHexChars[];

template<class T>
class value_changer
{
  public:
			value_changer(T& inVariable, const T inTempValue);
			~value_changer();
  private:
	T&		mVariable;
	T		mValue;
};

template<class T>
inline
value_changer<T>::value_changer(
	T&		inVariable,
	const T	inTempValue)
	: mVariable(inVariable)
	, mValue(inVariable)
{
	mVariable = inTempValue;
}

template<class T>
inline
value_changer<T>::~value_changer()
{
	mVariable = mValue;
}

// --------------------------------------------------------------------
// inlines

inline
MRect::MRect()
	: x(0), y(0), width(0), height(0) {}

inline
MRect::MRect(
	const MRect&		inRHS)
	: x(inRHS.x)
	, y(inRHS.y)
	, width(inRHS.width)
	, height(inRHS.height)
{
}

//inline
//MRect::MRect(
//	const GdkRectangle&	inRHS)
//	: x(inRHS.x)
//	, y(inRHS.y)
//	, width(inRHS.width)
//	, height(inRHS.height)
//{
//}

inline
MRect::MRect(
	int32		inX,
	int32		inY,
	int32		inWidth,
	int32		inHeight)
	: x(inX)
	, y(inY)
	, width(inWidth)
	, height(inHeight)
{
}

inline bool MRect::Intersects(
	const MRect&		inRHS) const
{
	return
		x <= inRHS.x + inRHS.width and
		x + width >= inRHS.x and
		y <= inRHS.y + inRHS.height and
		y + height >= inRHS.y;
}

inline bool MRect::ContainsPoint(
	int32				inX,
	int32				inY) const
{
	return
		x <= inX and x + width > inX and
		y <= inY and y + height > inY;
}

inline void MRect::InsetBy(
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

#endif
