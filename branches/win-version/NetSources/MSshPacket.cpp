//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSshConnection.cpp,v 1.38 2004/03/05 20:43:26 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:35:17
*/

#include "MLib.h"

#include <vector>
#include <zlib.h>

#include <boost/iostreams/copy.hpp>

#include "MSshPacket.h"
#include "MUtils.h"

using namespace std;
using namespace CryptoPP;
namespace io = boost::iostreams;

const int kDefaultCompressionLevel = 3;

class MSshPacketError : public std::exception
{
  public:
	virtual const char* what() const throw()
		{ return "malformed packet"; }
};

MSshPacketZLibBase::MSshPacketZLibBase()
{
	mStream = new z_stream_s;
	memset(mStream, 0, sizeof(z_stream_s));
}

MSshPacketZLibBase::~MSshPacketZLibBase()
{
	delete mStream;
}
	
MSshPacketCompressor::MSshPacketCompressor()
{
	int err = deflateInit(mStream, kDefaultCompressionLevel);
	if (err != Z_OK)
		THROW(("Compressor error: %s", mStream->msg));
}

MSshPacketCompressor::~MSshPacketCompressor()
{
	deflateEnd(mStream);
}
	
void MSshPacketCompressor::Process(
	vector<uint8>& ioData)
{
	mStream->next_in = &ioData[0];
	mStream->avail_in = ioData.size();
	mStream->total_in = 0;
	
	vector<uint8> data;
	uint8 buffer[1024];
	
	mStream->next_out = buffer;
	mStream->avail_out = sizeof(buffer);
	mStream->total_out = 0;

	int err;
	do
	{
		err = deflate(mStream, Z_SYNC_FLUSH);

		if (sizeof(buffer) - mStream->avail_out > 0)
		{
			copy(buffer, buffer + sizeof(buffer) - mStream->avail_out,
				back_inserter(data));
			mStream->next_out = buffer;
			mStream->avail_out = sizeof(buffer);
		}
	}
	while (err >= Z_OK);
	
	if (err != Z_BUF_ERROR)
		THROW(("Compression error %d (%s)", err, mStream->msg));
	swap(data, ioData);
}

MSshPacketDecompressor::MSshPacketDecompressor()
{
	int err = inflateInit(mStream);
	if (err != Z_OK)
		THROW(("Compressor error: %s", mStream->msg));
}

MSshPacketDecompressor::~MSshPacketDecompressor()
{
	deflateEnd(mStream);
}

void MSshPacketDecompressor::Process(
	vector<uint8>& ioData)
{
	mStream->next_in = &ioData[0];
	mStream->avail_in = ioData.size();
	mStream->total_in = 0;
	
	vector<uint8> data;
	uint8 buffer[1024];
	
	mStream->next_out = buffer;
	mStream->avail_out = sizeof(buffer);
	mStream->total_out = 0;

	int err;
	do
	{
		err = inflate(mStream, Z_SYNC_FLUSH);

		if (sizeof(buffer) - mStream->avail_out > 0)
		{
			copy(buffer, buffer + sizeof(buffer) - mStream->avail_out,
				back_inserter(data));
			mStream->next_out = buffer;
			mStream->avail_out = sizeof(buffer);
		}
	}
	while (err >= Z_OK);
	
	if (err != Z_BUF_ERROR)
		THROW(("Decompression error %d (%s)", err, mStream->msg));
	swap(data, ioData);
}

struct MSshPacketImpl
{
					MSshPacketImpl()
						: mRefCount(1) {}
	virtual			~MSshPacketImpl() {}

	void			Reference();
	void			Release();

	uint8			pop()
					{
						if (mData.empty())
							throw MSshPacketError();
						
						uint8 result = mData.front();
						mData.erase(mData.begin());
						return result;
					}
					
	void			push(uint8 inByte)
					{
						mData.push_back(inByte);
					}
	
