//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSshUtil.h,v 1.5 2004/02/28 22:28:13 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 10:55:25
*/

#ifndef MSSHUTIL_H
#define MSSHUTIL_H

#include <string>
#include <vector>
#include <deque>

#include "MUtils.h"
#include "MError.h"

#include <cryptopp/integer.h>

enum eSshMsg {
	SSH_MSG_DISCONNECT = 1,
	SSH_MSG_IGNORE,
	SSH_MSG_UNIMPLEMENTED,
	SSH_MSG_DEBUG,
	SSH_MSG_SERVICE_REQUEST,
	SSH_MSG_SERVICE_ACCEPT,

	SSH_MSG_KEXINIT = 20,
	SSH_MSG_NEWKEYS,

/*	Numbers 30-49 used for kex packets.
	Different kex methods may reuse message numbers in
	this range. */

	SSH_MSG_KEXDH_INIT = 30,
	SSH_MSG_KEXDH_REPLY,

 	SSH_MSG_USERAUTH_REQUEST = 50,
	SSH_MSG_USERAUTH_FAILURE,
	SSH_MSG_USERAUTH_SUCCESS,
	SSH_MSG_USERAUTH_BANNER,
	
	SSH_MSG_USERAUTH_INFO_REQUEST = 60,
	SSH_MSG_USERAUTH_INFO_RESPONSE,

	SSH_MSG_GLOBAL_REQUEST = 80,
	SSH_MSG_REQUEST_SUCCESS,
	SSH_MSG_REQUEST_FAILURE,

	SSH_MSG_CHANNEL_OPEN = 90,
	SSH_MSG_CHANNEL_OPEN_CONFIRMATION,
	SSH_MSG_CHANNEL_OPEN_FAILURE,
	SSH_MSG_CHANNEL_WINDOW_ADJUST,
	SSH_MSG_CHANNEL_DATA,
	SSH_MSG_CHANNEL_EXTENDED_DATA,
	SSH_MSG_CHANNEL_EOF,
	SSH_MSG_CHANNEL_CLOSE,
	SSH_MSG_CHANNEL_REQUEST,
	SSH_MSG_CHANNEL_SUCCESS,
	SSH_MSG_CHANNEL_FAILURE,
};

enum eSshDisconnect {
	SSH_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT = 1,
	SSH_DISCONNECT_PROTOCOL_ERROR,
	SSH_DISCONNECT_KEY_EXCHANGE_FAILED,
	SSH_DISCONNECT_RESERVED,
	SSH_DISCONNECT_MAC_ERROR,
	SSH_DISCONNECT_COMPRESSION_ERROR,
	SSH_DISCONNECT_SERVICE_NOT_AVAILABLE,
	SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED,
	SSH_DISCONNECT_HOST_KEY_NOT_VERIFIABLE,
	SSH_DISCONNECT_CONNECTION_LOST,
	SSH_DISCONNECT_BY_APPLICATION,
	SSH_DISCONNECT_TOO_MANY_CONNECTIONS,
	SSH_DISCONNECT_AUTH_CANCELLED_BY_USER,
	SSH_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE,
	SSH_DISCONNECT_ILLEGAL_USER_NAME
};

struct TokenNames {
	int			value;
	const char*	token;
};

const TokenNames kTokens[] = {
	{ SSH_MSG_DISCONNECT, "SSH_MSG_DISCONNECT" },
	{ SSH_MSG_IGNORE, "SSH_MSG_IGNORE" },
	{ SSH_MSG_UNIMPLEMENTED, "SSH_MSG_UNIMPLEMENTED" },
	{ SSH_MSG_DEBUG, "SSH_MSG_DEBUG" },
	{ SSH_MSG_SERVICE_REQUEST, "SSH_MSG_SERVICE_REQUEST" },
	{ SSH_MSG_SERVICE_ACCEPT, "SSH_MSG_SERVICE_ACCEPT" },
	{ SSH_MSG_KEXINIT, "SSH_MSG_KEXINIT" },
	{ SSH_MSG_NEWKEYS, "SSH_MSG_NEWKEYS" },
	{ SSH_MSG_KEXDH_INIT, "SSH_MSG_KEXDH_INIT" },
	{ SSH_MSG_KEXDH_REPLY, "SSH_MSG_KEXDH_REPLY" },
 	{ SSH_MSG_USERAUTH_REQUEST, "SSH_MSG_USERAUTH_REQUEST" },
	{ SSH_MSG_USERAUTH_FAILURE, "SSH_MSG_USERAUTH_FAILURE" },
	{ SSH_MSG_USERAUTH_SUCCESS, "SSH_MSG_USERAUTH_SUCCESS" },
	{ SSH_MSG_USERAUTH_BANNER, "SSH_MSG_USERAUTH_BANNER" },
	{ SSH_MSG_USERAUTH_INFO_REQUEST, "SSH_MSG_USERAUTH_INFO_REQUEST" },
	{ SSH_MSG_USERAUTH_INFO_RESPONSE, "SSH_MSG_USERAUTH_INFO_RESPONSE" },
	{ SSH_MSG_GLOBAL_REQUEST, "SSH_MSG_GLOBAL_REQUEST" },
	{ SSH_MSG_REQUEST_SUCCESS, "SSH_MSG_REQUEST_SUCCESS" },
	{ SSH_MSG_REQUEST_FAILURE, "SSH_MSG_REQUEST_FAILURE" },
	{ SSH_MSG_CHANNEL_OPEN, "SSH_MSG_CHANNEL_OPEN" },
	{ SSH_MSG_CHANNEL_OPEN_CONFIRMATION, "SSH_MSG_CHANNEL_OPEN_CONFIRMATION" },
	{ SSH_MSG_CHANNEL_OPEN_FAILURE, "SSH_MSG_CHANNEL_OPEN_FAILURE" },
	{ SSH_MSG_CHANNEL_WINDOW_ADJUST, "SSH_MSG_CHANNEL_WINDOW_ADJUST" },
	{ SSH_MSG_CHANNEL_DATA, "SSH_MSG_CHANNEL_DATA" },
	{ SSH_MSG_CHANNEL_EXTENDED_DATA, "SSH_MSG_CHANNEL_EXTENDED_DATA" },
	{ SSH_MSG_CHANNEL_EOF, "SSH_MSG_CHANNEL_EOF" },
	{ SSH_MSG_CHANNEL_CLOSE, "SSH_MSG_CHANNEL_CLOSE" },
	{ SSH_MSG_CHANNEL_REQUEST, "SSH_MSG_CHANNEL_REQUEST" },
	{ SSH_MSG_CHANNEL_SUCCESS, "SSH_MSG_CHANNEL_SUCCESS" },
	{ SSH_MSG_CHANNEL_FAILURE, "SSH_MSG_CHANNEL_FAILURE" },
	{ 0 }
};

