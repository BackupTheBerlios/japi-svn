//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSshConnection.cpp,v 1.38 2004/03/05 20:43:26 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:35:17
*/

#include "MJapi.h"

#include <vector>

#include <boost/iostreams/copy.hpp>

#include "MSshPacket.h"

using namespace std;
using namespace CryptoPP;
namespace io = boost::iostreams;

class MSshPacketError : public std::exception
{
  public:
	virtual const char* what() const throw()
		{ return "malformed packet"; }
};

struct MSshPacketImpl
{
					MSshPacketImpl()
						: mRefCount(1) {}
	virtual			~MSshPacketImpl() {}

	void			Reference();
	void			Release();
	
	uint32			mRefCount;
	vector<byte>	mData;
};

void MSshPacketImpl::Reference()
{
	++mRefCount;
}

void MSshPacketImpl::Release()
{
	if (--mRefCount == 0)
		delete this;
}

// --------------------------------------------------------------------

MSshPacket::MSshPacket()
	: mImpl(new MSshPacketImpl)
{
}

MSshPacket::MSshPacket(const MSshPacket& inPacket)
	: mImpl(inPacket.mImpl)
{
	mImpl->Reference();
}

MSshPacket::MSshPacket(std::streambuf& inBuffer)
	: mImpl(new MSshPacketImpl)
{
	istream in(&inBuffer);
	io::copy(in, back_inserter(mImpl->mData)); 
}

MSshPacket& MSshPacket::operator=(const MSshPacket& inPacket)
{
	mImpl->Release();
	mImpl = inPacket.mImpl;
	mImpl->Reference();
	
	return *this;
}

MSshPacket::~MSshPacket()
{
	mImpl->Release();
}

MSshPacket& MSshPacket::operator<<(bool inValue)
{
	mImpl->mData.push_back(inValue);
	return *this;
}

MSshPacket& MSshPacket::operator<<(int8 inValue)
{
	mImpl->mData.push_back(inValue);
	return *this;
}

MSshPacket& MSshPacket::operator<<(uint8 inValue)
{
	mImpl->mData.push_back(inValue);
	return *this;
}

MSshPacket& MSshPacket::operator<<(int16 inValue)
{
	mImpl->mData.push_back(inValue >> 8);
	mImpl->mData.push_back(inValue);
	return *this;
}

MSshPacket& MSshPacket::operator<<(uint16 inValue)
{
	mImpl->mData.push_back(inValue >> 8);
	mImpl->mData.push_back(inValue);
	return *this;
}

MSshPacket& MSshPacket::operator<<(int32 inValue)
{
	mImpl->mData.push_back(inValue >> 24);
	mImpl->mData.push_back(inValue >> 16);
	mImpl->mData.push_back(inValue >> 8);
	mImpl->mData.push_back(inValue);
	return *this;
}

MSshPacket& MSshPacket::operator<<(uint32 inValue)
{
	mImpl->mData.push_back(inValue >> 24);
	mImpl->mData.push_back(inValue >> 16);
	mImpl->mData.push_back(inValue >> 8);
	mImpl->mData.push_back(inValue);
	return *this;
}

MSshPacket& MSshPacket::operator<<(std::string inValue)
{
	operator<<(static_cast<uint32>(inValue.length()));
	copy(inValue.begin(), inValue.end(),
		std::back_inserter(mImpl->mData));
	return *this;
}

MSshPacket& MSshPacket::operator<<(const char* inValue)
{
	operator<<(std::string(inValue));
	return *this;
}

MSshPacket& MSshPacket::operator<<(const CryptoPP::Integer& inValue)
{
	uint32 l = inValue.MinEncodedSize(CryptoPP::Integer::SIGNED);
	operator<<(l);
	uint32 s = mImpl->mData.size();
	mImpl->mData.insert(mImpl->mData.end(), l, byte(0));
	inValue.Encode(reinterpret_cast<byte*>(&mImpl->mData[0] + s),
		l, CryptoPP::Integer::SIGNED);
	return *this;
}

