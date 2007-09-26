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

#include <string>

#define	nil					NULL

typedef char				int8;
typedef unsigned char		uint8;
typedef short				int16;
typedef unsigned short		uint16;
typedef int					int32;
typedef unsigned int		uint32;
typedef long long			int64;
typedef unsigned long long	uint64;

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

#endif
