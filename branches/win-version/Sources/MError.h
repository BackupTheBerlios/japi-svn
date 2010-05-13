//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MERROR_H
#define MERROR_H

#include <exception>
#include <iostream>
#include <boost/current_function.hpp>

class MException : public std::exception
{
  public:
				MException(const char* inErrMsg, ...);

	virtual const char*	what() const throw();

  protected:
				MException() {}

	char		mMessage[1024];
};

class MWinException : public MException
{
  public:
				MWinException(const char* inErrMsg, ...);
};

#ifdef NDEBUG

class StOKToThrow
{
  public:
	StOKToThrow() {}
	~StOKToThrow() {}
};

#define SIGNAL_THROW(a)

#define PRINT(x)

#else

class StOKToThrow
{
  public:
	StOKToThrow()	{ ++sOkToThrow; }
	~StOKToThrow()	{ --sOkToThrow; }
	
	static bool		IsOK()				{ return sOkToThrow != 0; }
	
  private:
	static int		sOkToThrow;
};

void __signal_throw(const char* inCode, const char* inFunction, const char* inFile, int inLine);

extern char*	__S_FILE;
extern int		__S_LINE;
void __debug_printf(const char* inMsg, ...);
//void __report_mach_error(const char* func, mach_error_t e);

#define PRINT(x)		do { __S_FILE = __FILE__; __S_LINE = __LINE__; __debug_printf x; } while (false)
#define	SIGNAL_THROW(a)	__signal_throw(a, BOOST_CURRENT_FUNCTION, __FILE__, __LINE__);

#endif

#define THROW_IF_OSERROR(x) \
	do { OSStatus __err = (x); if (__err != noErr) { SIGNAL_THROW(#x) throw MException(x); } } while (false)

#define THROW_IF_CFCREATE_FAILED(x) \
	do { if ((x) == nil) { SIGNAL_THROW(#x) throw MException("CFCreate failed for variable %s", #x); } } while (false)

#define THROW_IF_POSIX_ERROR(x) \
	do { if ((int)(x) == -1) { SIGNAL_THROW(#x) throw MException("POSIX error %d: '%s'", errno, strerror(errno)); } } while (false)

#define THROW(x) \
	do { SIGNAL_THROW(#x) throw MException x; } while (false)

#define THROW_IF_NIL(x) \
	do { if ((x) == nil) { SIGNAL_THROW(#x) throw MException("Nil pointer %s", #x); } } while (false)

#define THROW_ON_MACH_ERR(x)		\
	do { kern_return_t err = (x); if (err != KERN_SUCCESS) { SIGNAL_THROW(#x) throw MException("mach error %d: ('%s') calling %s", err, mach_error_string(err), #x); } } while (false)

#define THROW_EXCEPTION(e) \
	do { exception _e(e); SIGNAL_THROW(_e.what()) throw _e; } while (false)

#define THROW_WIN_ERROR(e) \
	do { MWinException _e(e); SIGNAL_THROW(_e.what()); throw _e; } while (false)

#define ASSERT(x, m)		if (not (x)) { SIGNAL_THROW(#x " => " #m); throw MException m; }

#endif