const TokenNames kErrors[] = {
	{ SSH_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT, "SSH_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT" },
	{ SSH_DISCONNECT_PROTOCOL_ERROR, "SSH_DISCONNECT_PROTOCOL_ERROR" },
	{ SSH_DISCONNECT_KEY_EXCHANGE_FAILED, "SSH_DISCONNECT_KEY_EXCHANGE_FAILED" },
	{ SSH_DISCONNECT_RESERVED, "SSH_DISCONNECT_RESERVED" },
	{ SSH_DISCONNECT_MAC_ERROR, "SSH_DISCONNECT_MAC_ERROR" },
	{ SSH_DISCONNECT_COMPRESSION_ERROR, "SSH_DISCONNECT_COMPRESSION_ERROR" },
	{ SSH_DISCONNECT_SERVICE_NOT_AVAILABLE, "SSH_DISCONNECT_SERVICE_NOT_AVAILABLE" },
	{ SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED, "SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED" },
	{ SSH_DISCONNECT_HOST_KEY_NOT_VERIFIABLE, "SSH_DISCONNECT_HOST_KEY_NOT_VERIFIABLE" },
	{ SSH_DISCONNECT_CONNECTION_LOST, "SSH_DISCONNECT_CONNECTION_LOST" },
	{ SSH_DISCONNECT_BY_APPLICATION, "SSH_DISCONNECT_BY_APPLICATION" },
	{ SSH_DISCONNECT_TOO_MANY_CONNECTIONS, "SSH_DISCONNECT_TOO_MANY_CONNECTIONS" },
	{ SSH_DISCONNECT_AUTH_CANCELLED_BY_USER, "SSH_DISCONNECT_AUTH_CANCELLED_BY_USER" },
	{ SSH_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE, "SSH_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE" },
	{ SSH_DISCONNECT_ILLEGAL_USER_NAME, "SSH_DISCONNECT_ILLEGAL_USER_NAME" },
	{ 0 }
};

struct MSshPacket
{
	typedef	net_swapper		swapper;
	swapper					swap;
	
	class MSshPacketError : public std::exception
	{
	  public:
		virtual const char* what() const throw()
			{ return "malformed packet"; }
	};
	
	template<typename T>
	MSshPacket&		operator<<(T inValue)
	{
		for (int32 i = sizeof(T) - 1; i >= 0; --i)
			data.push_back(static_cast<char>(inValue >> (i * 8)));
		return *this;
	}

	MSshPacket&		operator<<(bool inValue)
	{
		data += static_cast<char>(inValue);
		return *this;
	}
	
	MSshPacket&		operator<<(std::string inValue)
	{
		operator<<(static_cast<uint32>(inValue.length()));
		data.append(inValue);
		return *this;
	}
	
	MSshPacket&		operator<<(const char* inValue)
	{
		operator<<(std::string(inValue));
		return *this;
	}
	
	MSshPacket&		operator<<(const CryptoPP::Integer& inValue)
	{
		uint32 l = inValue.MinEncodedSize(CryptoPP::Integer::SIGNED);
		operator<<(l);
		uint32 s = data.length();
		data.append(l, 0);
		inValue.Encode(reinterpret_cast<byte*>(&data[0] + s), l, CryptoPP::Integer::SIGNED);
		return *this;
	}

	template<typename T>
	MSshPacket&		operator>>(T& outValue)
	{
		outValue = 0;
		for (uint32 i = 0; i < sizeof(T); ++i)
			outValue = outValue << 8 | static_cast<byte>(data[i]);
		data.erase(data.begin(), data.begin() + sizeof(T));
		return *this;
	}

	MSshPacket&		operator>>(std::string& outValue)
	{
		uint32 l;
		operator>>(l);
		
		if (l > data.size())
			throw MSshPacketError();

		outValue = data.substr(0, l);
		data.erase(data.begin(), data.begin() + l);
		return *this;
	}

	MSshPacket&		operator>>(bool& outValue)
	{
		uint8 v;
		operator>>(v);
		
		outValue = (v != 0);
		return *this;
	}

	MSshPacket&		operator>>(CryptoPP::Integer& outValue)
	{
		uint32 l;
		operator>>(l);
		
		if (l > data.size())
			throw MSshPacketError();

		outValue.Decode(reinterpret_cast<const byte*>(&data[0]), l, CryptoPP::Integer::SIGNED);
		data.erase(data.begin(), data.begin() + l);
		return *this;
	}
	
#if DEBUG
	void			Dump() const
					{
						hexdump(std::cerr, &data[0], data.length());
					}
#endif
	
	std::string		data;
};

#endif // MSSHUTIL_H
