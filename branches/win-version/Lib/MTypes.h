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

				// Intersection
	MRect		operator&(
					const MRect&		inRegion);
	MRect&		operator&=(
					const MRect&		inRegion);

				// Union
	MRect		operator|(
					const MRect&		inRegion);
	MRect&		operator|=(
					const MRect&		inRegion);

				// test for empty rectangle
				operator bool() const;
};

class MRegion
{
public:
				MRegion();
				MRegion(
					const MRect&		inRect);
				MRegion(
					const MRegion&		inRegion);
	MRegion&	operator=(
					const MRegion&		inRegion);

				~MRegion();

				// Intersection
	MRegion&	operator&(
					const MRegion&		inRegion);

	MRegion&	operator&(
					const MRect&		inRect);

				// Union
	MRegion&	operator|(
					const MRegion&		inRegion);

	MRegion&	operator|(
					const MRect&		inRect);

				// test for empty region
				operator bool() const;

private:
	struct MRegionImpl*
				mImpl;
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

enum MTriState {
	eTriStateOn,
	eTriStateOff,
	eTriStateLatent	
};

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
	kScrollToThumb,
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

enum {	// modifier keys
	kCmdKey						 = 1 << 0,
	kShiftKey 					 = 1 << 1,
	kOptionKey					 = 1 << 2,
	kControlKey					 = 1 << 3,
	kAlphaLock					 = 1 << 4,
	kNumPad 					 = 1 << 5,
	kRightShiftKey				 = 1 << 6,
	kRightOptionKey				 = 1 << 7,
	kRightControlKey			 = 1 << 8
};

enum {	// key codes
	kNullKeyCode 				= 0,
	kHomeKeyCode 				= 1,
	kCancelKeyCode				= 3,
	kEndKeyCode					= 4,
	kHelpKeyCode 				= 5,
	kBellKeyCode 				= 7,
	kBackspaceKeyCode			= 8,
	kTabKeyCode					= 9,
	kLineFeedKeyCode 			= 10,
	kVerticalTabKeyCode			= 11,
	kPageUpKeyCode				= 11,
	kFormFeedKeyCode 			= 12,
	kPageDownKeyCode 			= 12,
	kReturnKeyCode				= 13,
	kFunctionKeyKeyCode			= 16,
	kPauseKeyCode				= 19,
	kEscapeKeyCode				= 27,
	kClearKeyCode				= 27,
	kLeftArrowKeyCode			= 28,
	kRightArrowKeyCode			= 29,
	kUpArrowKeyCode				= 30,
	kDownArrowKeyCode			= 31,
	kSpaceKeyCode				= 32,
	kDeleteKeyCode				= 127,
	kF1KeyCode					= 0x0101,
	kF2KeyCode					= 0x0102,
	kF3KeyCode					= 0x0103,
	kF4KeyCode					= 0x0104,
	kF5KeyCode					= 0x0105,
	kF6KeyCode					= 0x0106,
	kF7KeyCode					= 0x0107,
	kF8KeyCode					= 0x0108,
	kF9KeyCode					= 0x0109,
	kF10KeyCode					= 0x010a,
	kF11KeyCode					= 0x010b,
	kF12KeyCode					= 0x010c,
	kF13KeyCode					= 0x010d,
	kF14KeyCode					= 0x010e,
	kF15KeyCode					= 0x010f,

	// my own pseudo key codes
	kEnterKeyCode				= 0x0201,
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
		x < inRHS.x + inRHS.width and
		x + width > inRHS.x and
		y < inRHS.y + inRHS.height and
		y + height > inRHS.y;
}

inline bool MRect::ContainsPoint(
	int32				inX,
	int32				inY) const
{
	return
		x <= inX and x + width > inX and
		y <= inY and y + height > inY;
}



#endif
