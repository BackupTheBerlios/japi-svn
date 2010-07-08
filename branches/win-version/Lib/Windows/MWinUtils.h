//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINUTILS_H
#define MWINUTILS_H

#include <string>

std::wstring c2w(const std::string& s);
std::string w2c(const std::wstring& s);

void LogWinMsg(const char* inWhere, uint32 inMsg);

template<typename T>
class MComPtr
{
public:
			MComPtr()
				: mPtr(nil) {}

			MComPtr(
				const MComPtr&	rhs)
				: mPtr(rhs.mPtr)
			{
				mPtr->AddRef();
			}

	template<class C>
	MComPtr&
			operator=(const MComPtr<C>& rhs)
			{
				if ((const void*)this != (const void*)&rhs)
				{
					if (mPtr != nil)
						mPtr->Release();
					
					mPtr = rhs.get();
					
					if (mPtr != nil)
						mPtr->AddRef();
				}

				return *this;
			}

			~MComPtr()
			{
				if (mPtr != nil)
					mPtr->Release();
			}

			operator T*() const
			{
				return mPtr;
			}

	T*		operator->() const
			{
				return mPtr;
			}

	T**		operator &()
			{
				if (mPtr != nil)
					mPtr->Release();
				
				mPtr = nil;
				return &mPtr;
			}

	void	reset(
				T*		inPtr)
			{
				if (mPtr != nil)
					mPtr->Release();

				mPtr = inPtr;
			}

	T*		get() const
			{
				return mPtr;
			}

	T*		release()
			{
				T* result = mPtr;
				mPtr = nil;
				return result;
			}

			operator bool() const
			{
				return mPtr != nil;
			}

private:
	T*		mPtr;
};

#endif