/*-
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

#ifndef MPATRICIATREE_H
#define MPATRICIATREE_H

#include <boost/iterator/iterator_facade.hpp>

template<class T>
class MPatriciaTree
{
	struct MNode
	{
		MNode*			mLeft;
		MNode*			mRight;
		T				mValue;
		uint16			mBit;
		uint16			mKeyLength;
		char			mKey[1];

						MNode();
						~MNode();

		static MNode*	Create(
							const std::string&	inKey,
							const T&			inValue,
							uint16				inBit);
	
	  private:
						MNode(
							const std::string&	inKey,
							const T&			inValue,
							uint16				inBit);
		
		void*			operator new(size_t, size_t inKeyLength);
		void			operator delete(void*);
	};
	
  public:
						MPatriciaTree();
	virtual				~MPatriciaTree();
	
//	class iterator : public boost::iterator_facade<iterator, T,
//		boost::forward_traversal_tag>
//	{
//	  public:
//						iterator();
//						iterator(
//							const iterator& rhs);
//						iterator(
//							MPatriciaTree* inTree);
//		
//		T&				dereference() const;
//		bool			equal(const iterator& rhs) const;
//		void			increment();
//
//	  private:
//		CNode*			mNode;
//	};

	T&					operator[](
							const std::string& inKey);

	const T&			operator[](
							const std::string& inKey) const;

//	iterator			begin();
//	iterator			end();
	
	uint32				size() const;

  private:
						MPatriciaTree(
							const MPatriciaTree&);

	MPatriciaTree&		operator=(
							const MPatriciaTree&);

	bool				TestKeyBit(
							const char*		inKey,
							uint32			inKeyLength,
							uint16			inBit);

	bool				CompareKeyBits(
							const char*		inKeyA,
							uint32			inKeyALength,
							const char*		inKeyB,
							uint32			inKeyBLength,
							uint16			inBit);

	MNode*				find(
							MNode*				inNode,
							const std::string&	inKey);

	MNode*				insert(
							const std::string&	inKey,
							const T&			inValue);
	
	MNode				mRoot;
	uint32				mCount;
};

#pragma mark -

template<class T>
void* MPatriciaTree<T>::MNode::operator new(size_t inItemSize, size_t inKeyLength)
{
	return new char[inItemSize + inKeyLength];
}

template<class T>
void MPatriciaTree<T>::MNode::operator delete(void* p)
{
	delete[] reinterpret_cast<char*>(p);
}

template<class T>
MPatriciaTree<T>::MNode::MNode()
	: mLeft(nil)
	, mRight(nil)
	, mValue(T())
	, mBit(0)
	, mKeyLength(0)
{
}

template<class T>
MPatriciaTree<T>::MNode::MNode(
		const std::string&		inKey,
		const T&				inValue,
		uint16					inBit)
	: mLeft(nil)
	, mRight(nil)
	, mValue(inValue)
	, mBit(inBit)
	, mKeyLength(inKey.length())
{
	std::copy(inKey.begin(), inKey.end(), mKey);
}

template<class T>
MPatriciaTree<T>::MNode::~MNode()
{
	if (mLeft->mBit > mBit)
		delete mLeft;
	if (mRight->mBit > mBit)
		delete mRight;
}

template<class T>
typename MPatriciaTree<T>::MNode* MPatriciaTree<T>::MNode::Create(
	const std::string&		inKey,
	const T&				inValue,
	uint16					inBit)
{
	MNode* node = new(inKey.length()) MNode(inKey, inValue, inBit);
	return node;
}

//#pragma mark -
//template<class T>
//MPatriciaTree<T>::iterator::iterator()
//{
//}
//
//template<class T>
//MPatriciaTree<T>::iterator::iterator(
//	const iterator&	rhs)
//{
//}
//
//template<class T>
//MPatriciaTree<T>::iterator::iterator(
//	MPatriciaTree* inTree)
//{
//}
//
//template<class T>
//T& MPatriciaTree<T>::iterator::dereference() const
//{
//}
//
//template<class T>
//bool MPatriciaTree<T>::iterator::equal(const iterator& rhs) const
//{
//}
//
//template<class T>
//void MPatriciaTree<T>::iterator::increment()
//{
//}

#pragma mark -

template<class T>
MPatriciaTree<T>::MPatriciaTree()
	: mCount(0)
{
	mRoot.mBit = 0;
	mRoot.mLeft = &mRoot;
	mRoot.mRight = &mRoot;
}

template<class T>
MPatriciaTree<T>::~MPatriciaTree()
{
}

template<class T>
inline
T& MPatriciaTree<T>::operator[](
	const std::string& inKey)
{
	MNode* node = find(&mRoot, inKey);

	if (inKey.compare(0, inKey.length(), node->mKey, node->mKeyLength) != 0)
		node = insert(inKey, T());
	
	return node->mValue;
}

template<class T>
inline
const T& MPatriciaTree<T>::operator[](
	const std::string& inKey) const
{
	MNode* node = find(inKey, &mRoot);

	if (inKey.compare(0, inKey.length(), node->mKey, node->mKeyLength) != 0)
		node = insert(inKey, T());
	
	return node->mValue;
}

//template<class T>
//MPatriciaTree<T>::iterator MPatriciaTree<T>::begin()
//{
//}
//
//template<class T>
//MPatriciaTree<T>::iterator MPatriciaTree<T>::end()
//{
//}
	
template<class T>
inline
uint32 MPatriciaTree<T>::size() const
{
	return mCount;
}

template<class T>
inline
bool MPatriciaTree<T>::TestKeyBit(
	const char*		inKey,
	uint32			inKeyLength,
	uint16			inBit)
{
	bool result = false;
	
	uint16 byte = inBit >> 3;
	if (byte < inKeyLength)
	{
		uint16 bit = 7 - (inBit & 0x0007);
		result = (inKey[byte] & (1 << bit)) != 0;
	}
	
	return result;
}

template<class T>
inline
bool MPatriciaTree<T>::CompareKeyBits(
	const char*		inKeyA,
	uint32			inKeyALength,
	const char*		inKeyB,
	uint32			inKeyBLength,
	uint16			inBit)
{
	return
		TestKeyBit(inKeyA, inKeyALength, inBit) ==
		TestKeyBit(inKeyB, inKeyBLength, inBit);
}

template<class T>
typename MPatriciaTree<T>::MNode* MPatriciaTree<T>::find(
	MNode*				inNode,
	const std::string&	inKey)
{
	MNode* p;
	
	do
	{
		p = inNode;
		
		if (TestKeyBit(inKey.c_str(), inKey.length(), inNode->mBit))
			inNode = inNode->mRight;
		else
			inNode = inNode->mLeft;
	}
	while (p->mBit < inNode->mBit);
	
	return inNode;
}

template<class T>
typename MPatriciaTree<T>::MNode* MPatriciaTree<T>::insert(
	const std::string&	inKey,
	const T&			inValue)
{
	MNode* x = &mRoot;
	MNode* t = find(x, inKey);

	if (inKey.compare(0, inKey.length(), t->mKey, t->mKeyLength) == 0)
	{
		t->mValue = inValue;
		return t;
	}

	uint16 i = 0;
	
	while (CompareKeyBits(inKey.c_str(), inKey.length(), t->mKey, t->mKeyLength, i))
		++i;

	MNode* p;
	
	do
	{
		p = x;
		if (TestKeyBit(inKey.c_str(), inKey.length(), x->mBit))
			x = x->mRight;
		else
			x = x->mLeft;
	}
	while (x->mBit < i and p->mBit < x->mBit);
	
	t = MNode::Create(inKey, inValue, i);
	
	if (TestKeyBit(inKey.c_str(), inKey.length(), t->mBit))
	{
		t->mRight = t;
		t->mLeft = x;
	}
	else
	{
		t->mRight = x;
		t->mLeft = t;
	}
	
	if (TestKeyBit(inKey.c_str(), inKey.length(), p->mBit))
		p->mRight = t;
	else
		p->mLeft = t;
	
	++mCount;
	
	return t;
}

#endif
