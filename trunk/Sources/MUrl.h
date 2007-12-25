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

#ifndef MURL_H
#define MURL_H

#include "MFile.h"

class MUrl
{
  public:
					MUrl();
					
					MUrl(
						const MUrl&			inUrl);
					
	explicit		MUrl(
						const std::string&	inUrl);

	explicit		MUrl(
						const char*			inUrl);

	explicit		MUrl(
						const fs::path&		inFile);

	MUrl&			operator=(
						const MUrl&			inRHS);

	MUrl&			operator=(
						const std::string&	inRHS);
	
	bool			operator==(
						const MUrl&			inUrl) const;
	
					~MUrl();
	
	bool			IsLocal() const;
	
	bool			IsValid() const;

	std::string		str() const;

	std::string		GetFileName() const;
	
	void			SetFileName(
						const std::string&	inName);

	fs::path		GetPath() const;

	void			SetPath(
						const fs::path&		inPath);

	std::string		GetScheme() const;
	
	void			SetScheme(
						const std::string&	inScheme);

	std::string		GetHost() const;
	
	void			SetHost(
						const std::string&	inHost);

	std::string		GetUser() const;
	
	void			SetUser(
						const std::string&	inUser);

	std::string		GetPassword() const;
	
	void			SetPassword(
						const std::string&	inPassword);

	int16			GetPort() const;
	
	void			SetPort(
						int16				inPort);

	void			operator/=(
						const std::string&	inPartialPath);

  private:

	struct MUrlImp*	mImpl;
};

MUrl operator/(const MUrl& lhs, std::string rhs);

template<class charT>
std::basic_ostream<charT>& operator<<(std::basic_ostream<charT>& lhs, const MUrl& rhs)
{
	lhs << rhs.str();
	return lhs;
}

#endif

