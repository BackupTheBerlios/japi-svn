/*
	Copyright (c) 2006, Maarten L. Hekkelman
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

typedef struct _xmlNode xmlNode;
typedef xmlNode *xmlNodePtr;

struct MRect
{
	int32		x, y;
	int32		width, height;
	
				MRect();

				MRect(
					const MRect&		inRHS);

				MRect(
					const GdkRectangle&	inRHS);

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
class auto_array
{
  public:
	explicit	auto_array(T* inPtr = nil);
				auto_array(auto_array& inOther);
	auto_array&	operator=(auto_array& inOther);	
	virtual		~auto_array();
	
	T*			get() const;
	T&			operator[](uint32 inIndex)	{ return ptr_[inIndex]; }
	
	T*			release();
	void		reset(T* inOther);
	
  private:
	T*			ptr_;
};

template<class T>
inline
T* auto_array<T>::release()
{
	T* result = ptr_;
	ptr_ = nil;
	return result;
}

template<class T>
inline
void auto_array<T>::reset(T* inOther)
{
	delete ptr_;
	ptr_ = inOther;
}

template<class T>
inline
auto_array<T>::auto_array(T* inPtr)
	: ptr_(inPtr)
{
}

template<class T>
inline
auto_array<T>::auto_array(auto_array& inOther)
	: ptr_(inOther.release())
{
}

template<class T>
inline
auto_array<T>& auto_array<T>::operator=(auto_array& inOther)
{
	reset(inOther.release());
	return *this;
}

template<class T>
inline
auto_array<T>::~auto_array()
{
	delete[] ptr_;
}

template<class T>
inline
T* auto_array<T>::get() const
{
	return ptr_;
}

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

inline
MRect::MRect(
	const GdkRectangle&	inRHS)
	: x(inRHS.x)
	, y(inRHS.y)
	, width(inRHS.width)
	, height(inRHS.height)
{
}

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

//template<class charT>
//inline
//std::basic_ostream<charT>& operator<<(std::basic_ostream<charT>& lhs, MRect rhs)
//{
//	lhs << rhs.x << ',' << rhs.y << '-' << rhs.width << ',' << rhs.height;
//	return lhs;
//}

#endif