MSshPacket& MSshPacket::operator<<(const MSshPacket& p)
{
	uint32 l = mImpl->mData.size();
	operator<<(l);
	mImpl->mData.insert(mImpl->mData.end(), p.mImpl->mData.begin(), p.mImpl->mData.end());
	return *this;
}

MSshPacket& MSshPacket::operator>>(bool& outValue)
{
	uint8 v;
	operator>>(v);
	
	outValue = (v != 0);
	return *this;
}

MSshPacket& MSshPacket::operator>>(int8& outValue)
{
	outValue = mImpl->mData.front();
	mImpl->mData.erase(mImpl->mData.begin());
	return *this;
}

MSshPacket& MSshPacket::operator>>(uint8& outValue)
{
	outValue = mImpl->mData.front();
	mImpl->mData.erase(mImpl->mData.begin());
	return *this;
}

MSshPacket& MSshPacket::operator>>(int16& outValue)
{
	outValue = mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	outValue = outValue << 8 | mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	return *this;
}

MSshPacket& MSshPacket::operator>>(uint16& outValue)
{
	outValue = mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	outValue = outValue << 8 | mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	return *this;
}

MSshPacket& MSshPacket::operator>>(int32& outValue)
{
	outValue = mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	outValue = outValue << 8 | mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	outValue = outValue << 8 | mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	outValue = outValue << 8 | mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	return *this;
}

MSshPacket& MSshPacket::operator>>(uint32& outValue)
{
	outValue = mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	outValue = outValue << 8 | mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	outValue = outValue << 8 | mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	outValue = outValue << 8 | mImpl->mData.front();
								mImpl->mData.erase(mImpl->mData.begin());
	return *this;
}

MSshPacket& MSshPacket::operator>>(std::string& outValue)
{
	uint32 l;
	operator>>(l);
	
	if (l > mImpl->mData.size())
		throw MSshPacketError();

	outValue.assign(reinterpret_cast<char*>(&mImpl->mData[0]), mImpl->mData.size());
	mImpl->mData.erase(mImpl->mData.begin(), mImpl->mData.begin() + l);
	return *this;
}

MSshPacket& MSshPacket::operator>>(CryptoPP::Integer& outValue)
{
	uint32 l;
	operator>>(l);
	
	if (l > mImpl->mData.size())
		throw MSshPacketError();

	outValue.Decode(&mImpl->mData[0], l, CryptoPP::Integer::SIGNED);
	mImpl->mData.erase(mImpl->mData.begin(), mImpl->mData.begin() + l);
	return *this;
}

MSshPacket& MSshPacket::operator>>(MSshPacket& p)
{
	uint32 l;
	operator>>(l);
	p.mImpl->mData.clear();
	copy(mImpl->mData.begin(), mImpl->mData.end(), std::back_inserter(p.mImpl->mData));
	mImpl->mData.erase(mImpl->mData.begin(), mImpl->mData.begin() + l);
	return *this;
}

void MSshPacket::Wrap(
	uint32 inBlockSize, CryptoPP::RandomNumberGenerator& inRNG)
{
	char b[5];
	mImpl->mData.insert(mImpl->mData.begin(), b, b + 5);
	
	uint8 padding = 0;
	
	do
	{
		operator<<(inRNG.GenerateByte());
		++padding;
	}
	while (mImpl->mData.size() < inBlockSize or padding < 4 or
		(mImpl->mData.size() + 5) % inBlockSize);

	mImpl->mData[4] = padding;
	
	uint32 l = mImpl->mData.size();
	for (int32 i = 3; i >= 0; --i)
	{
		mImpl->mData[i] = (l & 0x00FF);
		l >>= 8;
	}
}

//#if DEBUG
//void			Dump() const
//				{
//					HexDump(&data[0], mImpl->mData.size(), std::cerr);
//				}
//#endif

const byte* MSshPacket::peek() const
{
	return &mImpl->mData[0];
}

uint32 MSshPacket::size() const
{
	return mImpl->mData.size();
}

bool MSshPacket::empty() const
{
	return mImpl->mData.empty();
}

ostream& operator<<(ostream& os, MSshPacket& p)
{
	os.write(reinterpret_cast<const char*>(p.peek()), p.size());
	return os;
}

