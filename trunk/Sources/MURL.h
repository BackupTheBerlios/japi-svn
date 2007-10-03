#ifndef MURL_H
#define MURL_H

#include <string>

class MURL
{
  public:
	explicit		MURL(
						const std::string&		inURL)
						: mURL(inURL)
					{
					}

					MURL() { }
					
					MURL(
						const MURL&				inRHS)
						: mURL(inRHS.mURL)
					{
					}

	std::string		leaf() const;

	MURL&			operator=(
						const MURL&				inRHS)
					{
						mURL = inRHS.mURL;
						return *this;
					}
	
	MURL&			operator/=(
						const std::string&		inSubPath)
					{
						mURL += '/';
						mURL += inSubPath;
						return *this;
					}
					
	bool			operator==(
						const MURL&				inRHS) const
					{
						return mURL == inRHS.mURL;
					}
	
  private:
	std::string		mURL;
};

inline MURL operator/(
	const MURL&			inLHS,
	const std::string&	inRHS)
{
	MURL result(inLHS);
	result /= inRHS;
	return result;
}

#endif
