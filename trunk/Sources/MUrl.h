//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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

	std::string		str(
						bool				inEncoded = false) const;

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

