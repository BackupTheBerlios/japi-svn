//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MDOCCLOSEDNOTIFIER_H
#define MDOCCLOSEDNOTIFIER_H

class MDocClosedNotifierImpl
{
public:
						MDocClosedNotifierImpl();

	void				AddRef();
	void				Release();

	virtual bool		ReadSome(std::string& outText) = 0;

protected:
	virtual				~MDocClosedNotifierImpl();

	int32				mRefCount;
};

class MDocClosedNotifier
{
  public:
						MDocClosedNotifier(
							MDocClosedNotifierImpl*		inImpl);
						
						MDocClosedNotifier(
							const MDocClosedNotifier&	inRHS);
	
	MDocClosedNotifier&	operator=(
							const MDocClosedNotifier&	inRHS);

						~MDocClosedNotifier();

	// read some data from client (i.e. stdin)
	// returns false if the client stopped sending data
	bool				ReadSome(
							std::string&				outText);

  private:
	MDocClosedNotifierImpl*
						mImpl;
};

#endif
