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

#ifndef MUTILS_H
#define MUTILS_H

#include <boost/iterator/iterator_facade.hpp>

#include "MTypes.h"

struct no_swapper
{
	template<typename T>
	T			operator()(T inValue) const			{ return inValue; }
};

struct swapper
{
	template<typename T>
	T			operator()(T inValue) const
	{
		this_will_not_compile_I_hope(inValue);
	}
};

template<>
inline
bool swapper::operator()(bool inValue) const
{
	return inValue;
}

template<>
inline
int8 swapper::operator()(int8 inValue) const
{
	return inValue;
}

template<>
inline
uint8 swapper::operator()(uint8 inValue) const
{
	return inValue;
}

template<>
inline
int16 swapper::operator()(int16 inValue) const
{
	return static_cast<int16>(
		((inValue & 0xFF00UL) >>  8) |
		((inValue & 0x00FFUL) <<  8)
	);
}

template<>
inline
uint16 swapper::operator()(uint16 inValue) const
{
	return static_cast<uint16>(
		((inValue & 0xFF00UL) >>  8) |
		((inValue & 0x00FFUL) <<  8)
	);
}

template<>
inline
int32 swapper::operator()(int32 inValue) const
{
	return static_cast<int32>(
		((inValue & 0xFF000000UL) >> 24) |
		((inValue & 0x00FF0000UL) >>  8) |
		((inValue & 0x0000FF00UL) <<  8) |
		((inValue & 0x000000FFUL) << 24)
	);
}

template<>
inline
uint32 swapper::operator()(uint32 inValue) const
{
	return static_cast<uint32>(
		((inValue & 0xFF000000UL) >> 24) |
		((inValue & 0x00FF0000UL) >>  8) |
		((inValue & 0x0000FF00UL) <<  8) |
		((inValue & 0x000000FFUL) << 24)
	);
}

template<>
inline
long unsigned int swapper::operator()(long unsigned int inValue) const
{
	return static_cast<long unsigned int>(
		((inValue & 0xFF000000UL) >> 24) |
		((inValue & 0x00FF0000UL) >>  8) |
		((inValue & 0x0000FF00UL) <<  8) |
		((inValue & 0x000000FFUL) << 24)
	);
}

template<>
inline
int64 swapper::operator()(int64 inValue) const
{
	return static_cast<int64>(
		(((static_cast<uint64>(inValue))<<56) & 0xFF00000000000000ULL)  |
		(((static_cast<uint64>(inValue))<<40) & 0x00FF000000000000ULL)  |
		(((static_cast<uint64>(inValue))<<24) & 0x0000FF0000000000ULL)  |
		(((static_cast<uint64>(inValue))<< 8) & 0x000000FF00000000ULL)  |
		(((static_cast<uint64>(inValue))>> 8) & 0x00000000FF000000ULL)  |
		(((static_cast<uint64>(inValue))>>24) & 0x0000000000FF0000ULL)  |
		(((static_cast<uint64>(inValue))>>40) & 0x000000000000FF00ULL)  |
		(((static_cast<uint64>(inValue))>>56) & 0x00000000000000FFULL));
}

template<>
inline
uint64 swapper::operator()(uint64 inValue) const
{
	return static_cast<uint64>(
		((((uint64)inValue)<<56) & 0xFF00000000000000ULL)  |
		((((uint64)inValue)<<40) & 0x00FF000000000000ULL)  |
		((((uint64)inValue)<<24) & 0x0000FF0000000000ULL)  |
		((((uint64)inValue)<< 8) & 0x000000FF00000000ULL)  |
		((((uint64)inValue)>> 8) & 0x00000000FF000000ULL)  |
		((((uint64)inValue)>>24) & 0x0000000000FF0000ULL)  |
		((((uint64)inValue)>>40) & 0x000000000000FF00ULL)  |
		((((uint64)inValue)>>56) & 0x00000000000000FFULL));
}

#if BYTE_ORDER == LITTLE_ENDIAN
typedef no_swapper	lsb_swapper;
typedef swapper		msb_swapper;

typedef swapper		net_swapper;
#else
typedef swapper		lsb_swapper;
typedef no_swapper	msb_swapper;

typedef no_swapper	net_swapper;
#endif

// value changer, stack based

template<class T>
class MValueChanger
{
  public:
				MValueChanger(
					T&				inVariable,		
					const T&		inNewValue)
					: mVariable(inVariable)
					, mValue(inVariable)
				{
					mVariable = inNewValue;
				}
				
				~MValueChanger()
				{
					mVariable = mValue;
				}
  private:
	T&			mVariable;
	T			mValue;
};

typedef struct _xmlNode xmlNode;
typedef xmlNode *xmlNodePtr;

class XMLNode
{
  public:
					XMLNode(
						xmlNodePtr	inNode)
						: mNode(inNode)	{}

	std::string		name() const;

	std::string		text() const;

	std::string		property(
						const char*	inName) const;
	
	xmlNodePtr		children() const;
	
					operator xmlNodePtr() const		{ return mNode; }

	class iterator : public boost::iterator_facade<iterator,
		XMLNode, boost::forward_traversal_tag>
	{
	  public:
						iterator(
							xmlNodePtr	inNode = nil)
							: mNode(inNode)
							, mXMLNode(new XMLNode(inNode)) {}
		
						~iterator()
						{
							delete mXMLNode;
						}
	  private:
		friend class boost::iterator_core_access;

		void			increment();
		bool			equal(const iterator& inOther) const;
		reference		dereference() const;
		
		xmlNodePtr		mNode;
		XMLNode*		mXMLNode;
	};

	iterator			begin() const;
	iterator			end() const;

  private:
	friend class iterator;

	xmlNodePtr		mNode;
};

uint16 CalculateCRC(const void* inData, uint32 inLength, uint16 inCRC);

std::string Escape(std::string inString);
std::string Unescape(std::string inString);

std::string NumToString(uint32 inNumber);
uint32 StringToNum(std::string inString);

std::string	GetUserName(bool inShortName = false);
std::string	GetDateTime();

void NormalizePath(std::string& ioPath);

uint32 AddNameToNameTable(
			std::string&		ioNameTable,
			const std::string&	inName);

double GetLocalTime();
double GetDoubleClickTime();

bool IsModifierDown(
		int						inModifierMask);

void HexDump(
	const void*		inBuffer,
	uint32			inLength,
	std::ostream&	outStream);

#endif
