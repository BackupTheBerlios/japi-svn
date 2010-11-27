//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSshUtil.h,v 1.5 2004/02/28 22:28:13 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 10:55:25
*/

#ifndef MSSHPACKET_H
#define MSSHPACKET_H

#include <iostream>
#include <vector>
#include <deque>

#include <cryptopp/integer.h>

class MSshPacketCompressor;
class MSshPacketDecompressor;

class MSshPacket
{
  public:
					MSshPacket();
					MSshPacket(const MSshPacket& inPacket);
					MSshPacket(const std::vector<byte>& inData);
					MSshPacket(const std::deque<byte>& inData,
						uint32 inLength);

	MSshPacket&		operator=(const MSshPacket& inPacket);

	virtual			~MSshPacket();

	void			Wrap(uint32 inBlockSize,
						CryptoPP::RandomNumberGenerator& inRNG);

					// zlib compression
	void			Compress(
						MSshPacketCompressor&	inCompressor);
	void			Decompress(
						MSshPacketDecompressor&	inDecompressor);
	
	MSshPacket&		operator<<(bool inValue);
	MSshPacket&		operator<<(int8 inValue);
	MSshPacket&		operator<<(uint8 inValue);
	MSshPacket&		operator<<(int16 inValue);
	MSshPacket&		operator<<(uint16 inValue);
	MSshPacket&		operator<<(int32 inValue);
	MSshPacket&		operator<<(uint32 inValue);
	MSshPacket&		operator<<(int64 inValue);
	MSshPacket&		operator<<(uint64 inValue);
	MSshPacket&		operator<<(const std::string& inValue);
	MSshPacket&		operator<<(const std::vector<byte>& inValue);
	MSshPacket&		operator<<(const char* inValue);
	MSshPacket&		operator<<(const CryptoPP::Integer& inValue);
	MSshPacket&		operator<<(const MSshPacket& inPacket);

	MSshPacket&		operator>>(bool& outValue);
	MSshPacket&		operator>>(int8& inValue);
	MSshPacket&		operator>>(uint8& inValue);
	MSshPacket&		operator>>(int16& inValue);
	MSshPacket&		operator>>(uint16& inValue);
	MSshPacket&		operator>>(int32& inValue);
	MSshPacket&		operator>>(uint32& inValue);
	MSshPacket&		operator>>(int64& inValue);
	MSshPacket&		operator>>(uint64& inValue);
	MSshPacket&		operator>>(std::string& outValue);
	MSshPacket&		operator>>(CryptoPP::Integer& outValue);
	MSshPacket&		operator>>(MSshPacket& outPacket);
	
	const byte*		peek() const;
	uint32			size() const;
	bool			empty() const;
	
	uint8			pop_front();

  private:
	std::vector<uint8>
					mData;
};

class MSshPacketZLibBase
{
  public:
					MSshPacketZLibBase();
	virtual			~MSshPacketZLibBase();
	
	virtual void	Process(
						std::vector<uint8>& ioData) = 0;

  protected:
	struct z_stream_s*
					mStream;
};

class MSshPacketCompressor : public MSshPacketZLibBase
{
  public:
					MSshPacketCompressor();
	virtual			~MSshPacketCompressor();
	
	virtual void	Process(
						std::vector<uint8>& ioData);
};

class MSshPacketDecompressor : public MSshPacketZLibBase
{
  public:
					MSshPacketDecompressor();
	virtual			~MSshPacketDecompressor();
	
	virtual void	Process(
						std::vector<uint8>& ioData);
};

namespace std {
void swap(MSshPacket& a, MSshPacket& b);
}

std::ostream& operator<<(std::ostream& os, MSshPacket& p);

#endif