	uint32			mRefCount;
	vector<uint8>	mData;
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

MSshPacket::MSshPacket(const vector<uint8>& inData)
	: mImpl(new MSshPacketImpl)
{
	uint32 l = inData.size() - 5;
	l -= inData[4];
	
	mImpl->mData.reserve(l);

	copy(inData.begin() + 5, inData.begin() + 5 + l,
		back_inserter(mImpl->mData));

	assert(size() == l);
}

MSshPacket::MSshPacket(const deque<uint8>& inData, uint32 inLength)
	: mImpl(new MSshPacketImpl)
{
	mImpl->mData.reserve(inLength);
	copy(inData.begin(), inData.begin() + inLength, back_inserter(mImpl->mData));
	assert(size() == inLength);
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

MSshPacket& MSshPacket::operator<<(int64 inValue)
{
	mImpl->mData.push_back(inValue >> 56);
	mImpl->mData.push_back(inValue >> 48);
	mImpl->mData.push_back(inValue >> 40);
	mImpl->mData.push_back(inValue >> 32);
	mImpl->mData.push_back(inValue >> 24);
	mImpl->mData.push_back(inValue >> 16);
	mImpl->mData.push_back(inValue >> 8);
	mImpl->mData.push_back(inValue);
	return *this;
}

MSshPacket& MSshPacket::operator<<(uint64 inValue)
{
	mImpl->mData.push_back(inValue >> 56);
	mImpl->mData.push_back(inValue >> 48);
	mImpl->mData.push_back(inValue >> 40);
	mImpl->mData.push_back(inValue >> 32);
	mImpl->mData.push_back(inValue >> 24);
	mImpl->mData.push_back(inValue >> 16);
	mImpl->mData.push_back(inValue >> 8);
	mImpl->mData.push_back(inValue);
	return *this;
}

MSshPacket& MSshPacket::operator<<(const string& inValue)
{
	operator<<(static_cast<uint32>(inValue.length()));
	copy(inValue.begin(), inValue.end(), back_inserter(mImpl->mData));
	return *this;
}

MSshPacket& MSshPacket::operator<<(const vector<uint8>& inValue)
{
	copy(inValue.begin(), inValue.end(), back_inserter(mImpl->mData));
	return *this;
}

MSshPacket& MSshPacket::operator<<(const char* inValue)
{
	operator<<(string(inValue));
	return *this;
}

MSshPacket& MSshPacket::operator<<(const CryptoPP::Integer& inValue)
{
	uint32 n = mImpl->mData.size();

	uint32 l = inValue.MinEncodedSize(CryptoPP::Integer::SIGNED);
	operator<<(l);
	uint32 s = mImpl->mData.size();
	mImpl->mData.insert(mImpl->mData.end(), l, uint8(0));
	inValue.Encode(&mImpl->mData[0] + s, l, CryptoPP::Integer::SIGNED);
	
	assert(n + l + sizeof(uint32) == mImpl->mData.size());
	
	return *this;
}

MSshPacket& MSshPacket::operator<<(const MSshPacket& p)
{
	uint32 n = mImpl->mData.size();

	uint32 l = p.mImpl->mData.size();
	operator<<(l);
	mImpl->mData.insert(mImpl->mData.end(), p.mImpl->mData.begin(), p.mImpl->mData.end());

	assert(n + l + sizeof(uint32) == mImpl->mData.size());

	return *this;
}

MSshPacket& MSshPacket::operator>>(bool& outValue)
{
	if (size() < 1)
		throw MSshPacketError();
	
	uint8 v;
	operator>>(v);
	
	outValue = (v != 0);
	return *this;
}

MSshPacket& MSshPacket::operator>>(int8& outValue)
{
	if (size() < 1)
		throw MSshPacketError();
	
	outValue = mImpl->pop();
	return *this;
}

MSshPacket& MSshPacket::operator>>(uint8& outValue)
{
	if (size() < 1)
		throw MSshPacketError();
	
	outValue = mImpl->pop();
	return *this;
}

MSshPacket& MSshPacket::operator>>(int16& outValue)
{
	if (size() < 2)
		throw MSshPacketError();
	
	outValue = mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	return *this;
}

MSshPacket& MSshPacket::operator>>(uint16& outValue)
{
	if (size() < 2)
		throw MSshPacketError();
	
	outValue = mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	return *this;
}

MSshPacket& MSshPacket::operator>>(int32& outValue)
{
	if (size() < 4)
		throw MSshPacketError();
	
	outValue = mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	return *this;
}

MSshPacket& MSshPacket::operator>>(uint32& outValue)
{
	if (size() < 4)
		throw MSshPacketError();
	
	outValue = mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	return *this;
}

MSshPacket& MSshPacket::operator>>(int64& outValue)
{
	if (size() < 8)
		throw MSshPacketError();
	
	outValue = mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	return *this;
}

MSshPacket& MSshPacket::operator>>(uint64& outValue)
{
	if (size() < 8)
		throw MSshPacketError();
	
	outValue = mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	outValue = outValue << 8 | mImpl->pop();
	return *this;
}

MSshPacket& MSshPacket::operator>>(string& outValue)
{
	uint32 l;
	operator>>(l);
	
	if (l > 0)
	{
		if (l > size())
			throw MSshPacketError();

		outValue.assign(reinterpret_cast<char*>(&mImpl->mData[0]), l);
		mImpl->mData.erase(mImpl->mData.begin(), mImpl->mData.begin() + l);
	}
	else
		outValue.clear();

	return *this;
}

MSshPacket& MSshPacket::operator>>(CryptoPP::Integer& outValue)
{
	uint32 l;
	operator>>(l);
	
	if (l > size())
		throw MSshPacketError();

	outValue.Decode(&mImpl->mData[0], l, CryptoPP::Integer::SIGNED);
	mImpl->mData.erase(mImpl->mData.begin(), mImpl->mData.begin() + l);
	return *this;
}

MSshPacket& MSshPacket::operator>>(MSshPacket& p)
{
	uint32 l;
	operator>>(l);
	
	if (l > size())
		throw MSshPacketError();

	p.mImpl->mData.clear();
	copy(mImpl->mData.begin(), mImpl->mData.begin() + l, back_inserter(p.mImpl->mData));
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
	while ((mImpl->mData.size() - 5) < inBlockSize or padding < 4 or
		(mImpl->mData.size() % inBlockSize) != 0);

	mImpl->mData[4] = padding;
	
	uint32 l = mImpl->mData.size() - 4;
	for (int32 i = 3; i >= 0; --i)
	{
		mImpl->mData[i] = (l & 0x00FF);
		l >>= 8;
	}
}

void MSshPacket::Compress(
	MSshPacketCompressor&	inCompressor)
{
	inCompressor.Process(mImpl->mData);
}

void MSshPacket::Decompress(
	MSshPacketDecompressor&	inDecompressor)
{
	inDecompressor.Process(mImpl->mData);
}

const uint8* MSshPacket::peek() const
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

