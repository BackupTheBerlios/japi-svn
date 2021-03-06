/*
	Copyright (c) 2007, Maarten L. Hekkelman
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

/*	$Id: MSshUtil.h,v 1.5 2004/02/28 22:28:13 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 10:55:25
*/

#ifndef MSSHUTIL_H
#define MSSHUTIL_H

#include <string>
#include <vector>

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
		inValue = swap(inValue);
		data.append(reinterpret_cast<char*>(&inValue), sizeof(T));
		return *this;
	}

	MSshPacket&		operator<<(bool inValue)
	{
		uint8 v = inValue;
		data.append(reinterpret_cast<char*>(&v), 1);
		return *this;
	}
	
	MSshPacket&		operator<<(std::string inValue)
	{
		uint32 l = inValue.length();
		l = swap(l);
		data.append(reinterpret_cast<char*>(&l), sizeof(uint32));
		data.append(inValue);
		return *this;
	}
	
	MSshPacket&		operator<<(const char* inValue)
	{
		operator<<(std::string(reinterpret_cast<const char*>(inValue)));
		return *this;
	}
	
	MSshPacket&		operator<<(const CryptoPP::Integer& inValue)
	{
		uint32 l = inValue.MinEncodedSize(CryptoPP::Integer::SIGNED);
		operator<<(l);
		std::vector<char> b(l);
		inValue.Encode(reinterpret_cast<byte*>(&b[0]), l, CryptoPP::Integer::SIGNED);
		data.append(&b[0], l);
		return *this;
	}

	template<typename T>
	MSshPacket&		operator>>(T& outValue)
	{
		data.copy(reinterpret_cast<char*>(&outValue), sizeof(T), 0);
		outValue = swap(outValue);
		data.erase(0, sizeof(T));
		return *this;
	}

	MSshPacket&		operator>>(std::string& outValue)
	{
		uint32 l;
		operator>>(l);
		
		if (l <= data.size())
		{
			outValue.assign(data.begin(), data.begin() + l);
			data.erase(0, l);
		}
		else
			throw MSshPacketError();
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
		
		if (l <= data.size())
		{
			outValue.Decode(reinterpret_cast<const byte*>(data.c_str()),
				l, CryptoPP::Integer::SIGNED);
			data.erase(0, l);
		}
		else
			throw MSshPacketError();
		return *this;
	}
	
#if DEBUG
	void			Dump() const
					{
						std::cout << "<< " << data.length() << " bytes" << std::endl;
					
						const char kHex[] = "0123456789abcdef";
						char s[] = "xxxxxxxx  cccc cccc cccc cccc  cccc cccc cccc cccc  |................|";
						const int kHexOffset[] = { 10, 12, 15, 17, 20, 22, 25, 27, 31, 33, 36, 38, 41, 43, 46, 48 };
						const int kAsciiOffset = 53;
						
						const unsigned char* text = reinterpret_cast<const unsigned char*>(data.c_str());
						
						unsigned long offset = 0;
						
						while (offset < data.length())
						{
							int rr = data.length() - offset;
							if (rr > 16)
								rr = 16;
							
							char* t = s + 7;
							long o = offset;
							
							while (t >= s)
							{
								*t-- = kHex[o % 16];
								o /= 16;
							}
							
							for (int i = 0; i < rr; ++i)
							{
								s[kHexOffset[i] + 0] = kHex[text[i] >> 4];
								s[kHexOffset[i] + 1] = kHex[text[i] & 0x0f];
								if (text[i] < 128 and isprint(text[i]))
									s[kAsciiOffset + i] = text[i];
								else
									s[kAsciiOffset + i] = '.';
							}
							
							for (int i = rr; i < 16; ++i)
							{
								s[kHexOffset[i] + 0] = ' ';
								s[kHexOffset[i] + 1] = ' ';
								s[kAsciiOffset + i] = ' ';
							}
							
							puts(s);
							
							text += rr;
							offset += rr;
						}
					}
#endif
	
	std::string		data;
};

#endif // MSSHUTIL_H
